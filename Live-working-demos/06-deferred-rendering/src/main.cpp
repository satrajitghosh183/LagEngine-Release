#include "Window.h"
#include "DemoScene.h"
#include "DAGScheduler.h"
#include "PipelineController.h"
#include "UI.h"
#include "Profiler.h"
#include "Camera.h"
#include <glad/glad.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <chrono>

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    try {
        // Create window
        Window window(1280, 720, "GPU Scheduling Lab Demo");
        window.makeContextCurrent();
        
        // Load OpenGL functions
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            throw std::runtime_error("Failed to initialize GLAD");
        }
        
        std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
        
        // Enable VSync
        glfwSwapInterval(1);
        
        std::cout << "Initializing demo scene..." << std::endl;
        // Initialize demo scene
        DemoScene scene(1280, 720);
        scene.initialize();
        std::cout << "Demo scene initialized successfully." << std::endl;
        
        std::cout << "Creating scheduler..." << std::endl;
        // Create scheduler
        auto scheduler = std::make_shared<DAGScheduler>(scene.getRenderGraph());
        scheduler->setMainWindow(window.getHandle());  // Set main window for context sharing
        scheduler->setMode(SchedulingMode::Baseline);
        
        // Create pipeline controller
        PipelineController controller;
        controller.setStreamCountLimits(2, 8);
        
        // Connect scheduler to controller for Full System mode
        scheduler->setAIBackend(&controller);
        
        // Connect controller to scheduler for stream adaptation
        controller.setSchedulerAdaptCallback([scheduler](int streamCount) {
            scheduler->adaptStreamCount(streamCount);
        });
        
        std::cout << "Initializing UI..." << std::endl;
        // Initialize UI
        UI ui;
        if (!ui.initialize(window.getHandle())) {
            throw std::runtime_error("Failed to initialize UI");
        }
        std::cout << "UI initialized successfully." << std::endl;
        std::cout << "Entering main loop..." << std::endl;
        
        // Main loop
        auto lastTime = std::chrono::high_resolution_clock::now();
        float deltaTime = 0.0f;
        
        while (!window.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
            deltaTime = duration.count() / 1000000.0f;
            lastTime = currentTime;
            
            window.pollEvents();
            
            // Simple camera controls
            if (glfwGetKey(window.getHandle(), GLFW_KEY_W) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos + scene.getCamera().getForward() * deltaTime * 10.0f);
            }
            if (glfwGetKey(window.getHandle(), GLFW_KEY_S) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos - scene.getCamera().getForward() * deltaTime * 10.0f);
            }
            if (glfwGetKey(window.getHandle(), GLFW_KEY_A) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                glm::vec3 right = glm::cross(scene.getCamera().getForward(), scene.getCamera().getUp());
                scene.getCamera().setPosition(pos - right * deltaTime * 10.0f);
            }
            if (glfwGetKey(window.getHandle(), GLFW_KEY_D) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                glm::vec3 right = glm::cross(scene.getCamera().getForward(), scene.getCamera().getUp());
                scene.getCamera().setPosition(pos + right * deltaTime * 10.0f);
            }
            if (glfwGetKey(window.getHandle(), GLFW_KEY_Q) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos + glm::vec3(0, 1, 0) * deltaTime * 10.0f);
            }
            if (glfwGetKey(window.getHandle(), GLFW_KEY_E) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos - glm::vec3(0, 1, 0) * deltaTime * 10.0f);
            }
            
            if (window.wasResized()) {
                glViewport(0, 0, window.getWidth(), window.getHeight());
                scene.resize(window.getWidth(), window.getHeight());
                window.clearResized();
            }
            
            // Begin frame
            Profiler::instance().beginFrame();
            ui.beginFrame();
            
            // Update scene
            scene.update(deltaTime);
            
            // Execute render graph via scheduler (with error handling)
            try {
                scheduler->executeFrame();
            } catch (const std::exception& e) {
                std::cerr << "Error in scheduler execution: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error in scheduler execution" << std::endl;
            }
            
            // Update pipeline controller
            float frameTime = Profiler::instance().getFrameTime();
            float cpuTime = scheduler->getCPUTime();
            float gpuTime = scheduler->getGPUTime();
            float gpuUtil = std::min(1.0f, gpuTime / frameTime);
            controller.updateFrame(cpuTime, gpuTime, frameTime, gpuUtil);
            // Note: Stream adaptation happens automatically via callback in Full System mode
            
            // Render UI
            float fps = Profiler::instance().getFPS();
            ui.render(scheduler.get(), &controller, scene.getRenderGraph().get(), frameTime, fps);
            
            // End frame
            Profiler::instance().endFrame();
            ui.endFrame();
            
            window.swapBuffers();
        }
        
        ui.shutdown();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwTerminate();
    return 0;
}

