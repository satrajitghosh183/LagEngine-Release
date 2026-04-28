#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "Window.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief GLFW window implementation for Vulkan
     *
     * Features:
     * - Vulkan surface creation
     * - Input event callbacks
     * - Window event callbacks
     * - VSync control (via swapchain present mode)
     */
    class WindowGLFW : public Window {
    public:
        WindowGLFW(const WindowProps& props);
        virtual ~WindowGLFW();

        void OnUpdate() override;
        void SwapBuffers() override;

        uint32_t GetWidth() const override { return m_Data.Width; }
        uint32_t GetHeight() const override { return m_Data.Height; }
        float GetAspectRatio() const override {
            if (m_Data.Height == 0) return 1.0f;
            return static_cast<float>(m_Data.Width) / static_cast<float>(m_Data.Height);
        }

        void SetEventCallback(const EventCallbackFn& callback) override {
            m_Data.EventCallback = callback;
        }

        void SetVSync(bool enabled) override;
        bool IsVSync() const override { return m_Data.VSync; }
        void SetFullscreen(bool fullscreen) override;
        bool IsFullscreen() const override { return m_Data.Fullscreen; }

        void* GetNativeWindow() const override { return m_Window; }

        VkSurfaceKHR GetVulkanSurface() const { return m_Surface; }
        void CreateVulkanSurface(VkInstance instance);
        void DestroyVulkanSurface(VkInstance instance);

        bool WasResized() const { return m_Data.FramebufferResized; }
        void ResetResizedFlag() { m_Data.FramebufferResized = false; }

        void GetFramebufferSize(int* width, int* height) const {
            glfwGetFramebufferSize(m_Window, width, height);
        }

    private:
        void Init(const WindowProps& props);
        void Shutdown();
        void SetupCallbacks();

    private:
        GLFWwindow* m_Window = nullptr;
        VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
            bool Fullscreen = false;
            bool FramebufferResized = false;
            int WindowedX = 100, WindowedY = 100;
            uint32_t WindowedWidth = 0, WindowedHeight = 0;
            EventCallbackFn EventCallback;
        };

        WindowData m_Data;
    };
}
