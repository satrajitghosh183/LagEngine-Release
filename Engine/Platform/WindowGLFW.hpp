#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "Window.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief GLFW window implementation for OpenGL
     * 
     * Features:
     * - OpenGL context creation
     * - Input event callbacks
     * - Window event callbacks
     * - VSync control
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
        
    private:
        void Init(const WindowProps& props);
        void Shutdown();
        void SetupCallbacks();
        
    private:
        GLFWwindow* m_Window;
        
        struct WindowData {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
            bool Fullscreen = false;
            int WindowedX = 100, WindowedY = 100;
            uint32_t WindowedWidth = 0, WindowedHeight = 0;
            EventCallbackFn EventCallback;
        };
        
        WindowData m_Data;
    };
}