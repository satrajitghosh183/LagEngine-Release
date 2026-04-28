#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>
#include <glm/glm.hpp>

namespace vkdemo { class VulkanBase; }

class ShadowMap {
public:
    ShadowMap() = default;
    ~ShadowMap();

    void init(vkdemo::VulkanBase* vkBase, int width = 2048, int height = 2048);
    void cleanup();

    VkRenderPass  getRenderPass()  const { return m_renderPass; }
    VkFramebuffer getFramebuffer() const { return m_framebuffer; }
    VkImageView   getDepthView()   const { return m_depthView; }
    VkSampler     getSampler()     const { return m_sampler; }

    glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, float size = 20.0f);

    int getWidth()  const { return m_width; }
    int getHeight() const { return m_height; }

    // Legacy compat
    unsigned int getFBO() const { return 0; }

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_width  = 2048;
    int m_height = 2048;

    VkRenderPass  m_renderPass  = VK_NULL_HANDLE;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    VkSampler     m_sampler     = VK_NULL_HANDLE;

    VkImage        m_depthImage  = VK_NULL_HANDLE;
    VkDeviceMemory m_depthMemory = VK_NULL_HANDLE;
    VkImageView    m_depthView   = VK_NULL_HANDLE;

    void createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                     VkImage& image, VkDeviceMemory& memory);
    VkImageView createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect);
    uint32_t findMemoryType(uint32_t filter, VkMemoryPropertyFlags props);
};
