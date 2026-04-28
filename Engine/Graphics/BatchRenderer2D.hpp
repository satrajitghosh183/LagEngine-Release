#pragma once

#include "../Core/Base.hpp"
#include "Texture2D.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <memory>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief 2D sprite batch renderer (Vulkan)
     *
     * Accumulates colored and textured quads into a host-visible dynamic vertex
     * buffer.  When the batch is full or Flush() is called, a single
     * vkCmdDrawIndexed records the entire batch into the active command buffer.
     *
     * Textures are collected into a descriptor array (up to MaxTextureSlots
     * combined image samplers bound at set 0, binding 1). A push-constant
     * carries the projection matrix so no per-frame UBO update is needed.
     *
     * Lifecycle (called once at startup / shutdown):
     *   BatchRenderer2D::Init(renderPass);
     *   BatchRenderer2D::Shutdown();
     *
     * Per-frame usage (cmd must already be inside the render pass):
     *   BatchRenderer2D::BeginBatch(cmd, projectionMatrix);
     *   BatchRenderer2D::DrawQuad(...);
     *   BatchRenderer2D::DrawTexturedQuad(...);
     *   BatchRenderer2D::EndBatch();   // calls Flush() internally
     */
    class BatchRenderer2D {
    public:
        static constexpr int MaxQuads        = 10000;
        static constexpr int MaxVertices     = MaxQuads * 4;
        static constexpr int MaxIndices      = MaxQuads * 6;
        static constexpr int MaxTextureSlots = 16;

        /** @brief Per-vertex layout (tightly packed, binding 0). */
        struct Vertex {
            glm::vec2 Position;
            glm::vec2 TexCoord;
            glm::vec4 Color;
            float     TexIndex; // index into the texture array descriptor
        };

        /** @brief Push-constant block (mat4 projection). */
        struct PushConstants {
            glm::mat4 Projection;
        };

        struct Stats {
            int DrawCalls = 0;
            int QuadCount = 0;
        };

        // ---- Lifecycle ----

        /**
         * @brief Create Vulkan resources.
         * @param renderPass Compatible render pass (the one that will be active
         *                   when BeginBatch / EndBatch are called).
         */
        static void Init(VkRenderPass renderPass);
        static void Shutdown();

        // ---- Per-frame ----

        /**
         * @brief Begin a new batch.
         * @param cmd        Active command buffer (must be inside a render pass).
         * @param projection Orthographic projection matrix (pushed as push constant).
         */
        static void BeginBatch(VkCommandBuffer cmd, const glm::mat4& projection);

        /** @brief Flush remaining quads and end the batch. */
        static void EndBatch();

        /** @brief Flush accumulated quads to the GPU immediately. */
        static void Flush();

        // ---- Draw commands ----

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size,
                             const glm::vec4& color);

        /**
         * @brief Draw a textured quad.
         * @param texture Vulkan Texture2D (may be null — uses white 1x1).
         */
        static void DrawTexturedQuad(const glm::vec2& position, const glm::vec2& size,
                                     const Ref<Texture2D>& texture,
                                     const glm::vec4& tint  = glm::vec4(1.0f),
                                     const glm::vec2& uvMin = {0.0f, 0.0f},
                                     const glm::vec2& uvMax = {1.0f, 1.0f});

        // ---- Statistics ----

        static Stats GetStats();
        static void  ResetStats();

    private:
        static void CreateVertexBuffer();
        static void CreateIndexBuffer();
        static void CreateWhiteTexture();
        static void CreatePipeline(VkRenderPass renderPass);
        static void CreateDescriptorResources();
        static void UpdateTextureDescriptors();
    };

} // namespace GameEngine
