#pragma once

#include "../Core/Base.hpp"
#include "../Graphics/Vulkan/VulkanDevice.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace GameEngine {

    /**
     * @brief GPU-driven particle system using Vulkan compute shaders.
     *
     * Particles are stored in a VkBuffer (VMA-allocated, device-local) and
     * updated entirely on the GPU via a compute shader dispatch each frame.
     * Rendering uses an instanced draw call (one instance per particle).
     *
     * Pipeline
     * --------
     *  Update (compute pass):
     *    vkCmdBindPipeline(COMPUTE) → vkCmdDispatch
     *    Pipeline barrier: SHADER_WRITE → VERTEX_ATTRIBUTE_READ
     *
     *  Render (graphics pass):
     *    vkCmdBindPipeline(GRAPHICS, point-sprite pipeline)
     *    vkCmdDraw(m_MaxParticles) — particle data read from SSBO via descriptor set
     *
     * The SPIR-V shaders are loaded from:
     *   Assets/Shaders/Particles/particle_update.comp.spv
     *   Assets/Shaders/Particles/particle.vert.spv
     *   Assets/Shaders/Particles/particle.frag.spv
     *
     * CPU fallback (m_UseCompute == false)
     * ----------------------------------------
     * If the compute queue is unavailable, particle integration runs on the CPU
     * and the results are uploaded each frame via a host-visible staging buffer.
     */
    class GPUParticleSystem {
    public:
        /** Per-particle data layout matching the SSBO in the compute shader. */
        struct Particle {
            glm::vec4 Position;    // xyz = position, w = color state
            glm::vec4 Velocity;    // xyz = velocity, w = unused
            glm::vec4 Color;       // rgba
            float     Life;        // remaining life (<= 0 = dead)
            float     _pad[3];     // pad to 16-byte alignment
        };

        GPUParticleSystem() = default;
        ~GPUParticleSystem();

        GPUParticleSystem(const GPUParticleSystem&) = delete;
        GPUParticleSystem& operator=(const GPUParticleSystem&) = delete;

        /**
         * @brief Allocate Vulkan resources.
         * @param maxParticles  Capacity of the particle pool
         * @param renderPass    VkRenderPass the particles will be drawn into
         */
        void Init(uint32_t maxParticles, VkRenderPass renderPass);

        /**
         * @brief Dispatch compute shader to integrate particles.
         * @param cmd  Command buffer recording into the compute queue (or graphics queue).
         * @param dt   Delta time in seconds.
         */
        void Update(VkCommandBuffer cmd, float dt);

        /**
         * @brief Record particle draw commands.
         * @param cmd       Active command buffer (inside render pass)
         * @param viewProj  Camera view-projection matrix (pushed as push constant)
         */
        void Render(VkCommandBuffer cmd, const glm::mat4& viewProj);

        /** Release all Vulkan resources. */
        void Shutdown();

        // -------------------------------------------------------------------
        // Configurable parameters (may be changed between frames)
        // -------------------------------------------------------------------
        glm::vec3 Gravity      = glm::vec3(0.0f, -9.81f, 0.0f);
        float     Restitution  = 0.5f;
        float     Damping      = 0.99f;
        float     EmitHeight   = 3.0f;
        float     FloorY       = 0.0f;
        float     ParticleSize = 0.02f;
        int       EmitRate     = 1000;   // particles per second
        float     MaxLife      = 4.0f;   // seconds

    private:
        void CreateParticleBuffer();
        void CreateComputePipeline();
        void CreateRenderPipeline(VkRenderPass renderPass);
        void CreateDescriptorResources();
        void UpdateCPU(float dt);
        void UploadCPUParticles(VkCommandBuffer cmd);

        // Compute push constants (matched by the SPIR-V)
        struct ComputePushConstants {
            float DeltaTime;
            glm::vec3 Gravity;
            float Restitution;
            float Damping;
            float FloorY;
            float EmitHeight;
            float MaxLife;
            int   EmitCount;
            uint32_t MaxParticles;
            uint32_t FrameSeed;
            float _pad;
        };

        // Render push constants
        struct RenderPushConstants {
            glm::mat4 ViewProj;
            float     PointSize;
            float     _pad[3];
        };

        uint32_t m_MaxParticles     = 0;
        bool     m_Initialized      = false;
        bool     m_UseCompute       = false; // true if VK_KHR_compute is available

        // Particle SSBO (device-local)
        VkBuffer      m_ParticleBuffer     = VK_NULL_HANDLE;
        VmaAllocation m_ParticleAllocation = VK_NULL_HANDLE;

        // Staging buffer for CPU fallback uploads
        VkBuffer      m_StagingBuffer      = VK_NULL_HANDLE;
        VmaAllocation m_StagingAllocation  = VK_NULL_HANDLE;
        void*         m_StagingMapped      = nullptr;

        // Compute pipeline
        VkPipeline       m_ComputePipeline  = VK_NULL_HANDLE;
        VkPipelineLayout m_ComputeLayout    = VK_NULL_HANDLE;

        // Graphics pipeline
        VkPipeline       m_RenderPipeline   = VK_NULL_HANDLE;
        VkPipelineLayout m_RenderLayout     = VK_NULL_HANDLE;

        // Descriptors
        VkDescriptorPool      m_DescPool         = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescSetLayout     = VK_NULL_HANDLE;
        VkDescriptorSet       m_DescSet           = VK_NULL_HANDLE;

        // CPU fallback particle data
        std::vector<Particle> m_ParticlesCPU;
        float                 m_EmitAccumulator = 0.0f;

        static uint32_t       s_FrameCounter;
    };

} // namespace GameEngine
