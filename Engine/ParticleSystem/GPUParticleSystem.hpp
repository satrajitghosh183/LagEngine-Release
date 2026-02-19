#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief GPU-driven particle system using OpenGL 4.3 compute shaders.
     *
     * Particles are stored in Shader Storage Buffer Objects (SSBOs) and
     * updated entirely on the GPU via a compute shader dispatch.  Rendering
     * uses instanced draw calls with GL_POINTS (point-sprite mode).
     */
    class GPUParticleSystem {
    public:
        /** Per-particle data layout matching the SSBO in the compute shader. */
        struct Particle {
            glm::vec4 Position; // xyz = position, w = unused
            glm::vec4 Velocity; // xyz = velocity, w = unused
            glm::vec4 Color;    // rgba
            float Life;         // remaining life in seconds (<= 0 means dead)
            float _pad[3];      // pad to 16-byte alignment
        };

        GPUParticleSystem() = default;
        ~GPUParticleSystem();

        /** Allocate GPU resources.  Must be called after an OpenGL context is current. */
        void Init(uint32_t maxParticles);

        /** Dispatch compute shader to integrate particles, emit new ones, etc. */
        void Update(float dt);

        /** Render all living particles as coloured point sprites. */
        void Render(const glm::mat4& viewProj);

        /** Release all GPU resources. */
        void Shutdown();

        // -------------------------------------------------------------------
        // Configurable parameters  (safe to change between frames)
        // -------------------------------------------------------------------
        glm::vec3 Gravity       = glm::vec3(0.0f, -9.81f, 0.0f);
        float     Restitution   = 0.5f;
        float     Damping       = 0.99f;
        float     EmitHeight    = 3.0f;
        float     FloorY        = 0.0f;
        float     ParticleSize  = 0.02f;
        int       EmitRate      = 1000;   // particles per second
        float     MaxLife       = 4.0f;   // seconds

    private:
        void CompileComputeShader();
        void CompileRenderShaders();

        uint32_t m_MaxParticles   = 0;
        bool     m_Initialized    = false;

        // GL handles
        uint32_t m_ParticleSSBO   = 0;
        uint32_t m_ComputeProgram = 0;
        uint32_t m_RenderProgram  = 0;
        uint32_t m_VAO            = 0;

        // Emit accumulator (sub-frame emission smoothing)
        float m_EmitAccumulator   = 0.0f;
    };

} // namespace GameEngine
