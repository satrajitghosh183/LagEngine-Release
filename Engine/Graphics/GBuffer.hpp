#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Vulkan G-Buffer for deferred shading
     *
     * Stores per-pixel geometry data into multiple color attachments:
     *  - Position (R16G16B16A16_SFLOAT): world-space position
     *  - Normal (R16G16B16A16_SFLOAT): world-space normal
     *  - Albedo (R8G8B8A8_UNORM): base color + alpha
     *  - MetallicRoughness (R8G8_UNORM): metallic in R, roughness in G
     *  - Depth (D32_SFLOAT): depth buffer
     *
     * Uses a Vulkan render pass with all attachments configured for
     * optimal tiling and sampling in the lighting pass.
     */
    class GBuffer {
    public:
        GBuffer() = default;
        GBuffer(int width, int height) : m_Width(width), m_Height(height) {}
        ~GBuffer();

        void Init(int width, int height);
        void Resize(int width, int height);

        /**
         * @brief Begin G-Buffer render pass (records to command buffer)
         */
        void BeginRenderPass(VkCommandBuffer cmd);

        /**
         * @brief End G-Buffer render pass
         */
        void EndRenderPass(VkCommandBuffer cmd);

        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }

        VkImageView GetPositionView() const { return m_PositionView; }
        VkImageView GetNormalView() const { return m_NormalView; }
        VkImageView GetAlbedoView() const { return m_AlbedoView; }
        VkImageView GetMetallicRoughnessView() const { return m_MetallicRoughnessView; }
        VkImageView GetDepthView() const { return m_DepthView; }
        VkSampler GetSampler() const { return m_Sampler; }

        /**
         * @brief Get descriptor image info for each G-Buffer texture
         */
        VkDescriptorImageInfo GetPositionDescriptor() const;
        VkDescriptorImageInfo GetNormalDescriptor() const;
        VkDescriptorImageInfo GetAlbedoDescriptor() const;
        VkDescriptorImageInfo GetMetallicRoughnessDescriptor() const;
        VkDescriptorImageInfo GetDepthDescriptor() const;

        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }

    private:
        struct Attachment {
            VkImage Image = VK_NULL_HANDLE;
            VmaAllocation Allocation = VK_NULL_HANDLE;
            VkImageView View = VK_NULL_HANDLE;
        };

        void CreateAttachments();
        void CreateRenderPass();
        void CreateFramebuffer();
        void CreateSampler();
        void Cleanup();

        Attachment CreateColorAttachment(VkFormat format);
        Attachment CreateDepthAttachment(VkFormat format);
        void DestroyAttachment(Attachment& att);

        Attachment m_Position;
        Attachment m_Normal;
        Attachment m_Albedo;
        Attachment m_MetallicRoughness;
        Attachment m_Depth;

        VkImageView m_PositionView = VK_NULL_HANDLE;
        VkImageView m_NormalView = VK_NULL_HANDLE;
        VkImageView m_AlbedoView = VK_NULL_HANDLE;
        VkImageView m_MetallicRoughnessView = VK_NULL_HANDLE;
        VkImageView m_DepthView = VK_NULL_HANDLE;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;

        int m_Width = 0, m_Height = 0;
    };

}
