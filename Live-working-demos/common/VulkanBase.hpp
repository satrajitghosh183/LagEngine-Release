#pragma once

/**
 * VulkanBase — Shared bootstrap library for Live-working-demos
 *
 * Provides a self-contained Vulkan context: instance, device, swapchain,
 * command buffers, sync, and optional ImGui integration. Each demo links
 * this library and gets a ready-to-draw Vulkan environment.
 */

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <functional>
#include <cstdint>
#include <stdexcept>

namespace vkdemo {

    // ---- Configuration -------------------------------------------------------
    struct AppConfig {
        std::string Title = "Vulkan Demo";
        int Width = 800;
        int Height = 600;
        bool EnableValidation = true;
        bool EnableImGui = false;
        int MaxFramesInFlight = 2;
        // Additional Vulkan device extensions to enable (e.g. for external memory interop)
        std::vector<const char*> AdditionalDeviceExtensions;
        // Additional Vulkan instance extensions to enable
        std::vector<const char*> AdditionalInstanceExtensions;
    };

    // ---- Simple GPU buffer helper -------------------------------------------
    struct GPUBuffer {
        VkBuffer Buffer = VK_NULL_HANDLE;
        VkDeviceMemory Memory = VK_NULL_HANDLE;
        VkDeviceSize Size = 0;
    };

    // ---- Queue family indices -----------------------------------------------
    struct QueueFamilyIndices {
        uint32_t Graphics = UINT32_MAX;
        uint32_t Present  = UINT32_MAX;
        bool IsComplete() const { return Graphics != UINT32_MAX && Present != UINT32_MAX; }
    };

    // ---- VulkanBase class ---------------------------------------------------
    class VulkanBase {
    public:
        VulkanBase() = default;
        ~VulkanBase();

        // No copy
        VulkanBase(const VulkanBase&) = delete;
        VulkanBase& operator=(const VulkanBase&) = delete;

        // ---- Lifecycle ----
        void Init(const AppConfig& config);
        void Shutdown();

        // Returns false when window should close
        bool BeginFrame();
        void EndFrame();

        // ---- Accessors ----
        GLFWwindow*    GetWindow()        const { return m_Window; }
        VkInstance     GetInstance()       const { return m_Instance; }
        VkDevice       GetDevice()        const { return m_Device; }
        VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
        VkQueue        GetGraphicsQueue() const { return m_GraphicsQueue; }
        VkQueue        GetPresentQueue()  const { return m_PresentQueue; }
        VkCommandPool  GetCommandPool()   const { return m_CommandPool; }
        VkRenderPass   GetRenderPass()    const { return m_RenderPass; }
        VkExtent2D     GetSwapchainExtent() const { return m_SwapchainExtent; }
        VkFormat       GetSwapchainFormat() const { return m_SwapchainFormat; }
        uint32_t       GetImageIndex()    const { return m_CurrentImageIndex; }
        uint32_t       GetFrameIndex()    const { return m_CurrentFrame; }
        VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }
        VkFramebuffer  GetCurrentFramebuffer() const { return m_SwapchainFramebuffers[m_CurrentImageIndex]; }

        int GetWidth()  const { return m_Width; }
        int GetHeight() const { return m_Height; }

        // ---- Helpers ----
        GPUBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                               VkMemoryPropertyFlags memProps);
        void DestroyBuffer(GPUBuffer& buf);
        void CopyToBuffer(GPUBuffer& dst, const void* data, VkDeviceSize size);

        GPUBuffer CreateVertexBuffer(const void* data, VkDeviceSize size);
        GPUBuffer CreateIndexBuffer(const void* data, VkDeviceSize size);

        VkShaderModule CreateShaderModule(const std::vector<uint32_t>& spirv);
        VkShaderModule CreateShaderModule(const uint32_t* code, size_t sizeBytes);

        // One-shot command buffer helpers
        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer cmd);

        // Pipeline helpers
        VkPipelineLayout CreatePipelineLayout(
            const std::vector<VkDescriptorSetLayout>& setLayouts = {},
            const std::vector<VkPushConstantRange>& pushConstants = {});

        // ---- ImGui ----
        void ImGuiNewFrame();
        void ImGuiRender(VkCommandBuffer cmd);

        // ---- Callbacks ----
        std::function<void(int, int)> OnResize;
        std::function<void(GLFWwindow*, int, int, int, int)> OnKey;
        std::function<void(GLFWwindow*, double, double)> OnMouseMove;
        std::function<void(GLFWwindow*, int, int, int)> OnMouseButton;
        std::function<void(GLFWwindow*, double, double)> OnScroll;

    private:
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapchain();
        void CreateRenderPass();
        void CreateFramebuffers();
        void CreateCommandPool();
        void CreateCommandBuffers();
        void CreateSyncObjects();
        void CreateDepthResources();
        void RecreateSwapchain();
        void CleanupSwapchain();

        void InitImGui();
        void ShutdownImGui();

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props);
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        VkFormat FindDepthFormat();

        static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
        static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
        static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

        // ---- State ----
        GLFWwindow*      m_Window = nullptr;
        VkInstance        m_Instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
        VkSurfaceKHR     m_Surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
        VkDevice         m_Device = VK_NULL_HANDLE;
        VkQueue          m_GraphicsQueue = VK_NULL_HANDLE;
        VkQueue          m_PresentQueue = VK_NULL_HANDLE;

        VkSwapchainKHR   m_Swapchain = VK_NULL_HANDLE;
        VkFormat         m_SwapchainFormat{};
        VkExtent2D       m_SwapchainExtent{};
        std::vector<VkImage>     m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        std::vector<VkFramebuffer> m_SwapchainFramebuffers;

        VkImage          m_DepthImage = VK_NULL_HANDLE;
        VkDeviceMemory   m_DepthImageMemory = VK_NULL_HANDLE;
        VkImageView      m_DepthImageView = VK_NULL_HANDLE;
        VkFormat         m_DepthFormat{};

        VkRenderPass     m_RenderPass = VK_NULL_HANDLE;
        VkCommandPool    m_CommandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_CommandBuffers;

        std::vector<VkSemaphore> m_ImageAvailableSemaphores;
        std::vector<VkSemaphore> m_RenderFinishedSemaphores;
        std::vector<VkFence>     m_InFlightFences;

        VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;

        uint32_t m_CurrentFrame = 0;
        uint32_t m_CurrentImageIndex = 0;
        int      m_MaxFramesInFlight = 2;
        bool     m_FramebufferResized = false;
        bool     m_ImGuiEnabled = false;
        int      m_Width = 800;
        int      m_Height = 600;

        AppConfig m_Config;
    };

} // namespace vkdemo
