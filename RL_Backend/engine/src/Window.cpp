#include "rldemo/Window.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>

namespace rldemo {

static bool s_GLFWInitialized = false;

static void glfwErrorCallback(int error, const char* description) {
    spdlog::error("GLFW Error ({}): {}", error, description);
}

static void framebufferSizeCallback(GLFWwindow* win, int w, int h) {
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
    if (self && w > 0 && h > 0) {
        self->PollEvents();  // ensure we get the callback
    }
}

Window::Window(const std::string& title, uint32_t width, uint32_t height)
    : m_Width(width), m_Height(height) {
    if (!s_GLFWInitialized) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            spdlog::error("Failed to init GLFW");
            return;
        }
        s_GLFWInitialized = true;
    }
    // OpenGL 3.3 for WSL compatibility (WSLg Mesa, matches bundled GLAD)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    m_Window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        spdlog::error("Failed to create GLFW window");
        return;
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* w, int x, int y) {
        auto* win = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if (win && win->m_ResizeCallback && x > 0 && y > 0) {
            win->m_Width = static_cast<uint32_t>(x);
            win->m_Height = static_cast<uint32_t>(y);
            win->m_ResizeCallback(win->m_Width, win->m_Height);
        }
    });
    glfwMakeContextCurrent(m_Window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        spdlog::error("Failed to initialize GLAD");
    }
    glfwSwapInterval(1);
}

Window::~Window() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
}

void Window::PollEvents() {
    glfwPollEvents();
}

void Window::SwapBuffers() {
    if (m_Window) glfwSwapBuffers(m_Window);
}

bool Window::ShouldClose() const {
    return m_Window ? glfwWindowShouldClose(m_Window) : true;
}

} // namespace rldemo
