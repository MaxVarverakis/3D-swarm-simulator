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

    template <std::size_t Dim>
    void parallelSims(float L, float scale, uint32_t seed, float eta, float gamma, float v, unsigned int numParticles, float dt, unsigned int max_frame);

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

struct BoundingBox
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    int components;
};

template <std::size_t Dim>
BoundingBox createBoxData(float L)
{
    if constexpr (Dim == 2)
    {
        return BoundingBox{
            {
                0.0f, 0.0f,  L, 0.0f,  L, L,  L, 0.0f
            },
            {
                0, 1,  1, 2,  2, 3,  3, 0
            },
            2
        };
    }
    else
    {
        return BoundingBox{
            {
                0.0f, 0.0f, 0.0f,  L, 0.0f, 0.0f,  L, L, 0.0f,  0.0f, L, 0.0f, // back face
                0.0f, 0.0f,    L,  L, 0.0f,    L,  L, L,    L,  0.0f, L,    L, // front face
            },
            {
                0, 1,  1, 2,  2, 3,  3, 0, // back edges
                4, 5,  5, 6,  6, 7,  7, 4, // front edges
                0, 4,  1, 5,  2, 6,  3, 7 // side edges
            },
            3
        };
    }
}
