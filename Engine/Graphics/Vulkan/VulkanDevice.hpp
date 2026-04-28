#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <optional>

namespace GameEngine {

    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> PresentFamily;
        std::optional<uint32_t> ComputeFamily;
        std::optional<uint32_t> TransferFamily;

        bool isComplete() const {
            return GraphicsFamily.has_value() && PresentFamily.has_value();
        }
    };

    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities{};
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanDevice {
    public:
        VulkanDevice() = default;
        ~VulkanDevice();

        void Init(VkInstance instance, VkSurfaceKHR surface);
        void Shutdown();

        VkDevice GetDevice() const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VmaAllocator GetAllocator() const { return m_Allocator; }

        VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue GetPresentQueue() const { return m_PresentQueue; }
        VkQueue GetComputeQueue() const { return m_ComputeQueue; }

        const QueueFamilyIndices& GetQueueFamilies() const { return m_QueueFamilies; }
        SwapchainSupportDetails QuerySwapchainSupport() const;
        SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

        VkPhysicalDeviceProperties GetProperties() const;
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) const;
        VkFormat FindDepthFormat() const;

        VkCommandBuffer BeginSingleTimeCommands() const;
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

        void WaitIdle() const;

        static VulkanDevice& Get();

    private:
        void pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
        void createLogicalDevice();
        void createAllocator(VkInstance instance);

        bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        int rateDevice(VkPhysicalDevice device) const;
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const;
        bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    private:
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device = VK_NULL_HANDLE;
        VmaAllocator m_Allocator = VK_NULL_HANDLE;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue m_PresentQueue = VK_NULL_HANDLE;
        VkQueue m_ComputeQueue = VK_NULL_HANDLE;

        QueueFamilyIndices m_QueueFamilies;
        VkCommandPool m_SingleTimeCommandPool = VK_NULL_HANDLE;

        static VulkanDevice* s_Instance;
    };

} // namespace GameEngine
