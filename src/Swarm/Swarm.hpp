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
#include <type_traits>

#include "../Utilities/Utilities.hpp"

template <std::size_t Dim = 3>
struct Swarm
{
    static_assert(Dim == 2 || Dim == 3, "Swarm only supports 2 or 3 dimensions!");
    using vec = std::conditional_t<Dim == 2, glm::vec2, glm::vec3>;

    float L;
    vec domainMax;
    float scale, eta, gamma, velocity;
    uint32_t master_seed;

    std::vector<vec> positions, headings;

    unsigned int numThreads;

    uint64_t currentFrame;

    bool BC { true };

    Swarm(float set_L, float set_scale, uint32_t seed, float noise, float set_gamma, float v, unsigned int numParticles, int num_threads, uint64_t frame = 0)
        : L { set_L }
        , domainMax { vec(set_L) }
        , scale { set_scale }
        , eta { noise }
        , gamma { set_gamma }
        , velocity { v }
        , master_seed { seed }
        , numThreads { static_cast<unsigned int>(num_threads) }
        , currentFrame { frame }
    {
        positions.reserve(numParticles);
        headings.reserve(numParticles);

        initThreadData(num_threads);

        // generate random particles
        if constexpr (Dim == 2)
        {
            for (unsigned int i = 0; i < numParticles; ++i)
            {
                float theta = PI * deterministicRNG(i, 0, 1);
                headings.emplace_back(glm::cos(theta), glm::sin(theta));

                positions.emplace_back(
                        glm::vec2(
                            domainMax.x * 0.5f * (deterministicRNG(i, 0, 2) + 1.0f),
                            domainMax.y * 0.5f * (deterministicRNG(i, 0, 3) + 1.0f)
                        )
                    );
            }
        }
        else
        {
            for (unsigned int i = 0; i < numParticles; ++i)
            {
                float phi = PI * deterministicRNG(i, 0, 0);
                float theta = PI * deterministicRNG(i, 0, 1);

                float cp = glm::cos(phi);
                float sp = glm::sin(phi);
                float ct = glm::cos(theta);
                float st = glm::sin(theta);
                headings.emplace_back(st * cp, st * sp, ct);

                positions.emplace_back(
                    glm::vec3(
                        domainMax.x * 0.5f * (deterministicRNG(i, 0, 2) + 1.0f),
                        domainMax.y * 0.5f * (deterministicRNG(i, 0, 3) + 1.0f),
                        domainMax.z * 0.5f * (deterministicRNG(i, 0, 4) + 1.0f)
                    )
                );
            }
        }
    }

    void initThreadData(int num_threads)
    {
        omp_set_num_threads(num_threads);
    }

    void applyWallBC(vec& position, vec& heading)
    {
        for (int d = 0; d < static_cast<int>(Dim); ++d)
        {
            if (position[d] >= domainMax[d])
            {
                position[d] = domainMax[d] - (position[d] - domainMax[d]);
                heading[d] = -std::abs(heading[d]);
            }
            else if (position[d] < 0.0f)
            {
                position[d] = -position[d];
                heading[d] = std::abs(heading[d]);
            }
        }
    }

    void applyPeriodicBC(vec& position)
    {
        for (int d = 0; d < static_cast<int>(Dim); ++d)
        {
            if (position[d] > domainMax[d]) { position[d] -= domainMax[d]; }
            else if (position[d] < 0.0f) { position[d] += domainMax[d]; }
        }
    }

    void shortestDistance(vec& delta)
    {
        for (int d = 0; d < static_cast<int>(Dim); ++d)
        {
            float half_L = domainMax[d] / 2.0f;

            if (delta[d] > half_L) { delta[d] -= half_L; }
            else if (delta[d] < -half_L) { delta[d] += half_L; }
        }
    }

    void applyNoise(unsigned int pID, uint32_t frameHash, vec& vhat)
    {
        if constexpr (Dim == 2)
        {
            // generate random angle for noise
            float angle = glm::atan(vhat.y, vhat.x);
            float dtheta = eta * PI * deterministicRNG(pID, frameHash, 8);
            vhat = glm::vec2(glm::cos(angle + dtheta), glm::sin(angle + dtheta));
        }
        else
        {
            // generate random vector for noise
            glm::vec3 r = glm::vec3(
                deterministicRNG(pID, frameHash, 5),
                deterministicRNG(pID, frameHash, 6),
                deterministicRNG(pID, frameHash, 7)
            );

            // obtain normalized vector rejection (v perp) of r onto heading (\hat{v})
            glm::vec3 vp = glm::normalize(r - glm::dot(r, vhat) * vhat);

            // generate noise angle
            float angle = eta * PI * deterministicRNG(pID, frameHash, 8);

            // apply noise to vhat by rotating in the vhat-vp plane by `angle`
            vhat = vhat * glm::cos(angle) + vp * glm::sin(angle);
        }
    }

    void mfSense(unsigned int pID, uint32_t frameHash)
    {
        // `mfSense` updates heading based off weighted MF vector
        
        vec& position = positions[pID];
        
        // `headings` contains unit vectors \hat{v}
        vec& vhat = headings[pID];

        vec numerator(0.0f);
        float denominator = 0.0f;

        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            if (i != pID)
            {
                vec delta = positions[i] - position;
                // shortestDistance(delta); // only needed for periodic BCs
                float d = (gamma == 0.0f) ? 1.0f : 1.0f / powf(glm::length(delta), gamma);

                numerator   += delta * d;
                denominator += d;
            }
        }

        vec D = numerator / denominator;

        // perform vector rejection
        vhat += PI / L * ( D - glm::dot(D, vhat) * vhat );
        
        // enforce heading vector is normalized
        vhat = glm::normalize(vhat);

        applyNoise(pID, frameHash, vhat);
    }

    void tradSense(unsigned int pID, uint32_t frameHash)
    {
        static const float senseRadius = 10;

        vec headingSum = headings[pID];
        vec& position  = positions[pID];

        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            if (i == pID) continue;

            vec delta = position - positions[i];
            // account for periodic BCs:
            // shortestDistance(delta);

            float d2 = glm::dot(delta, delta);

            if (d2 < senseRadius * senseRadius)
            {
                float weight = 1.0f;
                // float weight = (d2 < 0.1f) ? 1.0f / 0.1f : 1.0f / d2;

                headingSum += headings[i] * weight;
            }
        }
        
        headings[pID] = glm::normalize(headingSum);
        applyNoise(pID, frameHash, headings[pID]);
    }

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
            mfSense(idx, frameHash);
            // tradSense(idx, frameHash);
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
