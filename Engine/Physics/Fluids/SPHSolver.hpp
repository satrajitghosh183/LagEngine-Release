#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Fluid particle
     */
    struct FluidParticle {
        glm::vec3 Position;
        glm::vec3 Velocity;
        glm::vec3 Force;
        float Density;
        float Pressure;
        
        FluidParticle() 
            : Position(0), Velocity(0), Force(0), Density(0), Pressure(0) {}
    };

    /**
     * @brief SPH (Smoothed Particle Hydrodynamics) fluid solver
     * 
     * Features:
     * - Navier-Stokes equations
     * - Pressure forces
     * - Viscosity
     * - Surface tension
     * - Spatial hashing for optimization
     */
    class SPHSolver {
    public:
        SPHSolver(int maxParticles = 10000);
        ~SPHSolver() = default;
        
        /**
         * @brief Update simulation
         */
        void Update(float deltaTime);
        
        /**
         * @brief Add particle
         */
        void AddParticle(const glm::vec3& position, const glm::vec3& velocity = glm::vec3(0));
        
        /**
         * @brief Clear all particles
         */
        void Clear();
        
        /**
         * @brief Get particles
         */
        const std::vector<FluidParticle>& GetParticles() const { return m_Particles; }
        
        /**
         * @brief Simulation parameters
         */
        float ParticleMass = 1.0f;
        float RestDensity = 1000.0f;      // Water density
        float GasConstant = 2000.0f;      // Stiffness
        float Viscosity = 0.018f;         // Water viscosity
        float SurfaceTension = 0.0728f;   // Water surface tension
        float SmoothingRadius = 0.1f;
        
        glm::vec3 Gravity = glm::vec3(0, -9.81f, 0);
        glm::vec3 BoundsMin = glm::vec3(-5, -5, -5);
        glm::vec3 BoundsMax = glm::vec3(5, 5, 5);
        float BoundsDamping = 0.3f;
        
    private:
        void ComputeDensityPressure();
        void ComputeForces();
        void Integrate(float deltaTime);
        void HandleBoundaries();
        
        // SPH kernels
        float Poly6Kernel(float r);
        glm::vec3 SpikyKernelGradient(const glm::vec3& r);
        float ViscosityKernelLaplacian(float r);
        
        // Spatial hashing
        void BuildSpatialHash();
        std::vector<int> GetNearbyParticles(int particleIndex);
        
        int HashPosition(const glm::vec3& position);
        
    private:
        std::vector<FluidParticle> m_Particles;
        int m_MaxParticles;
        
        // Spatial hash grid
        std::unordered_map<int, std::vector<int>> m_SpatialHash;
        int m_GridCellSize;
        
        // Precomputed kernel constants
        float m_Poly6Constant;
        float m_SpikyConstant;
        float m_ViscosityConstant;
    };

}}