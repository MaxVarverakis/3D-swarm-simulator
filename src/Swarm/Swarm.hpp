#pragma once

#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <omp.h>

#include "../Utilities/Utilities.hpp"

struct Swarm
{
    float L, x_max, y_max, z_max;
    float scale;
    uint32_t master_seed;
    float eta;

    float velocity;

    std::vector<float> targetPolarAngles;
    std::vector<float> targetAzimuthalAngles;
    std::vector<glm::vec3> positions, headings;
    
    unsigned int numThreads;

    uint64_t currentFrame;
    
    bool BC { true };

    Swarm(float set_L, float scaleShape, uint32_t seed, float noise, float v, unsigned int numParticles, int num_threads, uint64_t frame = 0)
        : L { set_L }
        , x_max { L }
        , y_max { L }
        , z_max { L }
        , scale { scaleShape }
        , master_seed { seed }
        , eta { noise }
        , velocity { v }
        , numThreads { static_cast<unsigned int>(num_threads) }
        , currentFrame { frame }
    {
        positions.reserve(numParticles);
        headings.reserve(numParticles);
        
        initThreadData(num_threads);

        // generate random particles
        for (unsigned int i = 0; i < numParticles; ++i)
        {
            float phi = PI * deterministicRNG(i, 0, 0);
            float theta = PI * deterministicRNG(i, 0, 1);
            positions.emplace_back(
                glm::vec3(x_max * 0.5f * (deterministicRNG(i, 0, 2) + 1.0f), y_max * 0.5f * (deterministicRNG(i, 0, 3) + 1.0f), z_max * 0.5f * (deterministicRNG(i, 0, 4) + 1.0f))
            );
            float cp = glm::cos(phi);
            float sp = glm::sin(phi);
            float ct = glm::cos(theta);
            float st = glm::sin(theta);
            headings.emplace_back(
                st * cp, st * sp, ct
            );
        }
    }

    void initThreadData(int num_threads)
    {
        omp_set_num_threads(num_threads);
    }

    void applyWallBC(glm::vec3& position, glm::vec3& heading)
    {
        if (position.x >= x_max)
        {
            position.x = x_max - (position.x - x_max);
            heading.x = -std::abs(heading.x);
        }
        else if (position.x < 0.0f)
        {
            position.x = -position.x;
            heading.x = std::abs(heading.x);
        }

        if (position.y >= y_max)
        {
            position.y = y_max - (position.y - y_max);
            heading.y = -std::abs(heading.y);
        }
        else if (position.y < 0.0f)
        {
            position.y = -position.y;
            heading.y = std::abs(heading.y);
        }

        if (position.z >= z_max)
        {
            position.z = z_max - (position.z - z_max);
            heading.z = -std::abs(heading.z);
        }
        else if (position.z < 0.0f)
        {
            position.z = -position.z;
            heading.z = std::abs(heading.z);
        }
    }

    void applyPeriodicBC(glm::vec3& position)
    {
        if (position.x >= x_max) { position.x -= x_max; }
        else if (position.x < 0.0f) { position.x += x_max; }

        if (position.y >= y_max) { position.y -= y_max; }
        else if (position.y < 0.0f) { position.y += y_max; }

        if (position.z >= z_max) { position.z -= z_max; }
        else if (position.z < 0.0f) { position.z += z_max; }
    }

    void shortestDistance(glm::vec3& delta)
    {
        float half_x = x_max / 2.0f;
        float half_y = y_max / 2.0f;
        float half_z = z_max / 2.0f;
        
        if (delta.x > half_x)       { delta.x -= x_max; }
        else if (delta.x < -half_x) { delta.x += x_max; }

        if (delta.y > half_y)       { delta.y -= y_max; }
        else if (delta.y < -half_y) { delta.y += y_max; }

        if (delta.z > half_z)       { delta.z -= z_max; }
        else if (delta.z < -half_z) { delta.z += z_max; }
    }
    
    void apply3DNoise(unsigned int pID, uint32_t frameHash, glm::vec3& vhat);

    void tradSense(unsigned int pID, uint32_t frameHash);

    void mfSense(unsigned int pID, uint32_t frameHash, float gamma);

    void updateParticle(unsigned int pID, float dt)
    {
        // v * dt == l which can be set to 1 s.t. only L = pi m v0 / k dt defines time step
        positions[pID] += velocity * dt * headings[pID];
        if (BC)
        {
            applyWallBC(positions[pID], headings[pID]);
        }
    }

    void update(float dt)
    {
        uint32_t frameHash = Utilities::hash32(static_cast<uint32_t>(currentFrame) + 0x85ebca6bu); // from MurmurHash3 

        #pragma omp parallel for schedule(guided)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            mfSense(idx, frameHash, 3.0f);
        }

        #pragma omp parallel for schedule(static, 256)
        for (std::size_t i = 0; i < static_cast<std::size_t>(positions.size()); ++i)
        {
            unsigned int idx = static_cast<unsigned int>(i);
            updateParticle(idx, dt);
        }

        currentFrame++;
    }

    inline float deterministicRNG(uint32_t pID, uint32_t frameHash, uint32_t sequence = 0) 
    {
        // mix the inputs to prevent sequential pID correlations
        // fibonacci hash the particle IDs
        uint32_t state = master_seed ^ frameHash ^ Utilities::hash32(pID + sequence * 0x9e3779b9u);
        uint32_t h = Utilities::hash32(state);
        
        return static_cast<float>(h) * (2.0f / 4294967295.0f) - 1.0f; // [-1, 1]
    }
};
