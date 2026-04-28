//
// Created by barth on 25/09/23.
//
// Vulkan port: Engine now wraps VulkanBase for window, input, and ImGui.
//

#ifndef FEATHERGL_ENGINE_H
#define FEATHERGL_ENGINE_H

#include "Observable.h"
#include <VulkanBase.hpp>
#include <imgui.h>
#include <map>
#include <glm/vec2.hpp>

class Engine {
public:
    Engine(int windowWidth, int windowHeight, const char *name);

    ~Engine();

    void setCursorEnabled(bool enabled) {
        glfwSetInputMode(_vkBase.GetWindow(), GLFW_CURSOR, enabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

    bool isMousePressed() const {
        return glfwGetMouseButton(_vkBase.GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    }

    bool isKeyPressed(int key) const {
        return _keyStates.find(key) != _keyStates.end() && _keyStates.at(key);
    }

    GLFWwindow *getWindow() const {
        return _vkBase.GetWindow();
    }

    void windowSize(int *width, int *height) {
        glfwGetWindowSize(_vkBase.GetWindow(), width, height);
    }

    float getElapsedSeconds() const {
        return (float) glfwGetTime();
    }

    float getDeltaSeconds() const {
        return deltaTime;
    }

    void setClearColor(float r, float g, float b, float a) {
        // Stored for future use; VulkanBase clear color is set in BeginFrame
        (void)r; (void)g; (void)b; (void)a;
    }

    void start();

    glm::vec2 getMousePosition() {
        double x, y;
        glfwGetCursorPos(_vkBase.GetWindow(), &x, &y);
        return {x, y};
    }

    glm::vec2 getWindowSize() {
        int width, height;
        glfwGetWindowSize(_vkBase.GetWindow(), &width, &height);
        return {width, height};
    }

    // Access the VulkanBase for rendering
    vkdemo::VulkanBase& vulkanBase() { return _vkBase; }

    Observable<> onExecuteLoopObservable{};

    Observable<int, int> onWindowResizeObservable{};

    Observable<int> onKeyPressObservable{};
    Observable<int> onKeyReleaseObservable{};
    Observable<double, double> onMouseScrollObservable{};
    Observable<double, double> onMouseMoveObservable{};

private:
    double mouseX{};
    double mouseY{};

    float lastFrameTime{};
    float deltaTime{};

    std::map<int, bool> _keyStates{};

    vkdemo::VulkanBase _vkBase;
};

#endif //FEATHERGL_ENGINE_H
