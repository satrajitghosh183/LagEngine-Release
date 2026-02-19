#pragma once

#include "Component.hpp"
#include "../../Physics/Fluids/SPHSolver.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Component for fluid particle emission
     * 
     * Emits fluid particles from a point, with configurable rate,
     * velocity, and spread. Works with the SPH fluid solver.
     */
    class FluidEmitterComponent : public Component {
    public:
        /**
         * @brief Emitter shape type
         */
        enum class EmitterShape {
            Point = 0,
            Box,
            Sphere
        };
        
        FluidEmitterComponent() = default;
        ~FluidEmitterComponent() = default;
        
        COMPONENT_TYPE(FluidEmitterComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Get the fluid solver
         */
        Ref<Physics::SPHSolver> GetSolver() const { return m_Solver; }
        
        /**
         * @brief Set external solver (shared between multiple emitters)
         */
        void SetSolver(const Ref<Physics::SPHSolver>& solver) { m_Solver = solver; }
        
        /**
         * @brief Emitter properties
         */
        EmitterShape Shape = EmitterShape::Point;
        
        // Emission rate (particles per second)
        float EmissionRate = 100.0f;
        
        // Initial velocity
        glm::vec3 InitialVelocity = glm::vec3(0, -1, 0);
        float VelocityRandomness = 0.2f;
        
        // Shape dimensions
        glm::vec3 BoxSize = glm::vec3(1, 1, 1);
        float SphereRadius = 0.5f;
        
        // Particle lifetime (0 = infinite)
        float ParticleLifetime = 0.0f;
        
        // Simulation bounds
        glm::vec3 BoundsMin = glm::vec3(-10, -10, -10);
        glm::vec3 BoundsMax = glm::vec3(10, 10, 10);
        
        // Fluid properties
        float ParticleMass = 1.0f;
        float RestDensity = 1000.0f;
        float Viscosity = 0.018f;
        float SurfaceTension = 0.0728f;
        
        // State
        bool Emitting = true;
        bool SimulationEnabled = true;
        
        // Max particles
        int MaxParticles = 5000;
        
        /**
         * @brief Lifecycle callbacks
         */
        void OnCreate();
        void OnFixedUpdate(float fixedDeltaTime);
        void OnDestroy();
        
        /**
         * @brief Emit a burst of particles
         */
        void EmitBurst(int count);
        
        /**
         * @brief Get particle count
         */
        int GetParticleCount() const;
        
        /**
         * @brief Get particle positions for rendering
         */
        const std::vector<Physics::FluidParticle>& GetParticles() const;
        
        /**
         * @brief Clear all particles
         */
        void Clear();
        
    private:
        glm::vec3 GetEmitPosition() const;
        glm::vec3 GetEmitVelocity() const;
        
        Ref<Physics::SPHSolver> m_Solver;
        float m_EmissionAccumulator = 0.0f;
        
        // Random number generation helpers
        float RandomFloat(float min, float max) const;
        glm::vec3 RandomInUnitSphere() const;
    };

}
