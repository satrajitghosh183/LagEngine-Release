#include "VulkanDevice.hpp"
#include "VulkanContext.hpp"
#include "../../Core/Logger.hpp"

#include <set>
#include <stdexcept>
#include <string>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace GameEngine {

    VulkanDevice* VulkanDevice::s_Instance = nullptr;

    static const std::vector<const char*> s_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VulkanDevice::~VulkanDevice() {
        Shutdown();
    }

    VulkanDevice& VulkanDevice::Get() {
        if (!s_Instance) {
            throw std::runtime_error("VulkanDevice not initialized!");
        }
        return *s_Instance;
    }

    void VulkanDevice::Init(VkInstance instance, VkSurfaceKHR surface) {
        s_Instance = this;
        m_Surface = surface;

        pickPhysicalDevice(instance, surface);
        createLogicalDevice();
        createAllocator(instance);

        // Create a command pool for single-time commands
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = m_QueueFamilies.GraphicsFamily.value();

        if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_SingleTimeCommandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create single-time command pool!");
        }

        GE_CORE_INFO("VulkanDevice initialized successfully");
    }

    void VulkanDevice::Shutdown() {
        if (m_SingleTimeCommandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_Device, m_SingleTimeCommandPool, nullptr);
            m_SingleTimeCommandPool = VK_NULL_HANDLE;
        }

        if (m_Allocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_Allocator);
            m_Allocator = VK_NULL_HANDLE;
        }

        if (m_Device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        s_Instance = nullptr;
    }

    void VulkanDevice::pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        GE_CORE_INFO("Found {0} Vulkan-capable GPU(s):", deviceCount);

        int bestScore = -1;
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            GE_CORE_INFO("  - {0} (type: {1})", props.deviceName, static_cast<int>(props.deviceType));

            if (isDeviceSuitable(device, surface)) {
                int score = rateDevice(device);
                if (score > bestScore) {
                    m_PhysicalDevice = device;
                    bestScore = score;
                }
            }
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }

        m_QueueFamilies = findQueueFamilies(m_PhysicalDevice, surface);

        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
        GE_CORE_INFO("Selected GPU: {0}", props.deviceName);
        GE_CORE_INFO("  API Version: {0}.{1}.{2}",
            VK_VERSION_MAJOR(props.apiVersion),
            VK_VERSION_MINOR(props.apiVersion),
            VK_VERSION_PATCH(props.apiVersion));
        GE_CORE_INFO("  Driver Version: {0}", props.driverVersion);
    }

    void VulkanDevice::createLogicalDevice() {
        std::set<uint32_t> uniqueQueueFamilies = {
            m_QueueFamilies.GraphicsFamily.value(),
            m_QueueFamilies.PresentFamily.value()
        };

        if (m_QueueFamilies.ComputeFamily.has_value()) {
            uniqueQueueFamilies.insert(m_QueueFamilies.ComputeFamily.value());
        }

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = VK_TRUE;
        deviceFeatures.fillModeNonSolid = VK_TRUE;
        deviceFeatures.wideLines = VK_TRUE;
        deviceFeatures.multiDrawIndirect = VK_TRUE;
        deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (VulkanContext::Get().ValidationEnabled()) {
            static const std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }

        vkGetDeviceQueue(m_Device, m_QueueFamilies.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilies.PresentFamily.value(), 0, &m_PresentQueue);

        if (m_QueueFamilies.ComputeFamily.has_value()) {
            vkGetDeviceQueue(m_Device, m_QueueFamilies.ComputeFamily.value(), 0, &m_ComputeQueue);
        }
    }

    void VulkanDevice::createAllocator(VkInstance instance) {
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.physicalDevice = m_PhysicalDevice;
        allocatorInfo.device = m_Device;
        allocatorInfo.instance = instance;
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;

        if (vmaCreateAllocator(&allocatorInfo, &m_Allocator) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create VMA allocator!");
        }
    }

    bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) const {
        QueueFamilyIndices indices = findQueueFamilies(device, surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapchainAdequate = false;
        if (extensionsSupported) {
            auto swapchainSupport = QuerySwapchainSupport(device);
            swapchainAdequate = !swapchainSupport.Formats.empty() && !swapchainSupport.PresentModes.empty();
        }

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

        return indices.isComplete() && extensionsSupported && swapchainAdequate &&
               supportedFeatures.samplerAnisotropy;
    }

    int VulkanDevice::rateDevice(VkPhysicalDevice device) const {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(device, &props);

        int score = 0;
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            score += 100;
        }

        score += static_cast<int>(props.limits.maxImageDimension2D / 1024);
        score += static_cast<int>(props.limits.maxBoundDescriptorSets);

        return score;
    }

    QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) const {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.GraphicsFamily = i;
            }

            if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.ComputeFamily = i;
            }

            if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.TransferFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.PresentFamily = i;
            }

            if (indices.isComplete()) break;
        }

        // Fallback: use graphics queue for compute if no dedicated compute queue
        if (!indices.ComputeFamily.has_value() && indices.GraphicsFamily.has_value()) {
            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    indices.ComputeFamily = i;
                    break;
                }
            }
        }

        return indices;
    }

    bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device) const {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport() const {
        return QuerySwapchainSupport(m_PhysicalDevice);
    }

    SwapchainSupportDetails VulkanDevice::QuerySwapchainSupport(VkPhysicalDevice device) const {
        SwapchainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);
        if (formatCount != 0) {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    VkPhysicalDeviceProperties VulkanDevice::GetProperties() const {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &props);
        return props;
    }

    uint32_t VulkanDevice::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    VkFormat VulkanDevice::FindSupportedFormat(
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features) const {

        for (VkFormat format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR &&
                (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                       (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }
        throw std::runtime_error("Failed to find supported format!");
    }

    VkFormat VulkanDevice::FindDepthFormat() const {
        return FindSupportedFormat(
            {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    VkCommandBuffer VulkanDevice::BeginSingleTimeCommands() const {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_SingleTimeCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_Device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    void VulkanDevice::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(m_Device, m_SingleTimeCommandPool, 1, &commandBuffer);
    }

    void VulkanDevice::WaitIdle() const {
        vkDeviceWaitIdle(m_Device);
    }

} // namespace GameEngine
