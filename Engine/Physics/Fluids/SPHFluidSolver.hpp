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

        /**
         * @brief Initialize a rectangular block of water particles — ported from old_code/src/sph_water.cpp
         * @param nx/ny/nz Grid dimensions (total particles = nx*ny*nz)
         * @param spacing  Particle spacing (should be slightly larger than SmoothingLength)
         * @param origin   Bottom-front-left corner of the block
         */
        void InitBlock(int nx, int ny, int nz, float spacing, const glm::vec3& origin);

        /**
         * @brief Dynamically add a single particle (for emitter-style effects)
         */
        void AddParticle(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0.0f));

        void Update(float dt);
        void Clear();

        int  GetParticleCount() const { return static_cast<int>(m_Particles.size()); }
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
