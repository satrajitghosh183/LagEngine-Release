//
// Created by barth on 25/09/23.
//
// Vulkan port: Engine initializes VulkanBase with ImGui support.
//

#include "Engine.h"
#include <stdexcept>
#include <cstdio>

Engine::Engine(int windowWidth, int windowHeight, const char *name) {
    vkdemo::AppConfig config;
    config.Title = name ? name : "HPBD Cloth Simulation";
    config.Width = windowWidth;
    config.Height = windowHeight;
    config.EnableValidation = true;
    config.EnableImGui = true;

    _vkBase.Init(config);

    GLFWwindow* window = _vkBase.GetWindow();

    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    onKeyPressObservable.add([this](int key) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(_vkBase.GetWindow(), true);
        }
    });

    // Wire up VulkanBase callbacks to Engine observables
    _vkBase.OnKey = [this](GLFWwindow* /*w*/, int key, int /*scancode*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS) {
            _keyStates[key] = true;
            onKeyPressObservable.notifyObservers(key);
        } else if (action == GLFW_RELEASE) {
            _keyStates[key] = false;
            onKeyReleaseObservable.notifyObservers(key);
        }
    };

    _vkBase.OnScroll = [this](GLFWwindow* /*w*/, double xOffset, double yOffset) {
        onMouseScrollObservable.notifyObservers(xOffset, yOffset);
    };

    _vkBase.OnMouseMove = [this](GLFWwindow* /*w*/, double xpos, double ypos) {
        double mouseDX = xpos - mouseX;
        double mouseDY = ypos - mouseY;
        if (mouseDX != 0 || mouseDY != 0) {
            onMouseMoveObservable.notifyObservers(mouseDX, mouseDY);
        }
        mouseX = xpos;
        mouseY = ypos;
    };

    _vkBase.OnResize = [this](int width, int height) {
        onWindowResizeObservable.notifyObservers(width, height);
    };

    lastFrameTime = getElapsedSeconds();
}

Engine::~Engine() {
    _vkBase.Shutdown();
}

void Engine::start() {
    int width, height;
    glfwGetWindowSize(_vkBase.GetWindow(), &width, &height);
    onWindowResizeObservable.notifyObservers(width, height);

    while (true) {
        float newFrameTime = getElapsedSeconds();
        deltaTime = newFrameTime - lastFrameTime;
        lastFrameTime = newFrameTime;

        // VulkanBase::BeginFrame polls events and begins command buffer recording
        if (!_vkBase.BeginFrame())
            break;

        onExecuteLoopObservable.notifyObservers();

        _vkBase.EndFrame();
    }

    vkDeviceWaitIdle(_vkBase.GetDevice());
}
