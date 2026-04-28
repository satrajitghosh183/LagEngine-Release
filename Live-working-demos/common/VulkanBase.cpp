#include "VulkanBase.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

// ImGui (optional — only used when config.EnableImGui == true)
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace vkdemo {

    // ---- Debug messenger callback -------------------------------------------
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT /*type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* /*pUserData*/)
    {
        if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            std::cerr << "[Vulkan] " << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }

    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pMessenger)
    {
        auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        return fn ? fn(instance, pCreateInfo, pAllocator, pMessenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT messenger,
        const VkAllocationCallbacks* pAllocator)
    {
        auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (fn) fn(instance, messenger, pAllocator);
    }

    // ---- VulkanBase lifecycle -----------------------------------------------

    VulkanBase::~VulkanBase() {
        Shutdown();
    }

    void VulkanBase::Init(const AppConfig& config) {
        m_Config = config;
        m_Width = config.Width;
        m_Height = config.Height;
        m_MaxFramesInFlight = config.MaxFramesInFlight;
        m_ImGuiEnabled = config.EnableImGui;

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow(m_Width, m_Height, config.Title.c_str(), nullptr, nullptr);
        if (!m_Window)
            throw std::runtime_error("Failed to create GLFW window");

        glfwSetWindowUserPointer(m_Window, this);
        glfwSetFramebufferSizeCallback(m_Window, FramebufferResizeCallback);
        glfwSetKeyCallback(m_Window, KeyCallback);
        glfwSetCursorPosCallback(m_Window, MouseMoveCallback);
        glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
        glfwSetScrollCallback(m_Window, ScrollCallback);

        CreateInstance();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateDepthResources();
        CreateRenderPass();
        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();

        if (m_ImGuiEnabled)
            InitImGui();
    }

    void VulkanBase::Shutdown() {
        if (m_Device == VK_NULL_HANDLE) return;

        vkDeviceWaitIdle(m_Device);

        if (m_ImGuiEnabled)
            ShutdownImGui();

        CleanupSwapchain();

        for (size_t i = 0; i < m_ImageAvailableSemaphores.size(); i++) {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(m_Device, m_RenderFinishedSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
        vkDestroyDevice(m_Device, nullptr);

        if (m_DebugMessenger != VK_NULL_HANDLE)
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        vkDestroyInstance(m_Instance, nullptr);

        if (m_Window) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
        glfwTerminate();

        m_Device = VK_NULL_HANDLE;
    }

    // ---- Instance -----------------------------------------------------------

    void VulkanBase::CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = m_Config.Title.c_str();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "LAGDemos";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        uint32_t glfwExtCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
        std::vector<const char*> extensions(glfwExts, glfwExts + glfwExtCount);

        // Append any additional instance extensions requested by the app
        for (auto ext : m_Config.AdditionalInstanceExtensions)
            extensions.push_back(ext);

        std::vector<const char*> validationLayers;
        if (m_Config.EnableValidation) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            validationLayers.push_back("VK_LAYER_KHRONOS_validation");
        }

        VkInstanceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &appInfo;
        ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        ci.ppEnabledExtensionNames = extensions.data();
        ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        ci.ppEnabledLayerNames = validationLayers.data();

        if (vkCreateInstance(&ci, nullptr, &m_Instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create Vulkan instance");

        if (m_Config.EnableValidation) {
            VkDebugUtilsMessengerCreateInfoEXT dci{};
            dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dci.pfnUserCallback = DebugCallback;
            CreateDebugUtilsMessengerEXT(m_Instance, &dci, nullptr, &m_DebugMessenger);
        }
    }

    void VulkanBase::CreateSurface() {
        if (glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create window surface");
    }

    // ---- Physical device ----------------------------------------------------

    void VulkanBase::PickPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(m_Instance, &count, nullptr);
        if (count == 0) throw std::runtime_error("No Vulkan-capable GPU found");

        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(m_Instance, &count, devices.data());

        for (auto& dev : devices) {
            auto indices = FindQueueFamilies(dev);
            if (!indices.IsComplete()) continue;

            // Check swapchain support
            uint32_t extCount;
            vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, nullptr);
            std::vector<VkExtensionProperties> exts(extCount);
            vkEnumerateDeviceExtensionProperties(dev, nullptr, &extCount, exts.data());
            bool swapchainOk = false;
            for (auto& e : exts)
                if (std::string(e.extensionName) == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                    swapchainOk = true;
            if (!swapchainOk) continue;

            m_PhysicalDevice = dev;
            break;
        }

        if (m_PhysicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("No suitable GPU found");
    }

    QueueFamilyIndices VulkanBase::FindQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &count, families.data());

        for (uint32_t i = 0; i < count; i++) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.Graphics = i;

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);
            if (presentSupport) indices.Present = i;

            if (indices.IsComplete()) break;
        }
        return indices;
    }

    // ---- Logical device -----------------------------------------------------

    void VulkanBase::CreateLogicalDevice() {
        auto indices = FindQueueFamilies(m_PhysicalDevice);
        std::set<uint32_t> uniqueFamilies = {indices.Graphics, indices.Present};

        float priority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCIs;
        for (uint32_t family : uniqueFamilies) {
            VkDeviceQueueCreateInfo qci{};
            qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qci.queueFamilyIndex = family;
            qci.queueCount = 1;
            qci.pQueuePriorities = &priority;
            queueCIs.push_back(qci);
        }

        VkPhysicalDeviceFeatures features{};
        features.fillModeNonSolid = VK_TRUE;
        features.wideLines = VK_TRUE;

        std::vector<const char*> deviceExts = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
        for (auto ext : m_Config.AdditionalDeviceExtensions)
            deviceExts.push_back(ext);

        VkDeviceCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        ci.queueCreateInfoCount = static_cast<uint32_t>(queueCIs.size());
        ci.pQueueCreateInfos = queueCIs.data();
        ci.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
        ci.ppEnabledExtensionNames = deviceExts.data();
        ci.pEnabledFeatures = &features;

        if (vkCreateDevice(m_PhysicalDevice, &ci, nullptr, &m_Device) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device");

        vkGetDeviceQueue(m_Device, indices.Graphics, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.Present, 0, &m_PresentQueue);
    }

    // ---- Swapchain ----------------------------------------------------------

    void VulkanBase::CreateSwapchain() {
        VkSurfaceCapabilitiesKHR caps;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &caps);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, nullptr);
        std::vector<VkSurfaceFormatKHR> formats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_PhysicalDevice, m_Surface, &formatCount, formats.data());

        uint32_t modeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &modeCount, nullptr);
        std::vector<VkPresentModeKHR> modes(modeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_PhysicalDevice, m_Surface, &modeCount, modes.data());

        // Choose format (prefer B8G8R8A8_SRGB)
        VkSurfaceFormatKHR surfaceFormat = formats[0];
        for (auto& f : formats) {
            if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surfaceFormat = f;
                break;
            }
        }
        m_SwapchainFormat = surfaceFormat.format;

        // Choose present mode (prefer mailbox)
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for (auto& m : modes) {
            if (m == VK_PRESENT_MODE_MAILBOX_KHR) { presentMode = m; break; }
        }

        // Choose extent
        if (caps.currentExtent.width != UINT32_MAX) {
            m_SwapchainExtent = caps.currentExtent;
        } else {
            int w, h;
            glfwGetFramebufferSize(m_Window, &w, &h);
            m_SwapchainExtent.width = std::clamp(static_cast<uint32_t>(w),
                caps.minImageExtent.width, caps.maxImageExtent.width);
            m_SwapchainExtent.height = std::clamp(static_cast<uint32_t>(h),
                caps.minImageExtent.height, caps.maxImageExtent.height);
        }

        uint32_t imageCount = caps.minImageCount + 1;
        if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
            imageCount = caps.maxImageCount;

        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = m_Surface;
        ci.minImageCount = imageCount;
        ci.imageFormat = surfaceFormat.format;
        ci.imageColorSpace = surfaceFormat.colorSpace;
        ci.imageExtent = m_SwapchainExtent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto indices = FindQueueFamilies(m_PhysicalDevice);
        uint32_t queueFamilyIndices[] = {indices.Graphics, indices.Present};
        if (indices.Graphics != indices.Present) {
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        ci.preTransform = caps.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = presentMode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_Device, &ci, nullptr, &m_Swapchain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain");

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
        m_SwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());

        // Create image views
        m_SwapchainImageViews.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo ivci{};
            ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ivci.image = m_SwapchainImages[i];
            ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ivci.format = m_SwapchainFormat;
            ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ivci.subresourceRange.baseMipLevel = 0;
            ivci.subresourceRange.levelCount = 1;
            ivci.subresourceRange.baseArrayLayer = 0;
            ivci.subresourceRange.layerCount = 1;
            if (vkCreateImageView(m_Device, &ivci, nullptr, &m_SwapchainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view");
        }

        m_Width = static_cast<int>(m_SwapchainExtent.width);
        m_Height = static_cast<int>(m_SwapchainExtent.height);
    }

    // ---- Depth resources ----------------------------------------------------

    VkFormat VulkanBase::FindDepthFormat() {
        std::vector<VkFormat> candidates = {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        for (auto format : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                return format;
        }
        throw std::runtime_error("Failed to find depth format");
    }

    void VulkanBase::CreateDepthResources() {
        m_DepthFormat = FindDepthFormat();

        VkImageCreateInfo ici{};
        ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ici.imageType = VK_IMAGE_TYPE_2D;
        ici.extent = {m_SwapchainExtent.width, m_SwapchainExtent.height, 1};
        ici.mipLevels = 1;
        ici.arrayLayers = 1;
        ici.format = m_DepthFormat;
        ici.tiling = VK_IMAGE_TILING_OPTIMAL;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        ici.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        ici.samples = VK_SAMPLE_COUNT_1_BIT;
        ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(m_Device, &ici, nullptr, &m_DepthImage);

        VkMemoryRequirements memReq;
        vkGetImageMemoryRequirements(m_Device, m_DepthImage, &memReq);

        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = memReq.size;
        mai.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(m_Device, &mai, nullptr, &m_DepthImageMemory);
        vkBindImageMemory(m_Device, m_DepthImage, m_DepthImageMemory, 0);

        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = m_DepthImage;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = m_DepthFormat;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        ivci.subresourceRange.baseMipLevel = 0;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(m_Device, &ivci, nullptr, &m_DepthImageView);
    }

    // ---- Render pass --------------------------------------------------------

    void VulkanBase::CreateRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = m_SwapchainFormat;
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

        VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = 0;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

        VkRenderPassCreateInfo rpci{};
        rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpci.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpci.pAttachments = attachments.data();
        rpci.subpassCount = 1;
        rpci.pSubpasses = &subpass;
        rpci.dependencyCount = 1;
        rpci.pDependencies = &dep;

        if (vkCreateRenderPass(m_Device, &rpci, nullptr, &m_RenderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass");
    }

    // ---- Framebuffers -------------------------------------------------------

    void VulkanBase::CreateFramebuffers() {
        m_SwapchainFramebuffers.resize(m_SwapchainImageViews.size());
        for (size_t i = 0; i < m_SwapchainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {
                m_SwapchainImageViews[i], m_DepthImageView
            };

            VkFramebufferCreateInfo fci{};
            fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fci.renderPass = m_RenderPass;
            fci.attachmentCount = static_cast<uint32_t>(attachments.size());
            fci.pAttachments = attachments.data();
            fci.width = m_SwapchainExtent.width;
            fci.height = m_SwapchainExtent.height;
            fci.layers = 1;

            if (vkCreateFramebuffer(m_Device, &fci, nullptr, &m_SwapchainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer");
        }
    }

    // ---- Command pool/buffers -----------------------------------------------

    void VulkanBase::CreateCommandPool() {
        auto indices = FindQueueFamilies(m_PhysicalDevice);

        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ci.queueFamilyIndex = indices.Graphics;

        if (vkCreateCommandPool(m_Device, &ci, nullptr, &m_CommandPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool");
    }

    void VulkanBase::CreateCommandBuffers() {
        m_CommandBuffers.resize(m_MaxFramesInFlight);

        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = m_CommandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        if (vkAllocateCommandBuffers(m_Device, &ai, m_CommandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers");
    }

    // ---- Sync objects -------------------------------------------------------

    void VulkanBase::CreateSyncObjects() {
        m_ImageAvailableSemaphores.resize(m_MaxFramesInFlight);
        m_RenderFinishedSemaphores.resize(m_MaxFramesInFlight);
        m_InFlightFences.resize(m_MaxFramesInFlight);

        VkSemaphoreCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < m_MaxFramesInFlight; i++) {
            vkCreateSemaphore(m_Device, &sci, nullptr, &m_ImageAvailableSemaphores[i]);
            vkCreateSemaphore(m_Device, &sci, nullptr, &m_RenderFinishedSemaphores[i]);
            vkCreateFence(m_Device, &fci, nullptr, &m_InFlightFences[i]);
        }
    }

    // ---- Frame cycle --------------------------------------------------------

    bool VulkanBase::BeginFrame() {
        glfwPollEvents();
        if (glfwWindowShouldClose(m_Window)) return false;

        vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(m_Device, m_Swapchain, UINT64_MAX,
            m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &m_CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            RecreateSwapchain();
            return true; // skip this frame
        }

        vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];
        vkResetCommandBuffer(cmd, 0);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cmd, &beginInfo);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpbi{};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = m_RenderPass;
        rpbi.framebuffer = m_SwapchainFramebuffers[m_CurrentImageIndex];
        rpbi.renderArea.extent = m_SwapchainExtent;
        rpbi.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpbi.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width = static_cast<float>(m_SwapchainExtent.width);
        viewport.height = static_cast<float>(m_SwapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = m_SwapchainExtent;
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        return true;
    }

    void VulkanBase::EndFrame() {
        VkCommandBuffer cmd = m_CommandBuffers[m_CurrentFrame];

        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &cmd;

        VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized) {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFramesInFlight;
    }

    // ---- Swapchain recreation -----------------------------------------------

    void VulkanBase::CleanupSwapchain() {
        vkDestroyImageView(m_Device, m_DepthImageView, nullptr);
        vkDestroyImage(m_Device, m_DepthImage, nullptr);
        vkFreeMemory(m_Device, m_DepthImageMemory, nullptr);

        for (auto fb : m_SwapchainFramebuffers) vkDestroyFramebuffer(m_Device, fb, nullptr);
        for (auto iv : m_SwapchainImageViews) vkDestroyImageView(m_Device, iv, nullptr);
        vkDestroyRenderPass(m_Device, m_RenderPass, nullptr);
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
    }

    void VulkanBase::RecreateSwapchain() {
        int w = 0, h = 0;
        glfwGetFramebufferSize(m_Window, &w, &h);
        while (w == 0 || h == 0) {
            glfwGetFramebufferSize(m_Window, &w, &h);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_Device);
        CleanupSwapchain();

        CreateSwapchain();
        CreateDepthResources();
        CreateRenderPass();
        CreateFramebuffers();

        if (OnResize) OnResize(m_Width, m_Height);
    }

    // ---- Buffer helpers -----------------------------------------------------

    uint32_t VulkanBase::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    GPUBuffer VulkanBase::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                        VkMemoryPropertyFlags memProps)
    {
        GPUBuffer buf;
        buf.Size = size;

        VkBufferCreateInfo bci{};
        bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bci.size = size;
        bci.usage = usage;
        bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateBuffer(m_Device, &bci, nullptr, &buf.Buffer);

        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(m_Device, buf.Buffer, &memReq);

        VkMemoryAllocateInfo mai{};
        mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mai.allocationSize = memReq.size;
        mai.memoryTypeIndex = FindMemoryType(memReq.memoryTypeBits, memProps);
        vkAllocateMemory(m_Device, &mai, nullptr, &buf.Memory);
        vkBindBufferMemory(m_Device, buf.Buffer, buf.Memory, 0);

        return buf;
    }

    void VulkanBase::DestroyBuffer(GPUBuffer& buf) {
        if (buf.Buffer) vkDestroyBuffer(m_Device, buf.Buffer, nullptr);
        if (buf.Memory) vkFreeMemory(m_Device, buf.Memory, nullptr);
        buf = {};
    }

    void VulkanBase::CopyToBuffer(GPUBuffer& dst, const void* data, VkDeviceSize size) {
        void* mapped;
        vkMapMemory(m_Device, dst.Memory, 0, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(m_Device, dst.Memory);
    }

    GPUBuffer VulkanBase::CreateVertexBuffer(const void* data, VkDeviceSize size) {
        // Staging buffer
        GPUBuffer staging = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CopyToBuffer(staging, data, size);

        GPUBuffer vb = CreateBuffer(size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkCommandBuffer cmd = BeginSingleTimeCommands();
        VkBufferCopy copyRegion{0, 0, size};
        vkCmdCopyBuffer(cmd, staging.Buffer, vb.Buffer, 1, &copyRegion);
        EndSingleTimeCommands(cmd);

        DestroyBuffer(staging);
        return vb;
    }

    GPUBuffer VulkanBase::CreateIndexBuffer(const void* data, VkDeviceSize size) {
        GPUBuffer staging = CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        CopyToBuffer(staging, data, size);

        GPUBuffer ib = CreateBuffer(size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkCommandBuffer cmd = BeginSingleTimeCommands();
        VkBufferCopy copyRegion{0, 0, size};
        vkCmdCopyBuffer(cmd, staging.Buffer, ib.Buffer, 1, &copyRegion);
        EndSingleTimeCommands(cmd);

        DestroyBuffer(staging);
        return ib;
    }

    VkShaderModule VulkanBase::CreateShaderModule(const std::vector<uint32_t>& spirv) {
        return CreateShaderModule(spirv.data(), spirv.size() * sizeof(uint32_t));
    }

    VkShaderModule VulkanBase::CreateShaderModule(const uint32_t* code, size_t sizeBytes) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = sizeBytes;
        ci.pCode = code;
        VkShaderModule shaderModule;
        if (vkCreateShaderModule(m_Device, &ci, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module");
        return shaderModule;
    }

    VkCommandBuffer VulkanBase::BeginSingleTimeCommands() {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool = m_CommandPool;
        ai.commandBufferCount = 1;
        VkCommandBuffer cmd;
        vkAllocateCommandBuffers(m_Device, &ai, &cmd);

        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cmd, &bi);
        return cmd;
    }

    void VulkanBase::EndSingleTimeCommands(VkCommandBuffer cmd) {
        vkEndCommandBuffer(cmd);

        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cmd;
        vkQueueSubmit(m_GraphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_GraphicsQueue);

        vkFreeCommandBuffers(m_Device, m_CommandPool, 1, &cmd);
    }

    VkPipelineLayout VulkanBase::CreatePipelineLayout(
        const std::vector<VkDescriptorSetLayout>& setLayouts,
        const std::vector<VkPushConstantRange>& pushConstants)
    {
        VkPipelineLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        ci.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        ci.pSetLayouts = setLayouts.data();
        ci.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
        ci.pPushConstantRanges = pushConstants.data();

        VkPipelineLayout layout;
        if (vkCreatePipelineLayout(m_Device, &ci, nullptr, &layout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout");
        return layout;
    }

    // ---- ImGui integration --------------------------------------------------

    void VulkanBase::InitImGui() {
        // Create descriptor pool for ImGui
        VkDescriptorPoolSize poolSizes[] = {
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}
        };
        VkDescriptorPoolCreateInfo dpci{};
        dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        dpci.maxSets = 100;
        dpci.poolSizeCount = 1;
        dpci.pPoolSizes = poolSizes;
        vkCreateDescriptorPool(m_Device, &dpci, nullptr, &m_ImGuiDescriptorPool);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(m_Window, true);

        auto indices = FindQueueFamilies(m_PhysicalDevice);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = m_Instance;
        initInfo.PhysicalDevice = m_PhysicalDevice;
        initInfo.Device = m_Device;
        initInfo.QueueFamily = indices.Graphics;
        initInfo.Queue = m_GraphicsQueue;
        initInfo.DescriptorPool = m_ImGuiDescriptorPool;
        initInfo.MinImageCount = static_cast<uint32_t>(m_SwapchainImages.size());
        initInfo.ImageCount = static_cast<uint32_t>(m_SwapchainImages.size());
        initInfo.RenderPass = m_RenderPass;

        ImGui_ImplVulkan_Init(&initInfo);

        // Upload font textures
        VkCommandBuffer cmd = BeginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture();
        EndSingleTimeCommands(cmd);
    }

    void VulkanBase::ShutdownImGui() {
        vkDeviceWaitIdle(m_Device);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(m_Device, m_ImGuiDescriptorPool, nullptr);
    }

    void VulkanBase::ImGuiNewFrame() {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanBase::ImGuiRender(VkCommandBuffer cmd) {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }

    // ---- GLFW callbacks -----------------------------------------------------

    void VulkanBase::FramebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
        auto* app = reinterpret_cast<VulkanBase*>(glfwGetWindowUserPointer(window));
        app->m_FramebufferResized = true;
    }

    void VulkanBase::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* app = reinterpret_cast<VulkanBase*>(glfwGetWindowUserPointer(window));
        if (app->OnKey) app->OnKey(window, key, scancode, action, mods);
    }

    void VulkanBase::MouseMoveCallback(GLFWwindow* window, double xpos, double ypos) {
        auto* app = reinterpret_cast<VulkanBase*>(glfwGetWindowUserPointer(window));
        if (app->OnMouseMove) app->OnMouseMove(window, xpos, ypos);
    }

    void VulkanBase::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        auto* app = reinterpret_cast<VulkanBase*>(glfwGetWindowUserPointer(window));
        if (app->OnMouseButton) app->OnMouseButton(window, button, action, mods);
    }

    void VulkanBase::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* app = reinterpret_cast<VulkanBase*>(glfwGetWindowUserPointer(window));
        if (app->OnScroll) app->OnScroll(window, xoffset, yoffset);
    }

} // namespace vkdemo
