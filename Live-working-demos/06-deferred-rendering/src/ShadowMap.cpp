#include "ShadowMap.h"
#include "VulkanBase.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <array>

ShadowMap::~ShadowMap() {
    cleanup();
}

void ShadowMap::init(vkdemo::VulkanBase* vkBase, int width, int height) {
    m_vkBase  = vkBase;
    m_width   = width;
    m_height  = height;

    VkDevice dev = m_vkBase->GetDevice();
    uint32_t w = static_cast<uint32_t>(m_width);
    uint32_t h = static_cast<uint32_t>(m_height);

    // Create depth image
    createImage(w, h, VK_FORMAT_D32_SFLOAT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_depthImage, m_depthMemory);
    m_depthView = createImageView(m_depthImage, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);

    // Sampler with border color for shadow mapping
    VkSamplerCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sci.maxLod = 1.0f;
    vkCreateSampler(dev, &sci, nullptr, &m_sampler);

    // Render pass: depth-only
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference depthRef{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 0;
    subpass.pColorAttachments = nullptr;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dep.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = 1;
    rpci.pAttachments = &depthAttachment;
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    vkCreateRenderPass(dev, &rpci, nullptr, &m_renderPass);

    // Framebuffer
    VkFramebufferCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = m_renderPass;
    fci.attachmentCount = 1;
    fci.pAttachments = &m_depthView;
    fci.width = w;
    fci.height = h;
    fci.layers = 1;
    vkCreateFramebuffer(dev, &fci, nullptr, &m_framebuffer);
}

void ShadowMap::cleanup() {
    if (!m_vkBase) return;
    VkDevice dev = m_vkBase->GetDevice();
    if (dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(dev);

    if (m_depthView)   { vkDestroyImageView(dev, m_depthView, nullptr); m_depthView = VK_NULL_HANDLE; }
    if (m_depthImage)  { vkDestroyImage(dev, m_depthImage, nullptr); m_depthImage = VK_NULL_HANDLE; }
    if (m_depthMemory) { vkFreeMemory(dev, m_depthMemory, nullptr); m_depthMemory = VK_NULL_HANDLE; }
    if (m_sampler)     { vkDestroySampler(dev, m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
    if (m_framebuffer) { vkDestroyFramebuffer(dev, m_framebuffer, nullptr); m_framebuffer = VK_NULL_HANDLE; }
    if (m_renderPass)  { vkDestroyRenderPass(dev, m_renderPass, nullptr); m_renderPass = VK_NULL_HANDLE; }
}

uint32_t ShadowMap::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_vkBase->GetPhysicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("ShadowMap: failed to find suitable memory type");
}

void ShadowMap::createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                             VkImage& image, VkDeviceMemory& memory) {
    VkImageCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = {w, h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.format = fmt;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage = usage;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice dev = m_vkBase->GetDevice();
    vkCreateImage(dev, &ici, nullptr, &image);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(dev, image, &memReq);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    mai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(dev, &mai, nullptr, &memory);
    vkBindImageMemory(dev, image, memory, 0);
}

VkImageView ShadowMap::createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = fmt;
    ci.subresourceRange.aspectMask = aspect;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    VkImageView view;
    vkCreateImageView(m_vkBase->GetDevice(), &ci, nullptr, &view);
    return view;
}

glm::mat4 ShadowMap::getLightSpaceMatrix(const glm::vec3& lightPos, const glm::vec3& lightDir, float size) {
    glm::mat4 lightProjection = glm::ortho(-size, size, -size, size, 1.0f, 50.0f);
    glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}
