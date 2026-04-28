#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>

namespace vkdemo { class VulkanBase; }

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    void init(vkdemo::VulkanBase* vkBase, int width, int height);
    void cleanup();

    VkRenderPass  getRenderPass()  const { return m_renderPass; }
    VkFramebuffer getFramebuffer() const { return m_framebuffer; }

    VkImageView getColorView() const { return m_colorView; }
    VkImageView getDepthView() const { return m_depthView; }
    VkSampler   getSampler()   const { return m_sampler; }

    int getWidth()  const { return m_width; }
    int getHeight() const { return m_height; }

    void resize(int width, int height);

    // Legacy compat
    unsigned int getColorTexture() const { return 0; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_width  = 0;
    int m_height = 0;

    VkRenderPass  m_renderPass  = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkSampler     m_sampler     = VK_NULL_HANDLE;

    VkImage        m_colorImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_colorMemory = VK_NULL_HANDLE;
    VkImageView    m_colorView   = VK_NULL_HANDLE;

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
