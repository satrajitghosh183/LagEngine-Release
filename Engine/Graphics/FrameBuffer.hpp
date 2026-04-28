#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Framebuffer attachment specification
     */
    enum class FramebufferAttachmentFormat {
        None = 0,
        RGBA8,           // VK_FORMAT_R8G8B8A8_UNORM
        RGBA16F,         // VK_FORMAT_R16G16B16A16_SFLOAT
        RG16F,           // VK_FORMAT_R16G16_SFLOAT
        R8,              // VK_FORMAT_R8_UNORM
        Depth32F,        // VK_FORMAT_D32_SFLOAT
        Depth24Stencil8  // VK_FORMAT_D24_UNORM_S8_UINT
    };

    struct FramebufferAttachmentSpec {
        FramebufferAttachmentFormat Format = FramebufferAttachmentFormat::None;
    };

    /**
     * @brief Framebuffer specification
     */
    struct FramebufferSpec {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        uint32_t Samples = 1;
        bool SwapChainTarget = false;

        std::vector<FramebufferAttachmentSpec> ColorAttachments;
        FramebufferAttachmentSpec DepthAttachment;
    };

    /**
     * @brief Vulkan Framebuffer (VkRenderPass + VkFramebuffer + attachments)
     *
     * Creates a complete render pass with color and depth attachments.
     * Supports multiple color attachments for deferred rendering (G-Buffer).
     */
    class Framebuffer {
    public:
        Framebuffer(const FramebufferSpec& spec);
        ~Framebuffer();

        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        /**
         * @brief Begin render pass (records to command buffer)
         */
        void BeginRenderPass(VkCommandBuffer cmd,
                              const std::vector<VkClearValue>& clearValues = {}) const;

        /**
         * @brief End render pass
         */
        void EndRenderPass(VkCommandBuffer cmd) const;

        /**
         * @brief Resize framebuffer (destroys and recreates all resources)
         */
        void Resize(uint32_t width, uint32_t height);

        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetFramebuffer() const { return m_Framebuffer; }
        VkExtent2D GetExtent() const { return {m_Spec.Width, m_Spec.Height}; }

        /**
         * @brief Get color attachment image view (for sampling in subsequent passes)
         */
        VkImageView GetColorAttachmentView(uint32_t index = 0) const;

        /**
         * @brief Get depth attachment image view
         */
        VkImageView GetDepthAttachmentView() const { return m_DepthImageView; }

        /**
         * @brief Get descriptor image info for a color attachment
         */
        VkDescriptorImageInfo GetColorDescriptorInfo(uint32_t index = 0, VkSampler sampler = VK_NULL_HANDLE) const;

        const FramebufferSpec& GetSpecification() const { return m_Spec; }
        uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(m_ColorImageViews.size()); }

    private:
        void Invalidate();
        void Release();

        static VkFormat AttachmentFormatToVk(FramebufferAttachmentFormat format);
        static bool IsDepthFormat(FramebufferAttachmentFormat format);

    private:
        FramebufferSpec m_Spec;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

        // Color attachments
        std::vector<VkImage> m_ColorImages;
        std::vector<VmaAllocation> m_ColorAllocations;
        std::vector<VkImageView> m_ColorImageViews;

        // Depth attachment
        VkImage m_DepthImage = VK_NULL_HANDLE;
        VmaAllocation m_DepthAllocation = VK_NULL_HANDLE;
        VkImageView m_DepthImageView = VK_NULL_HANDLE;
    };
}
