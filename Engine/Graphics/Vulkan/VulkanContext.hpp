#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <functional>

namespace GameEngine {

    struct VulkanContextCreateInfo {
        std::string AppName = "GameEngine";
        uint32_t AppVersion = VK_MAKE_VERSION(1, 0, 0);
        bool EnableValidation = true;
    };

    class VulkanContext {
    public:
        VulkanContext() = default;
        ~VulkanContext();

        void Init(const VulkanContextCreateInfo& createInfo = {});
        void Shutdown();

        VkInstance GetInstance() const { return m_Instance; }
        bool ValidationEnabled() const { return m_ValidationEnabled; }

        static VulkanContext& Get();

    private:
        void createInstance(const VulkanContextCreateInfo& createInfo);
        void setupDebugMessenger();
        bool checkValidationLayerSupport() const;
        std::vector<const char*> getRequiredExtensions() const;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

    private:
        VkInstance m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        bool m_ValidationEnabled = false;
        bool m_Initialized = false;

        static VulkanContext* s_Instance;
    };

} // namespace GameEngine
