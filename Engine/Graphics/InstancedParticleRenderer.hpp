#pragma once

#include "../Core/Base.hpp"
#include "Texture2D.hpp"
#include "Camera3D.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Instance data for a single particle (per-instance vertex attributes).
     *        Must match the layout declared in the particle vertex shader.
     */
    struct ParticleInstance {
        glm::vec3 Position;       // location = 2
        float     Size;           // location = 3
        glm::vec4 Color;          // location = 4
        float     Rotation;       // location = 5  (Z-axis for billboards)
        float     LifeRemaining;  // location = 6  (0-1 for fade effects)
    };

    /**
     * @brief GPU-instanced particle renderer (Vulkan)
     *
     * Renders thousands of billboarded quads using hardware instancing via
     * vkCmdDrawInstanced. Each call to DrawParticles() uploads the instance
     * data to a host-visible device buffer and records one draw call into the
     * active command buffer.
     *
     * Features:
     *   - Hardware instancing (one draw call per particle batch)
     *   - Automatic billboarding (camera right/up extracted from view matrix)
     *   - Texture atlas support (configurable grid)
     *   - Blend mode selection per DrawParticles call
     *   - Soft-particle depth fade (requires scene depth image view)
     *
     * Init must be called with a compatible VkRenderPass. The render pass must
     * have alpha blending enabled; use EnableAlphaBlending() or set it in your
     * own PipelineConfigInfo before calling Init.
     *
     * Usage:
     *   InstancedParticleRenderer::Init(renderPass);
     *   // per-frame, inside a command buffer / render pass:
     *   InstancedParticleRenderer::Begin(camera);
     *   InstancedParticleRenderer::DrawParticles(cmd, particles, texture, BlendMode::Alpha);
     *   InstancedParticleRenderer::End();
     */
    class InstancedParticleRenderer {
    public:
        enum class BlendMode {
            Additive,   // fire, glow
            Alpha,      // standard transparency
            Multiply    // shadows, smoke
        };

        struct Statistics {
            uint32_t DrawCalls       = 0;
            uint32_t ParticlesRendered = 0;
            uint32_t BatchCount      = 0;
        };

        // ---- Lifecycle ----

        /**
         * @brief Allocate GPU resources.
         * @param renderPass  Compatible render pass (active during Draw calls).
         * @param maxParticles Maximum instances per DrawParticles call.
         */
        static void Init(VkRenderPass renderPass, uint32_t maxParticles = 10000);
        static void Shutdown();

        // ---- Per-frame ----

        /**
         * @brief Cache camera vectors (call once per frame before DrawParticles).
         */
        static void Begin(const Camera3D& camera);

        /** @brief No-op; provided for symmetry. */
        static void End();

        /**
         * @brief Record an instanced draw for the given particles.
         * @param cmd      Active command buffer (must be inside the render pass).
         * @param particles Vector of particle instance data.
         * @param texture  Particle texture (nullptr = opaque white quad).
         * @param blendMode Additive, Alpha, or Multiply.
         */
        static void DrawParticles(
            VkCommandBuffer cmd,
            const std::vector<ParticleInstance>& particles,
            const Ref<Texture2D>& texture = nullptr,
            BlendMode blendMode = BlendMode::Alpha);

        // ---- Config ----

        static void SetAtlasSize(int columns, int rows);
        static void SetSoftParticles(bool enabled, float fadeDistance = 0.5f);

        // ---- Stats ----

        static const Statistics& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats = {}; }

        // ---- Push-constants block (must match shader) ----
        struct PushConstants {
            glm::mat4 ViewProjection;
            glm::vec4 CameraRight;   // w unused
            glm::vec4 CameraUp;      // w unused
            glm::ivec2 AtlasSize;
            float SoftFadeDistance;
            int   SoftParticles;
        };

    private:
        static void CreateQuadBuffers();
        static void CreateInstanceBuffer(uint32_t maxParticles);
        static void CreateDefaultTexture();
        static void CreateDescriptorResources();
        static void CreatePipeline(VkRenderPass renderPass, BlendMode blendMode);

        // Per-pipeline cache (one pipeline per blend mode)
        struct PipelineEntry {
            VulkanPipeline Pipeline;
            BlendMode      Mode;
        };
        static std::vector<PipelineEntry> s_Pipelines;
        static VulkanPipeline* GetOrCreatePipeline(VkRenderPass renderPass, BlendMode mode);

        // ---- Quad geometry (static) ----
        static VkBuffer      s_QuadVB;
        static VmaAllocation s_QuadVBAlloc;

        // ---- Instance buffer (dynamic, host-visible) ----
        static VkBuffer      s_InstanceVB;
        static VmaAllocation s_InstanceVBAlloc;
        static void*         s_InstanceMapped;
        static uint32_t      s_MaxParticles;

        // ---- Textures ----
        static Ref<Texture2D> s_DefaultTexture;

        // ---- Descriptor resources ----
        static std::unique_ptr<VulkanDescriptorSetLayout> s_DescLayout;
        static std::unique_ptr<VulkanDescriptorPool>      s_DescPool;
        static VkSampler s_Sampler;

        // ---- Pipeline layout ----
        static VkPipelineLayout s_PipelineLayout;

        // ---- Camera state ----
        static glm::mat4 s_ViewProjection;
        static glm::vec3 s_CameraRight;
        static glm::vec3 s_CameraUp;

        // ---- Config ----
        static int   s_AtlasColumns;
        static int   s_AtlasRows;
        static bool  s_SoftParticles;
        static float s_SoftFadeDistance;

        // ---- Active render pass (for late pipeline creation) ----
        static VkRenderPass s_RenderPass;

        static Statistics s_Stats;
    };

} // namespace GameEngine
