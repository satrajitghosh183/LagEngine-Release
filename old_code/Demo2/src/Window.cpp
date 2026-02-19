#include "Window.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <string>

static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

Window::Window(int width, int height, const char* title)
    : m_width(width)
    , m_height(height)
    , m_resized(false)
{
    // Set error callback to see detailed error messages
    glfwSetErrorCallback(glfwErrorCallback);
    
    // Try OpenGL 4.6 first, then fall back to lower versions
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    
    // If 4.6 fails, try 4.3 (minimum for compute shaders)
    if (!m_window) {
        std::cerr << "Failed to create window with OpenGL 4.6, trying 4.3..." << std::endl;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    }
    
    // If 4.3 fails, try 3.3 (minimum for most features)
    if (!m_window) {
        std::cerr << "Failed to create window with OpenGL 4.3, trying 3.3..." << std::endl;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    }
    if (!m_window) {
        const char* errorMsg;
        glfwGetError(&errorMsg);
        std::string fullError = "Failed to create GLFW window";
        if (errorMsg) {
            fullError += ": ";
            fullError += errorMsg;
        }
        fullError += "\n\nTroubleshooting:\n";
        fullError += "1. In WSL, ensure WSLg is enabled (Windows 11) or X11 forwarding is set up\n";
        fullError += "2. Check DISPLAY environment variable: echo $DISPLAY\n";
        fullError += "3. For X11: export DISPLAY=:0 or use VcXsrv/Xming\n";
        fullError += "4. Verify GPU drivers: nvidia-smi (if using NVIDIA GPU)\n";
        throw std::runtime_error(fullError);
    }
    
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
}

Window::~Window() {
    // Destroy all shared contexts first
    for (auto* ctx : m_sharedContexts) {
        if (ctx) {
            glfwDestroyWindow(ctx);
        }
    }
    m_sharedContexts.clear();
    
    // Destroy main window
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

void Window::swapBuffers() {
    glfwSwapBuffers(m_window);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::makeContextCurrent() {
    glfwMakeContextCurrent(m_window);
}

GLFWwindow* Window::createSharedContext() {
    if (!m_window) {
        return nullptr;
    }
    
    // Create a hidden window with shared context
    // Use same OpenGL version hints as main window
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);  // Hidden window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    
    GLFWwindow* sharedWindow = glfwCreateWindow(1, 1, "", nullptr, m_window);
    
    if (!sharedWindow) {
        // Try lower version
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        sharedWindow = glfwCreateWindow(1, 1, "", nullptr, m_window);
    }
    
    if (!sharedWindow) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        sharedWindow = glfwCreateWindow(1, 1, "", nullptr, m_window);
    }
    
    if (sharedWindow) {
        m_sharedContexts.push_back(sharedWindow);
    }
    
    return sharedWindow;
}

