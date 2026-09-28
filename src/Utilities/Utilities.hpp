#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/color_space.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// currently no need to forward declare Swarm struct!

static const float PI { std::acosf(-1.0f) };

namespace Utilities
{
    void singleColumnData(const std::string& filename, const std::vector<double>& data);
    void addLine(const std::string& filename, const std::vector<double>& data);

    template <typename FileStream>
    void checkFileOpen(const FileStream& file)
    {
        if (!file.is_open()) {
            throw std::ios_base::failure("Failed to open file!");
        }
    };

    
    template<typename... Args>
    void addLine(const std::string& filename, const Args&... args)
    {
        std::ofstream outFile(filename + ".txt", std::ios::app);
        checkFileOpen(outFile);

        bool first = true;
        ((outFile << (first ? "" : ",") << args, first = false), ...);
        outFile << '\n';

        outFile.close();
    }

    void parallelSims(float width, float height, float scaleFactor, uint32_t seed, float scaleNoise, float v, unsigned int numParticles, const float dt);

    // Fast, deterministic 32-bit hash function
    // https://gist.github.com/badboy/6267743#using-multiplication-for-hashing
    // Thomas Wang
    inline uint32_t hash32(uint32_t a)
    {
        a = (a ^ 61) ^ (a >> 16);
        a = a + (a << 3);
        a = a ^ (a >> 4);
        a = a * 0x27d4eb2d;
        a = a ^ (a >> 15);
        return a;
    }

}
