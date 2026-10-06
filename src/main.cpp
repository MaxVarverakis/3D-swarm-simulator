#include <iostream>
#include <math.h>
#include <random>
#include <thread>
#include <iomanip>
#include <chrono>

// set priority
#include <unistd.h>
#include <cerrno>
#include <sys/resource.h>

// OpenGL helpers
#include "VertexBuffer/VertexBuffer.hpp"
#include "IndexBuffer/IndexBuffer.hpp"
#include "VertexArray/VertexArray.hpp"
#include "Shader/Shader.hpp"
#include "VertexBufferLayout/VertexBufferLayout.hpp"
#include "Triangle/Triangle.hpp"

// ImGui stuff here
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

// project-specific includes go here
#include "Swarm/Swarm.hpp"

SDL_Window* window;
SDL_GLContext gl_context;

const int num_threads { static_cast<int>(std::thread::hardware_concurrency()) };

const bool renderGraphics { true };

bool is_running;
bool paused { true };
bool step { false };

float play_speed { 1.0f };
float max_play_speed { 5.0f };
float min_play_speed { 0.05f };
float base_target_fps { 60.0f };

const float width { 800.0f };
const float height { 800.0f };


// project-specific settings
constexpr std::size_t DIM { 3 };

const float L { 100.0f };
const float shape_scale { DIM == 3 ? 1.0f : 0.5f };
const unsigned int numObjs { 100 };

float v_magnitude { 1.0f };
float sim_DT { 1.0f / v_magnitude };

float noise { 0.1f };

bool color { true };
bool border { true };
bool alreadyDefault { false };
bool ui_collapsed { true };

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float cameraRadius = 2.2f * L;
float rotationSpeed = glm::radians(90.0f); // per IRL second

// initialize random
std::random_device rd;

void set_sdl_gl_attributes()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    // nice to have here (slows things down!)
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1); // Enable multisampling
    // SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); // 4x MSAA (Sweet spot for quality/performance)
}

void checkKeysPressed(SDL_Event& event)
{
    if (event.type == SDL_KEYDOWN)
    {
        switch (event.key.keysym.sym)
        {
            case SDLK_ESCAPE:
                is_running = false;
                break;
            case SDLK_SPACE:
                paused = !paused;
                break;
            case SDLK_f:
                step = true;
                break;
            case SDLK_d:
                play_speed = fminf(play_speed + 0.05f, max_play_speed);
                break;
            case SDLK_s:
                play_speed = fmaxf(play_speed - 0.05f, min_play_speed);
                break;
            case SDLK_c:
                color = !color;
                break;
            case SDLK_m:
                ui_collapsed = !ui_collapsed;
                break;
            case SDLK_b:
                border = !border;
                break;
            default:
                break;
        }
    }
}

void printKey()
{
    std::cout << '\n' << "###################################" << '\n';
    std::cout << "KEY" << '\n';
    std::cout << "Spacebar" << '\t' << "Play/pause" << '\n';
    std::cout << "f" << '\t' << '\t' << "Time forward" << '\n';
    std::cout << "c" << '\t' << '\t' << "Colors on/off" << '\n';
    std::cout << "m" << '\t' << '\t' << "Expand/minimize GUI" << '\n';
    std::cout << "L/R arrow" << '\t' << "Rotate azimuthally" << '\n';
    std::cout << "Up/Dn arrow" << '\t' << "Rotate polar/pitch" << '\n';
    std::cout << "###################################" << '\n' << '\n';
}

template <std::size_t Dim>
void imguiWindow(ImGuiIO& io, Swarm<Dim>& swarm)
{
    ImGui::SetNextWindowCollapsed(ui_collapsed, ImGuiCond_Always);

    ImGui::Begin("Swarm Control Center");

    // keep bool synced with ui state
    ui_collapsed = ImGui::IsWindowCollapsed();

    // Simulation Play/Pause State
    if (paused) {
        if (ImGui::Button("Resume Simulation", ImVec2(150, 0))) { paused = false; }
    } else {
        if (ImGui::Button("Pause Simulation", ImVec2(150, 0))) { paused = true; }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Step Forward")) { step = true; }
    
    ImGui::SameLine();
    if (ImGui::Button("Quit")) { is_running = false; }

    ImGui::Separator();

    ImGui::Text("Simulation Speed & Playback:");
    ImGui::SliderFloat("Speed Multiplier", &play_speed, min_play_speed, max_play_speed, "%.2fx");
    // ImGui::SliderFloat("Target FPS", &base_target_fps, 10.0f, fmaxf(base_target_fps, 240.0f), "%.0f FPS");
    

    // Live Parameter Tuning
    // Note: We expose &noise directly since it controls the swarm logic
    ImGui::Text("Physics Parameters:");
    ImGui::SliderFloat("Noise Scale (Eta)", &noise, 0.0f, 1.0f, "%.3f");
    
    // modify agent colors
    ImGui::Checkbox("Heading-based colors", &color);
    ImGui::Checkbox("Show bounding box", &border);
    
    // swarm info
    ImGui::Text("Particle Count: %u", numObjs);
    ImGui::Text("Frame Count: %llu", swarm.currentFrame);
    // ImGui::Text("Simulation Time: %.2f s", static_cast<double>(sim_DT) * static_cast<double>(swarm.currentFrame));

    ImGui::Separator();

    // Real-Time Performance Overlays
    ImGui::Text("Application Metrics:");
    ImGui::Text("Frame Rate: %.1f FPS", io.Framerate);

    // ImGui::Separator();

    // // velocity order parameter
    // // Maintain a static rolling buffer of historical data (e.g., last 200 frames)
    // const int HISTORY_SIZE = 200;
    // static float phi_history[HISTORY_SIZE] = { 0.0f };
    // static int head = 0;

    // if (!paused)
    // {
    //     phi_history[head] = swarm.order_param;
    //     head = (head + 1) % HISTORY_SIZE;
    // }

    // // Render the scrolling line graph
    // // Arguments: Label, data array, array size, values offset, overlay text, min value, max value, graph size
    // ImGui::PlotLines("##PhiGraph", phi_history, HISTORY_SIZE, head, "Global Order (Phi)", 0.0f, 1.0f, ImVec2(0, 100));
    
    ImGui::End();
}

int main()
{
    if (renderGraphics)
    {
        if(SDL_Init(SDL_INIT_EVERYTHING)==0)
        {
            std::cout<<"SDL2 initialized successfully."<<std::endl;
            set_sdl_gl_attributes();
            
            // specifying the number of agents automatically generates random particles
            Swarm<DIM> swarm(L, shape_scale, rd(), noise, v_magnitude, numObjs, num_threads, 0);
            // swarm.BC = false;
            
            window = SDL_CreateWindow("Swarm Simulation", 0.0f, 0.0f, static_cast<int>(width), static_cast<int>(height), SDL_WINDOW_OPENGL);
            gl_context = SDL_GL_CreateContext(window);
            SDL_GL_SetSwapInterval(0);

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

            // Setup Platform/Renderer backends
            ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
            ImGui_ImplOpenGL3_Init();

            if(glewInit() == GLEW_OK)
            {
                std::cout << "GLEW initialization successful" << std::endl;
            }
            else
            {
                std::cout << "GLEW initialization failed" << std::endl;
                return -1;
            }

            GLCall(glEnable(GL_BLEND));
            if constexpr (DIM == 2)
            {
                GLCall(glDisable(GL_DEPTH_TEST));
            }
            else
            {
                GLCall(glEnable(GL_DEPTH_TEST));
            }
            GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

            // GLCall(glEnable(GL_MULTISAMPLE));
            // GLCall(glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE));

            printKey();
            std::cout << "seed" << '\t' << swarm.master_seed << '\n' << '\n';

            // buffer stuff and initialization happens here
            std::vector<Triangle> tris;
            tris.reserve(numObjs);
            
            // create triangle (only need one since instance drawing!)
            tris.emplace_back(Triangle(glm::vec2(0.0f, 2.0f), glm::vec2(1.7321f, -1.0f), glm::vec2(-1.7321f, -1.0f), glm::fvec4(1.0f)));
            // tris.emplace_back(Triangle(glm::vec2(0.0f, 0.0f), shape_scale * glm::vec2(width/2.0f, 0.0f), shape_scale * glm::vec2(width/4.0f, height), glm::fvec4(1.0f)));

            // prepare triangles for graphics
            Triangles triangles(tris);

            VertexArray tri_VAO;

            // constructor automatically binds buffer
            VertexBuffer tri_VBO(triangles.m_vertices.data(), static_cast<unsigned int>(triangles.m_vertices.size() * sizeof(float)), GL_STATIC_DRAW);

            VertexBufferLayout tri_layout;
            for (int i = 0; i < 2; ++i)
            {
                tri_layout.push<float>(Triangle::layout_descriptor[i]);
            }
            tri_VAO.addBuffer(tri_VBO, tri_layout);
            
            VertexBuffer posInstanceVBO(swarm.positions.data(), static_cast<unsigned int>(swarm.positions.size() * sizeof(swarm.positions[0])), GL_DYNAMIC_DRAW);
            VertexBuffer dirInstanceVBO(swarm.headings.data(), static_cast<unsigned int>(swarm.headings.size() * sizeof(swarm.headings[0])), GL_DYNAMIC_DRAW);

            tri_VAO.addInstancedBuffer(posInstanceVBO, DIM, 2); // location = 2
            tri_VAO.addInstancedBuffer(dirInstanceVBO, DIM, 3); // location = 3

            // constructor automatically binds buffer
            IndexBuffer tri_IBO(triangles.m_indices.data(), static_cast<unsigned int>(triangles.m_indices.size()));

            tri_VBO.unbind();
            tri_VAO.unbind();
            tri_IBO.unbind();

            // bounding box setup
            BoundingBox box = createBoxData<DIM>(L);

            VertexArray box_VAO;
            VertexBuffer box_VBO(box.vertices.data(), static_cast<unsigned int>(box.vertices.size() * sizeof(float)), GL_STATIC_DRAW);

            // add buffer to VAO
            box_VAO.bind();
            box_VBO.bind();
            GLCall(glEnableVertexAttribArray(0));
            GLCall(glVertexAttribPointer(0, box.components, GL_FLOAT, GL_FALSE, box.components * static_cast<int>(sizeof(float)), reinterpret_cast<const void*>(0))); // location = 0 since new VAO

            // create IBO, which is automatically bound to current VAO in the constructor
            IndexBuffer box_IBO(box.indices.data(), static_cast<unsigned int>(box.indices.size()));

            box_VBO.unbind();
            box_VAO.unbind();
            box_IBO.unbind();

            glm::mat4 proj;
            glm::mat4 view;
            glm::vec3 domainCenter(0.0f);
            glm::vec3 cameraPos(0.0f);

            float aspect = width / height;

            // set up view/projection matrices
            if constexpr (DIM == 2)
            {
                proj = glm::ortho(0.0f, L * aspect, 0.0f, L, -1.0f, 1.0f);
                view = glm::mat4(1.0f);
            }
            else
            {
                proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
                domainCenter = glm::vec3(L * 0.5f, L * 0.5f, L * 0.5f);
                cameraPos = glm::vec3(L * 0.5f, L * 0.5f, cameraRadius); // backed up along z-direction
                view = glm::lookAt(cameraPos, domainCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            }

            // shader stuff happens here
            Shader tri_shader("/Users/max/UCLA/Research/Codes/3DMF/shaders", "circle_cheat_");
            tri_shader.bind();
            tri_shader.setUniformMatrix4fv("u_View", view);
            tri_shader.setUniformMatrix4fv("u_Proj", proj);
            tri_shader.setUniform1f("u_scale", shape_scale);
            tri_shader.setUniform1i("u_color", color);
            tri_shader.setUniform1i("u_2D", DIM == 2);
            tri_shader.unbind();

            Shader line_shader("/Users/max/UCLA/Research/Codes/3DMF/shaders", "line_");
            line_shader.bind();
            line_shader.setUniformMatrix4fv("u_View", view);
            line_shader.setUniformMatrix4fv("u_Proj", proj);
            line_shader.setUniform1i("u_border", border && DIM == 3);
            line_shader.unbind();

            Renderer renderer;

            // Main loop
            SDL_Event event;
            is_running = true;

            auto last_time = std::chrono::high_resolution_clock::now();

            while(is_running)
            {
                // Record start time of this step
                [[maybe_unused]] auto step_start = std::chrono::high_resolution_clock::now();
                auto current_time = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float>(current_time - last_time).count();
                last_time = current_time;

                while(SDL_PollEvent(&event))
                {
                    ImGui_ImplSDL2_ProcessEvent(&event); // Forward event to backend

                    if (event.type == SDL_QUIT)
                    {
                        is_running = false;
                    }
                    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    {
                        is_running = false;
                    }

                    checkKeysPressed(event);
                }

                if constexpr (DIM == 3)
                {
                    const Uint8* keyState = SDL_GetKeyboardState(NULL);

                    if (keyState[SDL_SCANCODE_LEFT])  cameraYaw   += rotationSpeed * dt;
                    if (keyState[SDL_SCANCODE_RIGHT]) cameraYaw   -= rotationSpeed * dt;
                    if (keyState[SDL_SCANCODE_UP])    cameraPitch -= rotationSpeed * dt;
                    if (keyState[SDL_SCANCODE_DOWN])  cameraPitch += rotationSpeed * dt;

                    // prevent flipping
                    cameraPitch = glm::clamp(cameraPitch, glm::radians(-89.0f), glm::radians(89.0f));

                    // orbit camera to match the keys pressed
                    cameraPos.z = domainCenter.z + cameraRadius * cosf(cameraPitch) * cosf(cameraYaw);
                    cameraPos.x = domainCenter.x + cameraRadius * cosf(cameraPitch) * sinf(cameraYaw);
                    cameraPos.y = domainCenter.y + cameraRadius * sinf(cameraPitch);

                    // rebuild view matrix
                    view = glm::lookAt(cameraPos, domainCenter, glm::vec3(0.0f, 1.0f, 0.0f)); 
                }

                // (After event loop)
                // Start the Dear ImGui frame
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplSDL2_NewFrame();
                ImGui::NewFrame();
                imguiWindow(io, swarm);

                // evolve time if unpaused
                if (not paused)
                {
                    swarm.eta = noise;
                    swarm.update(sim_DT);
                }
                if (step)
                {
                    swarm.eta = noise;
                    swarm.update(sim_DT);
                    step = false;
                }
                
                // updating and rendering stuff happens here
                // update buffers (position, color, alpha, etc.)
                posInstanceVBO.updateBuffer(swarm.positions.data());
                dirInstanceVBO.updateBuffer(swarm.headings.data());

                tri_shader.bind();
                tri_shader.setUniformMatrix4fv("u_View", view);
                tri_shader.setUniform1i("u_color", color);
                tri_shader.unbind();

                line_shader.bind();
                line_shader.setUniformMatrix4fv("u_View", view);
                line_shader.setUniform1i("u_border", border && DIM == 3);
                line_shader.unbind();

                renderer.clear();

                // draw objects
                renderer.drawTriangles(tri_VAO, tri_IBO, tri_shader, static_cast<int>(swarm.positions.size()));
                renderer.drawLines(box_VAO, box_IBO, line_shader);

                // Dear ImGui Rendering
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                SDL_GL_SwapWindow(window);

                // -------------------------------------------------------------
                // FRAME PACING / SPEED CONTROL
                // -------------------------------------------------------------
                if (!paused && play_speed > 0.0f)
                {
                    // Desired duration for this step in seconds
                    double target_step_duration = 1.0 / (base_target_fps * play_speed);

                    auto step_end = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double> elapsed = step_end - step_start;

                    if (elapsed.count() < target_step_duration)
                    {
                        // Calculate remaining time to wait
                        double sleep_needed = target_step_duration - elapsed.count();

                        // Hybrid Sleep & Busy-Wait for smooth pacing on OS schedulers
                        auto sleep_target = step_start + std::chrono::duration<double>(target_step_duration);
                        
                        // Sleep for most of the duration to avoid eating CPU
                        if (sleep_needed > 0.002) // if more than 2ms left
                        {
                            std::this_thread::sleep_for(std::chrono::duration<double>(sleep_needed - 0.0015));
                        }
                        // Spin-lock for the remaining fraction of a millisecond for precision
                        while (std::chrono::high_resolution_clock::now() < sleep_target)
                        {
                            // yield thread execution
                            std::this_thread::yield();
                        }
                    }
                }
            }

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL2_Shutdown();
            ImGui::DestroyContext();
            SDL_Quit();
        }
        else
        {
            std::cout<<"SDL2 initialization failed."<<std::endl;
            return -1;
        }

        return 0;
    }
    else
    {
        // parallel compute no graphics

        // TODO: thread pooling
        std::cout << "Thread count: " << num_threads << '\n';

        // std::vector<unsigned int> NNList{1, 2, 4, 8, 16, 24};
        // std::vector<std::thread> workers;
        // workers.reserve((NNList.size()));

        // for (unsigned int i = 0; i < NNList.size(); ++i)
        // {
        //     workers.emplace_back(&Utilities::parallelSims, width, height, shape_scale, rd(), noise, NNList[i], v_magnitude, numObjs, sim_DT);
        // }
        
        // for (auto& thread : workers)
        // {
        //     thread.join();
        // }
    }
}
