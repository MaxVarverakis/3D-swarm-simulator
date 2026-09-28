#include "Swarm.hpp"

void Swarm::apply3DNoise(unsigned int pID, uint32_t frameHash, glm::vec3& vhat)
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

void Swarm::mfSense(unsigned int pID, uint32_t frameHash, float gamma)
{
    // `mfSense` updates heading based off weighted MF vector
    
    glm::vec3& position = positions[pID];
    
    // `headings` contains unit vectors \hat{v}
    glm::vec3& vhat = headings[pID];

    glm::vec3 numerator(0.0f);
    float denominator = 0.0f;

    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        if (i != pID)
        {
            glm::vec3 delta = positions[i] - position;
            // shortestDistance(delta); // only needed for periodic BCs
            float d = (gamma == 0.0f) ? 1.0f : 1.0f / powf(glm::length(delta), gamma);

            numerator   += delta * d;
            denominator += d;
        }
    }

    glm::vec3 D = numerator / denominator;

    // perform vector rejection
    vhat += PI / L * ( D - glm::dot(D, vhat) * vhat );
    
    // enforce heading vector is normalized
    vhat = glm::normalize(vhat);

    apply3DNoise(pID, frameHash, vhat);
}

void Swarm::tradSense(unsigned int pID, uint32_t frameHash)
{
    static const float senseRadius = 10;

    glm::vec3 headingSum = headings[pID];
    glm::vec3& position = positions[pID];

    for (unsigned int i = 0; i < positions.size(); ++i)
    {
        if (i == pID) continue;

        glm::vec3 delta = position - positions[i];
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
    apply3DNoise(pID, frameHash, headings[pID]);
}
