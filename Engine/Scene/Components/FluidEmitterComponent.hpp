#pragma once

#include "Component.hpp"
#include "../../Physics/Fluids/SPHFluidSolver.hpp"  // upgraded solver (matches old_code params)
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Component for SPH fluid simulation.
     *
     * Supports two usage modes:
     *   1. Block init  — call InitWaterBlock(nx, ny, nz) to spawn a column of water
     *                    exactly like old_code/src/sph_water.cpp
     *   2. Emitter     — set Emitting=true, configure EmissionRate for continuous flow
     *
     * Uses SPHFluidSolver (tuned h=0.045, k=1.6, visc=0.15) ported from old code.
     */
    class FluidEmitterComponent : public Component {
    public:
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

        // -----------------------------------------------------------------------
        // Solver access
        // -----------------------------------------------------------------------
        Ref<Physics::SPHFluidSolver> GetSolver() const { return m_Solver; }

        /**
         * @brief Spawn a rectangular block of water (ported from old_code/src/sph_water.cpp).
         *        Call this instead of Emitting=true if you want a water-column effect.
         * @param nx/ny/nz  Grid cell counts in each axis
         * @param spacing   Particle spacing (default ≈ SmoothingLength * 0.62)
         */
        void InitWaterBlock(int nx = 20, int ny = 15, int nz = 10, float spacing = 0.028f);

        // -----------------------------------------------------------------------
        // Emitter properties
        // -----------------------------------------------------------------------
        EmitterShape Shape            = EmitterShape::Box;
        float        EmissionRate     = 300.0f;        // particles/second
        glm::vec3    InitialVelocity  = glm::vec3(0, -1, 0);
        float        VelocityRandomness = 0.1f;
        glm::vec3    BoxSize          = glm::vec3(1, 1, 1);
        float        SphereRadius     = 0.5f;
        float        ParticleLifetime = 0.0f;          // 0 = infinite

        // Simulation domain (passed to SPHFluidSolver)
        glm::vec3 BoundsMin = glm::vec3(-2, -2, -2);
        glm::vec3 BoundsMax = glm::vec3( 2,  2,  2);

        // SPH tuning (written to solver on OnCreate / at runtime)
        float SmoothingLength      = 0.045f;
        float RestDensity          = 1000.0f;
        float PressureCoefficient  = 1.6f;
        float ViscosityCoefficient = 0.15f;
        float BounceCoefficient    = 0.4f;
        float ParticleSize         = 0.014f;  // visual render radius

        // Control flags
        bool Emitting          = false;  // off by default — use InitWaterBlock instead
        bool SimulationEnabled = true;
        int  MaxParticles      = 10000;

        // Block-init parameters — used automatically in OnCreate() when Emitting=false
        int   InitBlockNx      = 16;
        int   InitBlockNy      = 12;
        int   InitBlockNz      = 12;
        float InitBlockSpacing = 0.06f; // must be ≤ SmoothingLength*1.5 for kernels to overlap

        // -----------------------------------------------------------------------
        // Runtime queries
        // -----------------------------------------------------------------------
        void EmitBurst(int count);
        int  GetParticleCount() const;

        /** Convenience: extract particle world-positions for rendering */
        std::vector<glm::vec3> GetParticlePositions() const;

        const std::vector<Physics::FluidParticle>& GetParticles() const;
        void Clear();

        // -----------------------------------------------------------------------
        // ECS lifecycle
        // -----------------------------------------------------------------------
        void OnCreate() override;
        void OnFixedUpdate(float fixedDeltaTime) override;
        void OnDestroy() override;

    private:
        glm::vec3 GetEmitPosition() const;
        glm::vec3 GetEmitVelocity() const;
        float     RandomFloat(float min, float max) const;
        glm::vec3 RandomInUnitSphere() const;

        Ref<Physics::SPHFluidSolver> m_Solver;
        float m_EmissionAccumulator = 0.0f;
    };

}
