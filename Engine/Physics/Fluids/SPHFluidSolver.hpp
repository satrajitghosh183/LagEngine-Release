#pragma once

#include "SpatialHashGrid.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine::Physics {

    struct FluidParticle {
        glm::vec3 position{0.0f};
        glm::vec3 velocity{0.0f};
        float density = 0.0f;
        float pressure = 0.0f;
        glm::vec3 force{0.0f};
    };

    class SPHFluidSolver {
    public:
        void Init(int numParticles, const glm::vec3& domainMin, const glm::vec3& domainMax);
        void Update(float dt);
        void Clear();

        const std::vector<FluidParticle>& GetParticles() const { return m_Particles; }
        std::vector<glm::vec3> GetPositions() const;

        // Configurable parameters
        float SmoothingLength = 0.045f;
        float RestDensity = 1000.0f;
        float PressureCoefficient = 1.6f;
        float ViscosityCoefficient = 0.15f;
        float BounceCoefficient = 0.4f;
        glm::vec3 Gravity = {0.0f, -9.81f, 0.0f};
        glm::vec3 DomainMin = {-1.0f, -1.0f, -1.0f};
        glm::vec3 DomainMax = {1.0f, 1.0f, 1.0f};

    private:
        void ComputeDensityPressure();
        void ComputeForces();
        void Integrate(float dt);
        void HandleBoundaries();

        std::vector<FluidParticle> m_Particles;
        SpatialHashGrid m_Grid;
    };

}
