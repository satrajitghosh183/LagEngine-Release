#include "GBuffer.hpp"
#include "../Core/Logger.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace GameEngine {

    GBuffer::~GBuffer() {
        Cleanup();
    }

    void GBuffer::Init(int width, int height) {
        m_Width = width;
        m_Height = height;

        CreateAttachments();
        CreateRenderPass();
        CreateFramebuffer();
        CreateSampler();

        GE_CORE_INFO("GBuffer initialized ({0}x{1})", m_Width, m_Height);
    }

    void GBuffer::Resize(int width, int height) {
        if (width == m_Width && height == m_Height) return;

        m_Width = width;
        m_Height = height;

        VulkanDevice::Get().WaitIdle();
        Cleanup();
        CreateAttachments();
        CreateRenderPass();
        CreateFramebuffer();
        CreateSampler();

        GE_CORE_INFO("GBuffer resized to {0}x{1}", m_Width, m_Height);
    }

    void GBuffer::BeginRenderPass(VkCommandBuffer cmd) {
        std::array<VkClearValue, 5> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Position
        clearValues[1].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Normal
        clearValues[2].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // Albedo
        clearValues[3].color = {{0.0f, 0.0f, 0.0f, 0.0f}}; // MetallicRoughness
        clearValues[4].depthStencil = {1.0f, 0};             // Depth

        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = m_RenderPass;
        beginInfo.framebuffer = m_Framebuffer;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderArea.extent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
        beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        beginInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(m_Width);
        viewport.height = static_cast<float>(m_Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height)};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void GBuffer::EndRenderPass(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
    }

    VkDescriptorImageInfo GBuffer::GetPositionDescriptor() const {
        return {m_Sampler, m_PositionView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkDescriptorImageInfo GBuffer::GetNormalDescriptor() const {
        return {m_Sampler, m_NormalView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkDescriptorImageInfo GBuffer::GetAlbedoDescriptor() const {
        return {m_Sampler, m_AlbedoView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkDescriptorImageInfo GBuffer::GetMetallicRoughnessDescriptor() const {
        return {m_Sampler, m_MetallicRoughnessView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }

    VkDescriptorImageInfo GBuffer::GetDepthDescriptor() const {
        return {m_Sampler, m_DepthView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    }

    GBuffer::Attachment GBuffer::CreateColorAttachment(VkFormat format) {
        auto& device = VulkanDevice::Get();
        Attachment att;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(device.GetAllocator(), &imageInfo, &allocInfo,
                        &att.Image, &att.Allocation, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = att.Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &att.View);

        return att;
    }

    GBuffer::Attachment GBuffer::CreateDepthAttachment(VkFormat format) {
        auto& device = VulkanDevice::Get();
        Attachment att;

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        vmaCreateImage(device.GetAllocator(), &imageInfo, &allocInfo,
                        &att.Image, &att.Allocation, nullptr);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = att.Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};

        vkCreateImageView(device.GetDevice(), &viewInfo, nullptr, &att.View);

        return att;
    }

    void GBuffer::DestroyAttachment(Attachment& att) {
        auto device = VulkanDevice::Get().GetDevice();
        auto allocator = VulkanDevice::Get().GetAllocator();

        if (att.View != VK_NULL_HANDLE) {
            vkDestroyImageView(device, att.View, nullptr);
            att.View = VK_NULL_HANDLE;
        }
        if (att.Image != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, att.Image, att.Allocation);
            att.Image = VK_NULL_HANDLE;
            att.Allocation = VK_NULL_HANDLE;
        }
    }

    void GBuffer::CreateAttachments() {
        m_Position = CreateColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
        m_Normal = CreateColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
        m_Albedo = CreateColorAttachment(VK_FORMAT_R8G8B8A8_UNORM);
        m_MetallicRoughness = CreateColorAttachment(VK_FORMAT_R8G8_UNORM);
        m_Depth = CreateDepthAttachment(VK_FORMAT_D32_SFLOAT);

        m_PositionView = m_Position.View;
        m_NormalView = m_Normal.View;
        m_AlbedoView = m_Albedo.View;
        m_MetallicRoughnessView = m_MetallicRoughness.View;
        m_DepthView = m_Depth.View;
    }

    void GBuffer::CreateRenderPass() {
        // 4 color attachments + 1 depth
        std::array<VkAttachmentDescription, 5> attachments{};

        // Position (R16G16B16A16_SFLOAT)
        attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Normal (R16G16B16A16_SFLOAT)
        attachments[1] = attachments[0];

        // Albedo (R8G8B8A8_UNORM)
        attachments[2] = attachments[0];
        attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;

        // MetallicRoughness (R8G8_UNORM)
        attachments[3] = attachments[0];
        attachments[3].format = VK_FORMAT_R8G8_UNORM;

        // Depth (D32_SFLOAT)
        attachments[4].format = VK_FORMAT_D32_SFLOAT;
        attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        std::array<VkAttachmentReference, 4> colorRefs{};
        colorRefs[0] = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorRefs[1] = {1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorRefs[2] = {2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorRefs[3] = {3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkAttachmentReference depthRef = {4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
        subpass.pColorAttachments = colorRefs.data();
        subpass.pDepthStencilAttachment = &depthRef;

        // Dependencies for external sync
        std::array<VkSubpassDependency, 2> dependencies{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                        VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                         VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();

        vkCreateRenderPass(VulkanDevice::Get().GetDevice(), &renderPassInfo, nullptr, &m_RenderPass);
    }

    void GBuffer::CreateFramebuffer() {
        std::array<VkImageView, 5> fbAttachments = {
            m_PositionView, m_NormalView, m_AlbedoView,
            m_MetallicRoughnessView, m_DepthView
        };

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        fbInfo.pAttachments = fbAttachments.data();
        fbInfo.width = static_cast<uint32_t>(m_Width);
        fbInfo.height = static_cast<uint32_t>(m_Height);
        fbInfo.layers = 1;

        vkCreateFramebuffer(VulkanDevice::Get().GetDevice(), &fbInfo, nullptr, &m_Framebuffer);
    }

    void GBuffer::CreateSampler() {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.maxLod = 1.0f;

        vkCreateSampler(VulkanDevice::Get().GetDevice(), &samplerInfo, nullptr, &m_Sampler);
    }

    void GBuffer::Cleanup() {
        auto device = VulkanDevice::Get().GetDevice();

        if (m_Framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
            m_Framebuffer = VK_NULL_HANDLE;
        }
        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }

        DestroyAttachment(m_Position);
        DestroyAttachment(m_Normal);
        DestroyAttachment(m_Albedo);
        DestroyAttachment(m_MetallicRoughness);
        DestroyAttachment(m_Depth);

        m_PositionView = VK_NULL_HANDLE;
        m_NormalView = VK_NULL_HANDLE;
        m_AlbedoView = VK_NULL_HANDLE;
        m_MetallicRoughnessView = VK_NULL_HANDLE;
        m_DepthView = VK_NULL_HANDLE;
    }

}
