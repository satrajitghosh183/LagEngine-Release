//
// Created by barth on 25/09/23.
//

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <cstdio>
#include "Engine.h"

Engine::Engine(int windowWidth, int windowHeight, const char *name = "Feather Project") {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    // Try OpenGL 4.3 first, then fallback to lower versions for compatibility
    struct OpenGLVersion { int major, minor; };
    OpenGLVersion versions[] = {{4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 3}, {3, 2}};
    window = nullptr;
    
    for (const auto& version : versions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, version.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, version.minor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        
        window = glfwCreateWindow(windowWidth, windowHeight, name, nullptr, nullptr);
        if (window != nullptr) {
            break; // Success!
        }
        
        // Clear the error for next attempt
        const char* errorDesc;
        glfwGetError(&errorDesc);
    }
    
    if (window == nullptr) {
        const char* errorDescription;
        int errorCode = glfwGetError(&errorDescription);
        glfwTerminate();
        std::string errorMsg = "Failed to create GLFW window";
        if (errorDescription) {
            errorMsg += ": ";
            errorMsg += errorDescription;
        }
        errorMsg += "\n\nTroubleshooting for WSL:";
        errorMsg += "\n1. For Windows 11: Ensure WSLg is enabled (should work automatically)";
        errorMsg += "\n2. For Windows 10: Install an X server (VcXsrv, X410, or Xming) and set DISPLAY";
        errorMsg += "\n3. Install Mesa: sudo apt-get install mesa-utils libgl1-mesa-glx";
        errorMsg += "\n4. Set DISPLAY: export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0.0";
        throw std::runtime_error(errorMsg);
    }

    glfwSetErrorCallback([](int error, const char *description) {
        throw std::runtime_error(description);
    });

    if (glfwRawMouseMotionSupported()) glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    onKeyPressObservable.add([this](int key) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true); // Closes the application if the escape key is pressed
        }
    });

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, [](GLFWwindow *window, int key, int scancode, int action, int mods) {
        auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
        if (action == GLFW_PRESS) {
            engine->_keyStates[key] = true;
            engine->onKeyPressObservable.notifyObservers(key);
        } else if (action == GLFW_RELEASE) {
            engine->_keyStates[key] = false;
            engine->onKeyReleaseObservable.notifyObservers(key);
        }
    });

    glfwSetScrollCallback(window, [](GLFWwindow *window, double xOffset, double yOffset) {
        auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
        engine->onMouseScrollObservable.notifyObservers(xOffset, yOffset);
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow *window, double xpos, double ypos) {
        auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
        double mouseDX = xpos - engine->mouseX;
        double mouseDY = ypos - engine->mouseY;
        if (mouseDX != 0 || mouseDY != 0) {
            engine->onMouseMoveObservable.notifyObservers(mouseDX, mouseDY);
        }

        engine->mouseX = xpos;
        engine->mouseY = ypos;
    });

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow *window, int width, int height) {
        glViewport(0, 0, width, height);

        auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
        engine->onWindowResizeObservable.notifyObservers(width, height);
    });

    // init imgui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
    // Get OpenGL version for ImGui shader
    GLint major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    if (major == 0 && minor == 0) {
        const char* versionStr = (const char*)glGetString(GL_VERSION);
        if (versionStr) {
            sscanf(versionStr, "%d.%d", &major, &minor);
        }
    }
    
    // Determine GLSL version for ImGui
    const char* imguiGlslVersion = "#version 330";
    if (major >= 4) {
        if (minor >= 3) imguiGlslVersion = "#version 430";
        else if (minor >= 2) imguiGlslVersion = "#version 420";
        else if (minor >= 1) imguiGlslVersion = "#version 410";
        else imguiGlslVersion = "#version 400";
    } else if (major == 3 && minor >= 3) {
        imguiGlslVersion = "#version 330";
    }
    
    ImGui_ImplOpenGL3_Init(imguiGlslVersion);

    // blueish background
    setClearColor(0.4f, 0.6f, 0.6f, 1.0f);

    lastFrameTime = getElapsedSeconds();

    glDepthFunc(GL_LESS);   // Specify the depth test for the z-buffer
    glEnable(GL_DEPTH_TEST);      // Enable the z-buffer test in the rasterization
    glEnable(GL_BLEND);
    glCullFace(GL_BACK); // Specifies the faces to cull (here the ones pointing away from the camera)
}
