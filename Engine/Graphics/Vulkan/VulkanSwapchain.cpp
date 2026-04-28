#include "VulkanSwapchain.hpp"
#include "VulkanDevice.hpp"
#include "../../Core/Logger.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace GameEngine {

    VulkanSwapchain::~VulkanSwapchain() {
        Shutdown();
    }

    void VulkanSwapchain::Init(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
        Init(surface, width, height, nullptr);
    }

    void VulkanSwapchain::Init(VkSurfaceKHR surface, uint32_t width, uint32_t height, VulkanSwapchain* oldSwapchain) {
        if (oldSwapchain) {
            m_OldSwapchain = oldSwapchain->m_Swapchain;
        }

        createSwapchain(surface, width, height);
        createImageViews();
        createDepthResources();
        createRenderPass();
        createFramebuffers();
        createSyncObjects();

        GE_CORE_INFO("VulkanSwapchain created: {0}x{1}, {2} images", m_Extent.width, m_Extent.height, m_Images.size());
    }

    void VulkanSwapchain::Shutdown() {
        auto device = VulkanDevice::Get().GetDevice();
        vkDeviceWaitIdle(device);

        for (auto framebuffer : m_Framebuffers) {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
        m_Framebuffers.clear();

        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(device, m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < m_DepthImages.size(); i++) {
            vkDestroyImageView(device, m_DepthImageViews[i], nullptr);
            vkDestroyImage(device, m_DepthImages[i], nullptr);
            vkFreeMemory(device, m_DepthImageMemorys[i], nullptr);
        }
        m_DepthImages.clear();
        m_DepthImageViews.clear();
        m_DepthImageMemorys.clear();

        for (auto imageView : m_ImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        m_ImageViews.clear();

        if (m_Swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, m_InFlightFences[i], nullptr);
        }
        m_ImageAvailableSemaphores.clear();
        m_RenderFinishedSemaphores.clear();
        m_InFlightFences.clear();
    }

    void VulkanSwapchain::createSwapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height) {
        auto& vulkanDevice = VulkanDevice::Get();
        SwapchainSupportDetails swapchainSupport = vulkanDevice.QuerySwapchainSupport();

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.Formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.PresentModes);
        VkExtent2D extent = chooseSwapExtent(swapchainSupport.Capabilities, width, height);

        uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;
        if (swapchainSupport.Capabilities.maxImageCount > 0 &&
            imageCount > swapchainSupport.Capabilities.maxImageCount) {
            imageCount = swapchainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto& queueFamilies = vulkanDevice.GetQueueFamilies();
        uint32_t queueFamilyIndices[] = {
            queueFamilies.GraphicsFamily.value(),
            queueFamilies.PresentFamily.value()
        };

        if (queueFamilies.GraphicsFamily != queueFamilies.PresentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = m_OldSwapchain;

        if (vkCreateSwapchainKHR(vulkanDevice.GetDevice(), &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain!");
        }

        // Clean up old swapchain if it existed
        if (m_OldSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(vulkanDevice.GetDevice(), m_OldSwapchain, nullptr);
            m_OldSwapchain = VK_NULL_HANDLE;
        }

        vkGetSwapchainImagesKHR(vulkanDevice.GetDevice(), m_Swapchain, &imageCount, nullptr);
        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(vulkanDevice.GetDevice(), m_Swapchain, &imageCount, m_Images.data());

        m_ImageFormat = surfaceFormat.format;
        m_Extent = extent;
    }

    void VulkanSwapchain::createImageViews() {
        auto device = VulkanDevice::Get().GetDevice();
        m_ImageViews.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Images[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_ImageFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view!");
            }
        }
    }

    void VulkanSwapchain::createDepthResources() {
        auto& vulkanDevice = VulkanDevice::Get();
        auto device = vulkanDevice.GetDevice();

        m_DepthFormat = vulkanDevice.FindDepthFormat();

        m_DepthImages.resize(m_Images.size());
        m_DepthImageMemorys.resize(m_Images.size());
        m_DepthImageViews.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++) {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = m_Extent.width;
            imageInfo.extent.height = m_Extent.height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = m_DepthFormat;
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(device, &imageInfo, nullptr, &m_DepthImages[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, m_DepthImages[i], &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = vulkanDevice.FindMemoryType(
                memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &m_DepthImageMemorys[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to allocate depth image memory!");
            }

            vkBindImageMemory(device, m_DepthImages[i], m_DepthImageMemorys[i], 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_DepthImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = m_DepthFormat;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &viewInfo, nullptr, &m_DepthImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create depth image view!");
            }
        }
    }

    void VulkanSwapchain::createRenderPass() {
        auto device = VulkanDevice::Get().GetDevice();

        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_ImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = m_DepthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass!");
        }
    }

    void VulkanSwapchain::createFramebuffers() {
        auto device = VulkanDevice::Get().GetDevice();
        m_Framebuffers.resize(m_Images.size());

        for (size_t i = 0; i < m_Images.size(); i++) {
            std::array<VkImageView, 2> attachments = {m_ImageViews[i], m_DepthImageViews[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_RenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_Extent.width;
            framebufferInfo.height = m_Extent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void VulkanSwapchain::createSyncObjects() {
        auto device = VulkanDevice::Get().GetDevice();

        m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        m_ImagesInFlight.resize(m_Images.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
    }

    VkResult VulkanSwapchain::AcquireNextImage(uint32_t* imageIndex) {
        auto device = VulkanDevice::Get().GetDevice();

        vkWaitForFences(device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE,
                        std::numeric_limits<uint64_t>::max());

        return vkAcquireNextImageKHR(device, m_Swapchain, std::numeric_limits<uint64_t>::max(),
                                      m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, imageIndex);
    }

    VkResult VulkanSwapchain::SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex) {
        auto& vulkanDevice = VulkanDevice::Get();
        auto device = vulkanDevice.GetDevice();

        if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &m_ImagesInFlight[*imageIndex], VK_TRUE,
                            std::numeric_limits<uint64_t>::max());
        }
        m_ImagesInFlight[*imageIndex] = m_InFlightFences[m_CurrentFrame];

        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = buffers;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(device, 1, &m_InFlightFences[m_CurrentFrame]);
        if (vkQueueSubmit(vulkanDevice.GetGraphicsQueue(), 1, &submitInfo,
                          m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_Swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = imageIndex;

        auto result = vkQueuePresentKHR(vulkanDevice.GetPresentQueue(), &presentInfo);

        m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

        return result;
    }

    VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                GE_CORE_INFO("Present mode: Mailbox (triple buffering)");
                return availablePresentMode;
            }
        }
        GE_CORE_INFO("Present mode: FIFO (vsync)");
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::chooseSwapExtent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        uint32_t width, uint32_t height) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }

        VkExtent2D actualExtent = {width, height};
        actualExtent.width = std::clamp(actualExtent.width,
                                        capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height,
                                         capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);
        return actualExtent;
    }

} // namespace GameEngine
