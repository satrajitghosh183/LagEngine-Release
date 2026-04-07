#pragma once

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif

#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();
    void makeContextCurrent();
    
    // Create a shared OpenGL context for worker threads
    GLFWwindow* createSharedContext();
    
    GLFWwindow* getHandle() { return m_window; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool wasResized() const { return m_resized; }
    void clearResized() { m_resized = false; }

private:
    GLFWwindow* m_window;
    std::vector<GLFWwindow*> m_sharedContexts;  // Track shared contexts for cleanup
    int m_width;
    int m_height;
    bool m_resized;
    
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};

