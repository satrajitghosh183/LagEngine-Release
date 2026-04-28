#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>

namespace vkdemo { class VulkanBase; }

class GBuffer {
public:
    GBuffer() = default;
    ~GBuffer();

    void init(vkdemo::VulkanBase* vkBase, int width, int height);
    void cleanup();

    VkRenderPass   getRenderPass()     const { return m_renderPass; }
    VkFramebuffer  getFramebuffer()    const { return m_framebuffer; }

    VkImageView getPositionView() const { return m_positionView; }
    VkImageView getNormalView()   const { return m_normalView; }
    VkImageView getAlbedoView()   const { return m_albedoView; }
    VkImageView getDepthView()    const { return m_depthView; }

    VkSampler getSampler() const { return m_sampler; }

    int getWidth()  const { return m_width; }
    int getHeight() const { return m_height; }

    void resize(int width, int height);

    // Legacy compat
    unsigned int getFBO() const { return 0; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_width  = 0;
    int m_height = 0;

    VkRenderPass  m_renderPass  = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkSampler     m_sampler     = VK_NULL_HANDLE;

    // Position (RGBA16F)
    VkImage        m_positionImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_positionMemory = VK_NULL_HANDLE;
    VkImageView    m_positionView   = VK_NULL_HANDLE;

    // Normal (RGBA16F)
    VkImage        m_normalImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_normalMemory = VK_NULL_HANDLE;
    VkImageView    m_normalView   = VK_NULL_HANDLE;

    // Albedo (RGBA8)
    VkImage        m_albedoImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_albedoMemory = VK_NULL_HANDLE;
    VkImageView    m_albedoView   = VK_NULL_HANDLE;

    // Depth
    VkImage        m_depthImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView    m_depthView   = VK_NULL_HANDLE;

    void create();
    void destroy();

    void createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                     VkImage& image, VkDeviceMemory& memory);
    VkImageView createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect);
    uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};
