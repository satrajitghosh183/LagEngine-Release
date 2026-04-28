#include "Window.h"
#include <iostream>
#include <stdexcept>

static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

Window::Window(int width, int height, const char* title)
    : m_window(nullptr)
    , m_width(width)
    , m_height(height)
    , m_resized(false)
{
    glfwSetErrorCallback(glfwErrorCallback);

    // Vulkan: tell GLFW not to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        const char* errorMsg = nullptr;
        glfwGetError(&errorMsg);
        std::string fullError = "Failed to create GLFW window (Vulkan mode)";
        if (errorMsg) {
            fullError += ": ";
            fullError += errorMsg;
        }
        throw std::runtime_error(fullError);
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

Window::~Window() {
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (win) {
        win->m_width = width;
        win->m_height = height;
        win->m_resized = true;
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}
