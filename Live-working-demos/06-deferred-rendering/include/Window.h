#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose() const;
    void pollEvents();

    GLFWwindow* getHandle() { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool wasResized() const { return m_resized; }
    void clearResized() { m_resized = false; }

private:
    GLFWwindow* m_window;
    int m_width;
    int m_height;
    bool m_resized;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};
