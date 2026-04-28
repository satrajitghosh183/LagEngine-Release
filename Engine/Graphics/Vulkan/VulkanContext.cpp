#include "VulkanContext.hpp"
#include "../../Core/Logger.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstring>
#include <stdexcept>
#include <set>

namespace GameEngine {

    VulkanContext* VulkanContext::s_Instance = nullptr;

    static const std::vector<const char*> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger) {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VulkanContext::~VulkanContext() {
        if (m_Initialized) {
            Shutdown();
        }
    }

    VulkanContext& VulkanContext::Get() {
        if (!s_Instance) {
            throw std::runtime_error("VulkanContext not initialized!");
        }
        return *s_Instance;
    }

    void VulkanContext::Init(const VulkanContextCreateInfo& createInfo) {
        if (m_Initialized) return;

        s_Instance = this;
        m_ValidationEnabled = createInfo.EnableValidation;

        if (m_ValidationEnabled && !checkValidationLayerSupport()) {
            GE_CORE_WARN("Validation layers requested but not available, disabling");
            m_ValidationEnabled = false;
        }

        createInstance(createInfo);

        if (m_ValidationEnabled) {
            setupDebugMessenger();
        }

        m_Initialized = true;
        GE_CORE_INFO("VulkanContext initialized successfully");
    }

    void VulkanContext::Shutdown() {
        if (!m_Initialized) return;

        if (m_DebugMessenger != VK_NULL_HANDLE) {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        if (m_Instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_Instance, nullptr);
            m_Instance = VK_NULL_HANDLE;
        }

        m_Initialized = false;
        s_Instance = nullptr;
        GE_CORE_INFO("VulkanContext shut down");
    }

    void VulkanContext::createInstance(const VulkanContextCreateInfo& createInfo) {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = createInfo.AppName.c_str();
        appInfo.applicationVersion = createInfo.AppVersion;
        appInfo.pEngineName = "LAG Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        auto extensions = getRequiredExtensions();

        VkInstanceCreateInfo instanceCreateInfo{};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (m_ValidationEnabled) {
            instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            instanceCreateInfo.ppEnabledLayerNames = s_ValidationLayers.data();

            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;

            instanceCreateInfo.pNext = &debugCreateInfo;
        } else {
            instanceCreateInfo.enabledLayerCount = 0;
            instanceCreateInfo.pNext = nullptr;
        }

        VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance! VkResult: " + std::to_string(result));
        }

        // Log available extensions
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

        GE_CORE_INFO("Available Vulkan extensions ({0}):", extensionCount);
        for (const auto& ext : availableExtensions) {
            GE_CORE_TRACE("  {0}", ext.extensionName);
        }
    }

    void VulkanContext::setupDebugMessenger() {
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        VkResult result = CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
        if (result != VK_SUCCESS) {
            GE_CORE_WARN("Failed to set up Vulkan debug messenger");
        }
    }

    bool VulkanContext::checkValidationLayerSupport() const {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : s_ValidationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) return false;
        }
        return true;
    }

    std::vector<const char*> VulkanContext::getRequiredExtensions() const {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_ValidationEnabled) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        (void)messageType;
        (void)pUserData;

        switch (messageSeverity) {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                GE_CORE_TRACE("Vulkan Validation: {0}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                GE_CORE_INFO("Vulkan Validation: {0}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                GE_CORE_WARN("Vulkan Validation: {0}", pCallbackData->pMessage);
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                GE_CORE_ERROR("Vulkan Validation: {0}", pCallbackData->pMessage);
                break;
            default:
                break;
        }

        return VK_FALSE;
    }

} // namespace GameEngine
