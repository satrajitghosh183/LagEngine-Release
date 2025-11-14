#include "WindowGLFW.hpp"
#include "../Core/Logger.hpp"
#include "../Core/Events/WindowEvents.hpp"
#include "../Core/Events/InputEvents.hpp"
#include <glad/glad.h>

namespace GameEngine {

    static bool s_GLFWInitialized = false;
    
    static void GLFWErrorCallback(int error, const char* description) {
        GE_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
    }

    Scope<Window> Window::Create(const WindowProps& props) {
        return CreateScope<WindowGLFW>(props);
    }

    WindowGLFW::WindowGLFW(const WindowProps& props) {
        Init(props);
    }

    WindowGLFW::~WindowGLFW() {
        Shutdown();
    }

    void WindowGLFW::Init(const WindowProps& props) {
        m_Data.Title = props.Title;
        m_Data.Width = props.Width;
        m_Data.Height = props.Height;
        m_Data.VSync = props.VSync;
        
        GE_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);
        
        // Initialize GLFW
        if (!s_GLFWInitialized) {
            int success = glfwInit();
            GE_CORE_ASSERT(success, "Could not initialize GLFW!");
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }
        
        // Configure GLFW
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        #ifdef __APPLE__
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        #endif
        
        // Enable MSAA
        glfwWindowHint(GLFW_SAMPLES, 4);
        
        // Create window
        m_Window = glfwCreateWindow(
            static_cast<int>(props.Width),
            static_cast<int>(props.Height),
            m_Data.Title.c_str(),
            nullptr,
            nullptr
        );
        
        GE_CORE_ASSERT(m_Window, "Failed to create GLFW window!");
        
        // Make OpenGL context current
        glfwMakeContextCurrent(m_Window);
        
        // Load OpenGL function pointers with GLAD
        int gladStatus = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        GE_CORE_ASSERT(gladStatus, "Failed to initialize GLAD!");
        
        GE_CORE_INFO("OpenGL Info:");
        GE_CORE_INFO("  Vendor:   {0}", (const char*)glGetString(GL_VENDOR));
        GE_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
        GE_CORE_INFO("  Version:  {0}", (const char*)glGetString(GL_VERSION));
        
        // Set user pointer for callbacks
        glfwSetWindowUserPointer(m_Window, &m_Data);
        
        // Enable VSync
        SetVSync(props.VSync);
        
        // Setup GLFW callbacks
        SetupCallbacks();
    }

    void WindowGLFW::Shutdown() {
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void WindowGLFW::OnUpdate() {
        glfwPollEvents();
    }

    void WindowGLFW::SwapBuffers() {
        glfwSwapBuffers(m_Window);
    }

    void WindowGLFW::SetVSync(bool enabled) {
        if (enabled)
            glfwSwapInterval(1);
        else
            glfwSwapInterval(0);
        
        m_Data.VSync = enabled;
    }

    void WindowGLFW::SetupCallbacks() {
        // Window resize callback
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.Width = width;
            data.Height = height;
            
            WindowResizeEvent event(width, height);
            data.EventCallback(event);
        });
        
        // Window close callback
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            data.EventCallback(event);
        });
        
        // Key callback
        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            
            switch (action) {
                case GLFW_PRESS: {
                    KeyPressedEvent event(key, 0);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(key);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    KeyPressedEvent event(key, 1);
                    data.EventCallback(event);
                    break;
                }
            }
        });
        
        // Mouse button callback
        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            
            switch (action) {
                case GLFW_PRESS: {
                    MouseButtonPressedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    MouseButtonReleasedEvent event(button);
                    data.EventCallback(event);
                    break;
                }
            }
        });
        
        // Mouse scroll callback
        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            
            MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
            data.EventCallback(event);
        });
        
        // Mouse move callback
        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            
            MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
            data.EventCallback(event);
        });
    }
}