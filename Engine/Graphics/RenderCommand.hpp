#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Low-level rendering commands (Vulkan)
     *
     * In Vulkan, most GL-era "state" (depth test, blend, cull) is baked
     * into the VkPipeline at creation time. These helpers record common
     * commands into an active command buffer.
     */
    class RenderCommand {
    public:
        static void Init();

        /**
         * @brief Set the active command buffer for recording
         */
        static void SetCommandBuffer(VkCommandBuffer cmd);
        static VkCommandBuffer GetCommandBuffer();

        /**
         * @brief Set viewport + scissor (dynamic state)
         */
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        /**
         * @brief Store clear color for next render pass begin
         */
        static void SetClearColor(const glm::vec4& color);
        static glm::vec4 GetClearColor();

        /**
         * @brief Draw indexed geometry (records to active command buffer)
         */
        static void DrawIndexed(VkCommandBuffer cmd, uint32_t indexCount,
                                 uint32_t instanceCount = 1, uint32_t firstIndex = 0);

        /**
         * @brief Draw non-indexed geometry
         */
        static void DrawArrays(VkCommandBuffer cmd, uint32_t vertexCount,
                                uint32_t instanceCount = 1, uint32_t firstVertex = 0);

        // Convenience overloads using the active command buffer
        static void DrawIndexed(uint32_t indexCount);
        static void DrawArrays(uint32_t vertexCount);

        /**
         * @brief Pipeline memory barrier
         */
        static void PipelineBarrier(VkCommandBuffer cmd,
                                     VkPipelineStageFlags srcStage,
                                     VkPipelineStageFlags dstStage,
                                     VkAccessFlags srcAccess,
                                     VkAccessFlags dstAccess);

        /**
         * @brief These are no-ops in Vulkan — state is in the pipeline.
         * Kept for API compatibility during migration.
         */
        static void SetDepthTest(bool enabled);
        static void SetBlending(bool enabled);
        static void SetCulling(bool enabled);

        enum class PolygonMode { Fill, Line, Point };
        static void SetPolygonMode(PolygonMode mode);

        static void Clear();

    private:
        static VkCommandBuffer s_ActiveCmd;
        static glm::vec4 s_ClearColor;
    };
}
