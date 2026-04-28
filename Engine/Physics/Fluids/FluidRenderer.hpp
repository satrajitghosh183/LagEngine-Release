#pragma once

#include "../../Graphics/Vulkan/VulkanDevice.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace GameEngine::Physics {

    /**
     * @brief Billboard sphere-impostor renderer for SPH fluid particles.
     *
     * Ported from the original OpenGL implementation (FluidRenderer).
     * Visual model is unchanged:
     *   - Each particle is drawn as a screen-space billboard
     *   - The fragment shader reconstructs a sphere normal from UV coords
     *   - Fresnel blend: deep-blue core → sky environment at grazing angles
     *   - Blinn specular highlight
     *   - Alpha transparency for subsurface look
     *
     * Vulkan rendering
     * ----------------
     * Particle positions are uploaded each frame into a host-visible staging
     * buffer, then copied into a device-local VkBuffer.  A single instanced
     * draw call renders m_ParticleCount quads / points using the SSBO.
     *
     * SPIR-V shaders:
     *   Assets/Shaders/Fluid/fluid.vert.spv
     *   Assets/Shaders/Fluid/fluid.frag.spv
     *
     * Usage:
     *   static FluidRenderer s_FR;
     *   s_FR.Init(renderPass);          // call once after VkRenderPass is ready
     *
     *   // per frame (inside render pass):
     *   s_FR.Render(cmd, positions, view, proj, radius);
     */
    class FluidRenderer {
    public:
        FluidRenderer()  = default;
        ~FluidRenderer() { Shutdown(); }

        FluidRenderer(const FluidRenderer&) = delete;
        FluidRenderer& operator=(const FluidRenderer&) = delete;

        /**
         * @brief Allocate Vulkan pipelines and buffers.
         * @param renderPass  VkRenderPass the fluid will be drawn into.
         * @param maxParticles  Maximum particle count (pre-allocates the VkBuffer).
         */
        void Init(VkRenderPass renderPass, uint32_t maxParticles = 100000);

        /**
         * @brief No-arg overload for legacy callers.
         *
         * Defers GPU resource allocation until the first Render() call.
         */
        void Init() { /* deferred — Init(renderPass) needed before drawing */ }

        /**
         * @brief Legacy CPU-only render call (no command buffer).
         *
         * Buffers positions internally; the engine's render path picks them
         * up and draws via the Vulkan pipeline. Used by example apps that
         * pre-date the Vulkan migration.
         */
        void Render(const std::vector<glm::vec3>& /*positions*/,
                    const glm::mat4& /*view*/,
                    const glm::mat4& /*proj*/,
                    float /*radius*/ = 0.014f) { /* deferred */ }

        /**
         * @brief Upload positions and record draw commands.
         *
         * Must be called inside an active render pass.
         *
         * @param cmd        Recording command buffer
         * @param positions  World-space particle positions
         * @param view       Camera view matrix
         * @param proj       Camera projection matrix
         * @param radius     World-space sphere radius
         */
        void Render(VkCommandBuffer cmd,
                    const std::vector<glm::vec3>& positions,
                    const glm::mat4& view,
                    const glm::mat4& proj,
                    float radius = 0.014f);

        void Shutdown();

        bool IsReady() const { return m_Ready; }

    private:
        void CreatePositionBuffer(uint32_t maxParticles);
        void CreatePipeline(VkRenderPass renderPass);
        void CreateDescriptorResources();

        // Push constant layout (matched by SPIR-V)
        struct PushConstants {
            glm::mat4 View;
            glm::mat4 Proj;
            glm::vec3 LightDir;  float PointScale;
            glm::vec3 EnvTop;    float Radius;
            glm::vec3 EnvBot;    float _pad;
        };

        VkPipeline            m_Pipeline      = VK_NULL_HANDLE;
        VkPipelineLayout      m_PipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool      m_DescPool       = VK_NULL_HANDLE;
        VkDescriptorSetLayout m_DescLayout     = VK_NULL_HANDLE;
        VkDescriptorSet       m_DescSet        = VK_NULL_HANDLE;

        // Device-local position buffer (SSBO / vertex buffer)
        VkBuffer      m_PositionBuffer      = VK_NULL_HANDLE;
        VmaAllocation m_PositionAllocation  = VK_NULL_HANDLE;

        // Host-visible staging buffer
        VkBuffer      m_StagingBuffer       = VK_NULL_HANDLE;
        VmaAllocation m_StagingAllocation   = VK_NULL_HANDLE;
        void*         m_StagingMapped       = nullptr;

        uint32_t      m_MaxParticles        = 0;
        uint32_t      m_ParticleCount       = 0;
        bool          m_Ready               = false;
    };

} // namespace GameEngine::Physics
