#include "Utilities.hpp"

#include "../Swarm/Swarm.hpp"


void Utilities::singleColumnData(const std::string& filename, const std::vector<double>& data)
{
    std::ofstream outFile(filename + ".txt");
    checkFileOpen(outFile);

    for (unsigned int i = 0; i < data.size(); ++i)
    {
        outFile << data[i] << '\n';
    }

    outFile.close();
}

void Utilities::addLine(const std::string& filename, const std::vector<double>& data)
{
    std::ofstream outFile(filename + ".txt", std::ios::app);
    checkFileOpen(outFile);

    for (unsigned int i = 0; i < data.size(); ++i)
    {
        outFile << data[i];
        
        if (i < data.size() - 1) { outFile << ','; }
    }
    outFile << '\n';

    outFile.close();
}

template <std::size_t Dim>
void Utilities::parallelSims(float L, float scale, uint32_t seed, float eta, float gamma, float v, unsigned int numParticles, float dt, unsigned int max_frame)
{
    Swarm<Dim> swarm(L, scale, seed, eta, gamma, v, numParticles, 1);
    swarm.BC = false;

    // func wrapper for "write vals to file"
    // ************************************
    // MAKE SURE TO CREATE DIRECTORY FIRST!
    // ************************************

    std::string gamma_string = std::format("{:.1f}", swarm.gamma);
    std::string eta_string = std::format("{:.1f}", swarm.eta);
    std::string front_path = "/Users/max/UCLA/Research/Codes/Data/<path>/";

    std::string md_path { front_path + "meta_data_g_" + gamma_string + "_eta_" + eta_string };
    std::string d_path { front_path + "swarm_data_g_" + gamma_string + "_eta_" + eta_string };

    {
        std::ofstream md_file = openFile(md_path);
        addLineAlreadyOpen(md_file, "Dim", 'N', 'L', "master_seed", "eta", "gamma", "BC");
        addLineAlreadyOpen(md_file, Dim, numParticles, L, swarm.master_seed, swarm.eta, swarm.gamma, swarm.BC);
    } // md_file closes once out of scope here
    
    std::ofstream d_file = openFile(d_path);

    if (Dim == 2)
    {
        addLineAlreadyOpen(d_file, "id", 'x', 'y', "vx", "vy");
    }
    else
    {
        addLineAlreadyOpen(d_file, "id", 'x', 'y', 'z', "vx", "vy", "vz");
    }

    for (unsigned int frame = 0; frame < max_frame; ++frame)
    {
        swarm.update(dt);

        for (unsigned int i = 0; i < swarm.positions.size(); ++i)
        {
            if constexpr (Dim == 2)
            {
                const glm::vec2& pos  = swarm.positions[i];
                const glm::vec2& vhat = swarm.headings[i];
                // TODO: keep file open between writes
                addLineAlreadyOpen(d_file, i, pos.x, pos.y, vhat.x, vhat.y);
            }
            else
            {
                const glm::vec3& pos  = swarm.positions[i];
                const glm::vec3& vhat = swarm.headings[i];
                addLineAlreadyOpen(d_file, i, pos.x, pos.y, pos.z, vhat.x, vhat.y, vhat.z);
            }
        }
    } // d_file flushes and closes
}

// explicitly instantiate for the linker
template void Utilities::parallelSims<2>(float, float, uint32_t, float, float, float, unsigned int, float, unsigned int);
template void Utilities::parallelSims<3>(float, float, uint32_t, float, float, float, unsigned int, float, unsigned int);
