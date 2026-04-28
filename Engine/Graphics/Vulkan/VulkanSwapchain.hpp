#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace GameEngine {

    class VulkanSwapchain {
    public:
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VulkanSwapchain() = default;
        ~VulkanSwapchain();

        void Init(VkSurfaceKHR surface, uint32_t width, uint32_t height);
        void Init(VkSurfaceKHR surface, uint32_t width, uint32_t height, VulkanSwapchain* oldSwapchain);
        void Shutdown();

        VkResult AcquireNextImage(uint32_t* imageIndex);
        VkResult SubmitCommandBuffers(const VkCommandBuffer* buffers, uint32_t* imageIndex);

        VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
        VkFormat GetImageFormat() const { return m_ImageFormat; }
        VkExtent2D GetExtent() const { return m_Extent; }
        uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
        VkImageView GetImageView(uint32_t index) const { return m_ImageViews[index]; }
        VkRenderPass GetRenderPass() const { return m_RenderPass; }
        VkFramebuffer GetFramebuffer(uint32_t index) const { return m_Framebuffers[index]; }

        float GetAspectRatio() const {
            return static_cast<float>(m_Extent.width) / static_cast<float>(m_Extent.height);
        }

        bool CompareFormats(const VulkanSwapchain& other) const {
            return m_ImageFormat == other.m_ImageFormat && m_DepthFormat == other.m_DepthFormat;
        }

        uint32_t GetCurrentFrame() const { return m_CurrentFrame; }

    private:
        void createSwapchain(VkSurfaceKHR surface, uint32_t width, uint32_t height);
        void createImageViews();
        void createDepthResources();
        void createRenderPass();
        void createFramebuffers();
        void createSyncObjects();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    private:
        VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
        VkSwapchainKHR m_OldSwapchain = VK_NULL_HANDLE;
        VkFormat m_ImageFormat{};
        VkFormat m_DepthFormat{};
        VkExtent2D m_Extent{};

        std::vector<VkImage> m_Images;
        std::vector<VkImageView> m_ImageViews;

        // Depth
        std::vector<VkImage> m_DepthImages;
        std::vector<VkDeviceMemory> m_DepthImageMemorys;
        std::vector<VkImageView> m_DepthImageViews;

        VkRenderPass m_RenderPass = VK_NULL_HANDLE;
        std::vector<VkFramebuffer> m_Framebuffers;

        // Sync objects
        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence> m_InFlightFences;
        std::vector<VkFence> m_ImagesInFlight;

        uint32_t m_CurrentFrame = 0;
    };

} // namespace GameEngine
