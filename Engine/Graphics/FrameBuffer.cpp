#include "FrameBuffer.hpp"
#include "../Core/Logger.hpp"
#include "Vulkan/VulkanDevice.hpp"

namespace GameEngine {

    Framebuffer::Framebuffer(const FramebufferSpec& spec)
        : m_Spec(spec) {
        // Default: 1 RGBA8 color + Depth32F if no attachments specified
        if (m_Spec.ColorAttachments.empty()) {
            m_Spec.ColorAttachments.push_back({FramebufferAttachmentFormat::RGBA8});
        }
        if (m_Spec.DepthAttachment.Format == FramebufferAttachmentFormat::None) {
            m_Spec.DepthAttachment.Format = FramebufferAttachmentFormat::Depth32F;
        }
        Invalidate();
    }

    Framebuffer::~Framebuffer() {
        Release();
    }

    void Framebuffer::Invalidate() {
        if (m_Framebuffer != VK_NULL_HANDLE) {
            Release();
        }

        auto device = VulkanDevice::Get().GetDevice();
        auto allocator = VulkanDevice::Get().GetAllocator();

        // --- Create color attachment images ---
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorRefs;
        uint32_t attachmentIndex = 0;

        for (const auto& colorSpec : m_Spec.ColorAttachments) {
            VkFormat format = AttachmentFormatToVk(colorSpec.Format);

            // Create image
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = {m_Spec.Width, m_Spec.Height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            VkImage image;
            VmaAllocation alloc;
            vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, &alloc, nullptr);
            m_ColorImages.push_back(image);
            m_ColorAllocations.push_back(alloc);

            // Create image view
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView view;
            vkCreateImageView(device, &viewInfo, nullptr, &view);
            m_ColorImageViews.push_back(view);

            // Attachment description
            VkAttachmentDescription desc{};
            desc.format = format;
            desc.samples = VK_SAMPLE_COUNT_1_BIT;
            desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachments.push_back(desc);

            VkAttachmentReference ref{};
            ref.attachment = attachmentIndex++;
            ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs.push_back(ref);
        }

        // --- Create depth attachment ---
        VkAttachmentReference depthRef{};
        bool hasDepth = !IsDepthFormat(FramebufferAttachmentFormat::None) &&
                        m_Spec.DepthAttachment.Format != FramebufferAttachmentFormat::None;

        if (hasDepth) {
            VkFormat depthFormat = AttachmentFormatToVk(m_Spec.DepthAttachment.Format);

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent = {m_Spec.Width, m_Spec.Height, 1};
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = depthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            VmaAllocationCreateInfo allocInfo{};
            allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

            vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_DepthImage, &m_DepthAllocation, nullptr);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_DepthImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = depthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (m_Spec.DepthAttachment.Format == FramebufferAttachmentFormat::Depth24Stencil8) {
                viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            vkCreateImageView(device, &viewInfo, nullptr, &m_DepthImageView);

            VkAttachmentDescription depthDesc{};
            depthDesc.format = depthFormat;
            depthDesc.samples = VK_SAMPLE_COUNT_1_BIT;
            depthDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            attachments.push_back(depthDesc);

            depthRef.attachment = attachmentIndex;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        // --- Create render pass ---
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
        subpass.pColorAttachments = colorRefs.data();
        if (hasDepth) {
            subpass.pDepthStencilAttachment = &depthRef;
        }

        // Subpass dependencies for layout transitions
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
                                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
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

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create framebuffer render pass");
            return;
        }

        // --- Create framebuffer ---
        std::vector<VkImageView> fbAttachments;
        for (auto& view : m_ColorImageViews) {
            fbAttachments.push_back(view);
        }
        if (hasDepth) {
            fbAttachments.push_back(m_DepthImageView);
        }

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_RenderPass;
        fbInfo.attachmentCount = static_cast<uint32_t>(fbAttachments.size());
        fbInfo.pAttachments = fbAttachments.data();
        fbInfo.width = m_Spec.Width;
        fbInfo.height = m_Spec.Height;
        fbInfo.layers = 1;

        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &m_Framebuffer) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create framebuffer");
            return;
        }

        GE_CORE_INFO("Framebuffer created: {0}x{1} ({2} color + {3} depth)",
                      m_Spec.Width, m_Spec.Height, m_ColorImages.size(), hasDepth ? 1 : 0);
    }

    void Framebuffer::Release() {
        auto device = VulkanDevice::Get().GetDevice();
        auto allocator = VulkanDevice::Get().GetAllocator();

        if (m_Framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, m_Framebuffer, nullptr);
            m_Framebuffer = VK_NULL_HANDLE;
        }

        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

        for (auto& view : m_ColorImageViews) {
            vkDestroyImageView(device, view, nullptr);
        }
        for (size_t i = 0; i < m_ColorImages.size(); i++) {
            vmaDestroyImage(allocator, m_ColorImages[i], m_ColorAllocations[i]);
        }
        m_ColorImageViews.clear();
        m_ColorImages.clear();
        m_ColorAllocations.clear();

        if (m_DepthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, m_DepthImageView, nullptr);
            m_DepthImageView = VK_NULL_HANDLE;
        }
        if (m_DepthImage != VK_NULL_HANDLE) {
            vmaDestroyImage(allocator, m_DepthImage, m_DepthAllocation);
            m_DepthImage = VK_NULL_HANDLE;
            m_DepthAllocation = VK_NULL_HANDLE;
        }
    }

    void Framebuffer::BeginRenderPass(VkCommandBuffer cmd,
                                       const std::vector<VkClearValue>& clearValues) const {
        std::vector<VkClearValue> clears = clearValues;
        if (clears.empty()) {
            // Default clear values
            for (size_t i = 0; i < m_ColorImageViews.size(); i++) {
                VkClearValue clear{};
                clear.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
                clears.push_back(clear);
            }
            if (m_DepthImageView != VK_NULL_HANDLE) {
                VkClearValue depthClear{};
                depthClear.depthStencil = {1.0f, 0};
                clears.push_back(depthClear);
            }
        }

        VkRenderPassBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = m_RenderPass;
        beginInfo.framebuffer = m_Framebuffer;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderArea.extent = {m_Spec.Width, m_Spec.Height};
        beginInfo.clearValueCount = static_cast<uint32_t>(clears.size());
        beginInfo.pClearValues = clears.data();

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // Set viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_Spec.Width);
        viewport.height = static_cast<float>(m_Spec.Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {m_Spec.Width, m_Spec.Height};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void Framebuffer::EndRenderPass(VkCommandBuffer cmd) const {
        vkCmdEndRenderPass(cmd);
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0 || width > 8192 || height > 8192) {
            GE_CORE_WARN("Invalid framebuffer size: {0}x{1}", width, height);
            return;
        }

        m_Spec.Width = width;
        m_Spec.Height = height;

        VulkanDevice::Get().WaitIdle();
        Invalidate();
    }

    VkImageView Framebuffer::GetColorAttachmentView(uint32_t index) const {
        GE_CORE_ASSERT(index < m_ColorImageViews.size(), "Color attachment index out of range");
        return m_ColorImageViews[index];
    }

    VkDescriptorImageInfo Framebuffer::GetColorDescriptorInfo(uint32_t index, VkSampler sampler) const {
        VkDescriptorImageInfo info{};
        info.sampler = sampler;
        info.imageView = GetColorAttachmentView(index);
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    VkFormat Framebuffer::AttachmentFormatToVk(FramebufferAttachmentFormat format) {
        switch (format) {
            case FramebufferAttachmentFormat::RGBA8:           return VK_FORMAT_R8G8B8A8_UNORM;
            case FramebufferAttachmentFormat::RGBA16F:         return VK_FORMAT_R16G16B16A16_SFLOAT;
            case FramebufferAttachmentFormat::RG16F:           return VK_FORMAT_R16G16_SFLOAT;
            case FramebufferAttachmentFormat::R8:              return VK_FORMAT_R8_UNORM;
            case FramebufferAttachmentFormat::Depth32F:        return VK_FORMAT_D32_SFLOAT;
            case FramebufferAttachmentFormat::Depth24Stencil8: return VK_FORMAT_D24_UNORM_S8_UINT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    bool Framebuffer::IsDepthFormat(FramebufferAttachmentFormat format) {
        return format == FramebufferAttachmentFormat::Depth32F ||
               format == FramebufferAttachmentFormat::Depth24Stencil8;
    }
}
