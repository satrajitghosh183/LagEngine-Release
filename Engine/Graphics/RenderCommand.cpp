#include "RenderCommand.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    VkCommandBuffer RenderCommand::s_ActiveCmd = VK_NULL_HANDLE;
    glm::vec4 RenderCommand::s_ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};

    void RenderCommand::Init() {
        GE_CORE_INFO("RenderCommand initialized (Vulkan backend)");
    }

    void RenderCommand::SetCommandBuffer(VkCommandBuffer cmd) {
        s_ActiveCmd = cmd;
    }

    VkCommandBuffer RenderCommand::GetCommandBuffer() {
        return s_ActiveCmd;
    }

    void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        if (s_ActiveCmd == VK_NULL_HANDLE) return;

        VkViewport viewport{};
        viewport.x = static_cast<float>(x);
        viewport.y = static_cast<float>(y);
        viewport.width = static_cast<float>(width);
        viewport.height = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(s_ActiveCmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {static_cast<int32_t>(x), static_cast<int32_t>(y)};
        scissor.extent = {width, height};
        vkCmdSetScissor(s_ActiveCmd, 0, 1, &scissor);
    }

    void RenderCommand::SetClearColor(const glm::vec4& color) {
        s_ClearColor = color;
    }

    glm::vec4 RenderCommand::GetClearColor() {
        return s_ClearColor;
    }

    void RenderCommand::DrawIndexed(VkCommandBuffer cmd, uint32_t indexCount,
                                     uint32_t instanceCount, uint32_t firstIndex) {
        vkCmdDrawIndexed(cmd, indexCount, instanceCount, firstIndex, 0, 0);
    }

    void RenderCommand::DrawArrays(VkCommandBuffer cmd, uint32_t vertexCount,
                                    uint32_t instanceCount, uint32_t firstVertex) {
        vkCmdDraw(cmd, vertexCount, instanceCount, firstVertex, 0);
    }

    void RenderCommand::DrawIndexed(uint32_t indexCount) {
        if (s_ActiveCmd != VK_NULL_HANDLE) {
            DrawIndexed(s_ActiveCmd, indexCount);
        }
    }

    void RenderCommand::DrawArrays(uint32_t vertexCount) {
        if (s_ActiveCmd != VK_NULL_HANDLE) {
            DrawArrays(s_ActiveCmd, vertexCount);
        }
    }

    void RenderCommand::PipelineBarrier(VkCommandBuffer cmd,
                                         VkPipelineStageFlags srcStage,
                                         VkPipelineStageFlags dstStage,
                                         VkAccessFlags srcAccess,
                                         VkAccessFlags dstAccess) {
        VkMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                             1, &barrier, 0, nullptr, 0, nullptr);
    }

    // These are compile-time pipeline state in Vulkan.
    // Kept as no-ops for source-level compatibility.
    void RenderCommand::SetDepthTest(bool) {}
    void RenderCommand::SetBlending(bool) {}
    void RenderCommand::SetCulling(bool) {}
    void RenderCommand::SetPolygonMode(PolygonMode) {}
    void RenderCommand::Clear() {}
}
