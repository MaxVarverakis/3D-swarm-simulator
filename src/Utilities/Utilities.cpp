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

    // func wrapper for "write vals to file"
    // ************************************
    // MAKE SURE TO CREATE DIRECTORY FIRST!
    // ************************************

    std::string gamma_string = std::format("{:.1f}", swarm.gamma);
    std::string eta_string = std::format("{:.1f}", swarm.eta);

    std::string md_path { "/Users/max/UCLA/Research/Codes/Data/JF2026_Fig2_Gamma_Eta_sweep/meta_data_g_" + gamma_string + "_eta_" + eta_string };
    std::string d_path { "/Users/max/UCLA/Research/Codes/Data/JF2026_Fig2_Gamma_Eta_sweep/swarm_data_g_" + gamma_string + "_eta_" + eta_string };

    Utilities::addLine(md_path, "Dim", 'N', 'L', "master_seed", "eta", "gamma", "BC");
    Utilities::addLine(md_path, Dim, numParticles, L, swarm.master_seed, swarm.eta, swarm.gamma, swarm.BC);
    
    if (Dim == 2)
    {
        Utilities::addLine(d_path, "id", 'x', 'y', "vx", "vy");
    }
    else
    {
        Utilities::addLine(d_path, "id", 'x', 'y', 'z', "vx", "vy", "vz");
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
                Utilities::addLine(d_path, i, pos.x, pos.y, vhat.x, vhat.y);
            }
            else
            {
                const glm::vec3& pos  = swarm.positions[i];
                const glm::vec3& vhat = swarm.headings[i];
                Utilities::addLine(d_path, i, pos.x, pos.y, pos.z, vhat.x, vhat.y, vhat.z);
            }
        }
    }
}

// explicitly instantiate for the linker
template void Utilities::parallelSims<2>(float, float, uint32_t, float, float, float, unsigned int, float, unsigned int);
template void Utilities::parallelSims<3>(float, float, uint32_t, float, float, float, unsigned int, float, unsigned int);
