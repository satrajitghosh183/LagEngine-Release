#pragma once

#include "../Core/Base.hpp"
#include "../Core/Events/Event.hpp"
#include <string>
#include <functional>

namespace GameEngine {

    /**
     * @brief Window properties
     */
    struct WindowProps {
        std::string Title;
        uint32_t Width;
        uint32_t Height;
        bool VSync;
        
        WindowProps(const std::string& title = "GameEngine",
                   uint32_t width = 1280,
                   uint32_t height = 720,
                   bool vsync = true)
            : Title(title), Width(width), Height(height), VSync(vsync) {}
    };

    /**
     * @brief Abstract window interface
     * 
     * Platform-agnostic window abstraction
     * Implemented by WindowGLFW for OpenGL rendering
     */
    class Window {
    public:
        using EventCallbackFn = std::function<void(Event&)>;
        
        virtual ~Window() = default;
        
        /**
         * @brief Update window (poll events)
         */
        virtual void OnUpdate() = 0;
        
        /**
         * @brief Swap front and back buffers
         */
        virtual void SwapBuffers() = 0;
        
        /**
         * @brief Get window width
         */
        virtual uint32_t GetWidth() const = 0;
        
        /**
         * @brief Get window height
         */
        virtual uint32_t GetHeight() const = 0;
        
        /**
         * @brief Get aspect ratio
         */
        virtual float GetAspectRatio() const = 0;
        
        /**
         * @brief Set event callback function
         */
        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        
        /**
         * @brief Enable/disable VSync
         */
        virtual void SetVSync(bool enabled) = 0;
        
        /**
         * @brief Check if VSync is enabled
         */
        virtual bool IsVSync() const = 0;
        
        /**
         * @brief Get native window handle (platform-specific)
         */
        virtual void* GetNativeWindow() const = 0;
        
        /**
         * @brief Create window (factory method)
         */
        static Scope<Window> Create(const WindowProps& props = WindowProps());
    };
}