#pragma once

#include <glm/glm.hpp>
#include <cmath>

namespace GameEngine::Physics::SPHKernels {

    inline float Poly6(float r2, float h) {
        if (r2 >= h * h) return 0.0f;
        float h2 = h * h;
        float diff = h2 - r2;
        return (315.0f / (64.0f * 3.14159265f * std::pow(h, 9.0f))) * diff * diff * diff;
    }

    inline glm::vec3 SpikyGrad(const glm::vec3& r, float rLen, float h) {
        if (rLen >= h || rLen < 1e-6f) return glm::vec3(0.0f);
        float diff = h - rLen;
        float coeff = -45.0f / (3.14159265f * std::pow(h, 6.0f)) * diff * diff;
        return coeff * (r / rLen);
    }

    inline float ViscosityLap(float r, float h) {
        if (r >= h) return 0.0f;
        return 45.0f / (3.14159265f * std::pow(h, 6.0f)) * (h - r);
    }

}
