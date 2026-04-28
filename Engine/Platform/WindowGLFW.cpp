#include "WindowGLFW.hpp"
#include "../Core/Logger.hpp"
#include "../Core/Events/WindowEvents.hpp"
#include "../Core/Events/InputEvents.hpp"
#include <stdexcept>

namespace GameEngine {

    static bool s_GLFWInitialized = false;
    static int s_GLFWWindowCount = 0;

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
            if (!success) {
                GE_CORE_ERROR("Could not initialize GLFW!");
                throw std::runtime_error("Could not initialize GLFW!");
            }
            glfwSetErrorCallback(GLFWErrorCallback);
            s_GLFWInitialized = true;
        }

        // Tell GLFW not to create an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        // Create window
        m_Window = glfwCreateWindow(
            static_cast<int>(props.Width),
            static_cast<int>(props.Height),
            m_Data.Title.c_str(),
            nullptr,
            nullptr
        );

        if (!m_Window) {
            GE_CORE_ERROR("Failed to create GLFW window!");
            throw std::runtime_error("Failed to create GLFW window!");
        }
        s_GLFWWindowCount++;

        // Check Vulkan support
        if (!glfwVulkanSupported()) {
            GE_CORE_ERROR("Vulkan is not supported on this system!");
            throw std::runtime_error("Vulkan is not supported!");
        }

        GE_CORE_INFO("Vulkan window created successfully");

        // Set user pointer for callbacks
        glfwSetWindowUserPointer(m_Window, &m_Data);

        // Setup GLFW callbacks
        SetupCallbacks();
    }

    void WindowGLFW::Shutdown() {
        if (m_Window) {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
            s_GLFWWindowCount--;
            if (s_GLFWWindowCount <= 0) {
                s_GLFWWindowCount = 0;
                glfwTerminate();
                s_GLFWInitialized = false;
            }
        }
    }

    void WindowGLFW::OnUpdate() {
        glfwPollEvents();
    }

    void WindowGLFW::SwapBuffers() {
        // Vulkan uses swapchain presentation, not glfwSwapBuffers
        // This is now a no-op; presentation is handled by VulkanSwapchain
    }

    void WindowGLFW::SetVSync(bool enabled) {
        // VSync in Vulkan is controlled via swapchain present mode
        // (FIFO = vsync on, MAILBOX/IMMEDIATE = vsync off)
        // Store the preference; swapchain recreation will pick it up
        m_Data.VSync = enabled;
    }

    void WindowGLFW::SetFullscreen(bool fullscreen) {
        if (m_Data.Fullscreen == fullscreen) return;

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if (fullscreen) {
            glfwGetWindowPos(m_Window, &m_Data.WindowedX, &m_Data.WindowedY);
            m_Data.WindowedWidth = m_Data.Width;
            m_Data.WindowedHeight = m_Data.Height;
            glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else {
            int w = m_Data.WindowedWidth > 0 ? static_cast<int>(m_Data.WindowedWidth) : static_cast<int>(m_Data.Width);
            int h = m_Data.WindowedHeight > 0 ? static_cast<int>(m_Data.WindowedHeight) : static_cast<int>(m_Data.Height);
            glfwSetWindowMonitor(m_Window, nullptr, m_Data.WindowedX, m_Data.WindowedY, w, h, mode->refreshRate);
        }

        m_Data.Fullscreen = fullscreen;
        m_Data.FramebufferResized = true;
    }

    void WindowGLFW::CreateVulkanSurface(VkInstance instance) {
        if (glfwCreateWindowSurface(instance, m_Window, nullptr, &m_Surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan window surface!");
        }
        GE_CORE_INFO("Vulkan surface created");
    }

    void WindowGLFW::DestroyVulkanSurface(VkInstance instance) {
        if (m_Surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }
    }

    void WindowGLFW::SetupCallbacks() {
        // Framebuffer resize callback
        glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.FramebufferResized = true;
            data.Width = static_cast<uint32_t>(width);
            data.Height = static_cast<uint32_t>(height);
        });

        // Window resize callback
        glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            data.Width = width;
            data.Height = height;

            WindowResizeEvent event(width, height);
            if (data.EventCallback) data.EventCallback(event);
        });

        // Window close callback
        glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            WindowCloseEvent event;
            if (data.EventCallback) data.EventCallback(event);
        });

        // Key callback
        glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
            (void)scancode; (void)mods;
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    KeyPressedEvent event(key, 0);
                    if (data.EventCallback) data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    KeyReleasedEvent event(key);
                    if (data.EventCallback) data.EventCallback(event);
                    break;
                }
                case GLFW_REPEAT: {
                    KeyPressedEvent event(key, 1);
                    if (data.EventCallback) data.EventCallback(event);
                    break;
                }
            }
        });

        // Mouse button callback
        glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
            (void)mods;
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

            switch (action) {
                case GLFW_PRESS: {
                    MouseButtonPressedEvent event(button);
                    if (data.EventCallback) data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE: {
                    MouseButtonReleasedEvent event(button);
                    if (data.EventCallback) data.EventCallback(event);
                    break;
                }
            }
        });

        // Mouse scroll callback
        glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            MouseScrolledEvent event(static_cast<float>(xOffset), static_cast<float>(yOffset));
            if (data.EventCallback) data.EventCallback(event);
        });

        // Mouse move callback
        glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
            WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
            MouseMovedEvent event(static_cast<float>(xPos), static_cast<float>(yPos));
            if (data.EventCallback) data.EventCallback(event);
        });
    }
}
