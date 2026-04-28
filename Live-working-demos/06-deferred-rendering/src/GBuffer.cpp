#include "GBuffer.h"
#include "VulkanBase.hpp"
#include <stdexcept>
#include <array>

GBuffer::~GBuffer() {
    cleanup();
}

void GBuffer::init(vkdemo::VulkanBase* vkBase, int width, int height) {
    m_vkBase  = vkBase;
    m_width   = width;
    m_height  = height;
    create();
}

void GBuffer::cleanup() {
    destroy();
}

uint32_t GBuffer::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_vkBase->GetPhysicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("GBuffer: failed to find suitable memory type");
}

void GBuffer::createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
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

VkImageView GBuffer::createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect) {
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

void GBuffer::create() {
    VkDevice dev = m_vkBase->GetDevice();
    uint32_t w = static_cast<uint32_t>(m_width);
    uint32_t h = static_cast<uint32_t>(m_height);

    // Create images
    createImage(w, h, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_positionImage, m_positionMemory);
    createImage(w, h, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_normalImage, m_normalMemory);
    createImage(w, h, VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                m_albedoImage, m_albedoMemory);
    createImage(w, h, VK_FORMAT_D32_SFLOAT,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                m_depthImage, m_depthMemory);

    // Create image views
    m_positionView = createImageView(m_positionImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    m_normalView   = createImageView(m_normalImage,   VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
    m_albedoView   = createImageView(m_albedoImage,   VK_FORMAT_R8G8B8A8_UNORM,       VK_IMAGE_ASPECT_COLOR_BIT);
    m_depthView    = createImageView(m_depthImage,    VK_FORMAT_D32_SFLOAT,            VK_IMAGE_ASPECT_DEPTH_BIT);

    // Create sampler
    VkSamplerCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_NEAREST;
    sci.minFilter = VK_FILTER_NEAREST;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.maxLod = 1.0f;
    vkCreateSampler(dev, &sci, nullptr, &m_sampler);

    // Create render pass with 3 color + 1 depth attachment
    std::array<VkAttachmentDescription, 4> attachments{};
    // Position
    attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Normal
    attachments[1] = attachments[0];
    // Albedo
    attachments[2] = attachments[0];
    attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
    // Depth
    attachments[3].format = VK_FORMAT_D32_SFLOAT;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorRefs[3] = {
        {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };
    VkAttachmentReference depthRef{3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 3;
    subpass.pColorAttachments = colorRefs;
    subpass.pDepthStencilAttachment = &depthRef;

    VkSubpassDependency dep{};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rpci{};
    rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpci.attachmentCount = static_cast<uint32_t>(attachments.size());
    rpci.pAttachments = attachments.data();
    rpci.subpassCount = 1;
    rpci.pSubpasses = &subpass;
    rpci.dependencyCount = 1;
    rpci.pDependencies = &dep;
    vkCreateRenderPass(dev, &rpci, nullptr, &m_renderPass);

    // Framebuffer
    std::array<VkImageView, 4> fbAttachments = {
        m_positionView, m_normalView, m_albedoView, m_depthView
    };
    VkFramebufferCreateInfo fci{};
    fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fci.renderPass = m_renderPass;
    fci.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
    fci.pAttachments = fbAttachments.data();
    fci.width = w;
    fci.height = h;
    fci.layers = 1;
    vkCreateFramebuffer(dev, &fci, nullptr, &m_framebuffer);
}

void GBuffer::destroy() {
    if (!m_vkBase) return;
    VkDevice dev = m_vkBase->GetDevice();
    if (dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(dev);

    auto destroyImg = [&](VkImage& img, VkDeviceMemory& mem, VkImageView& view) {
        if (view) { vkDestroyImageView(dev, view, nullptr); view = VK_NULL_HANDLE; }
        if (img)  { vkDestroyImage(dev, img, nullptr); img = VK_NULL_HANDLE; }
        if (mem)  { vkFreeMemory(dev, mem, nullptr); mem = VK_NULL_HANDLE; }
    };

    destroyImg(m_positionImage, m_positionMemory, m_positionView);
    destroyImg(m_normalImage,   m_normalMemory,   m_normalView);
    destroyImg(m_albedoImage,   m_albedoMemory,   m_albedoView);
    destroyImg(m_depthImage,    m_depthMemory,    m_depthView);

    if (m_sampler)     { vkDestroySampler(dev, m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
    if (m_framebuffer) { vkDestroyFramebuffer(dev, m_framebuffer, nullptr); m_framebuffer = VK_NULL_HANDLE; }
    if (m_renderPass)  { vkDestroyRenderPass(dev, m_renderPass, nullptr); m_renderPass = VK_NULL_HANDLE; }
}

void GBuffer::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    destroy();
    m_width = width;
    m_height = height;
    create();
}
