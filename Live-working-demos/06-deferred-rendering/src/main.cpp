#include "DemoScene.h"
#include "DAGScheduler.h"
#include "PipelineController.h"
#include "UI.h"
#include "Profiler.h"
#include "Camera.h"
#include "VulkanBase.hpp"
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
        // Create VulkanBase context (handles window, instance, device, swapchain, etc.)
        vkdemo::AppConfig config;
        config.Title = "GPU Scheduling Lab Demo (Vulkan)";
        config.Width = 1280;
        config.Height = 720;
        config.EnableValidation = true;
        config.EnableImGui = true;
        config.MaxFramesInFlight = 2;

        vkdemo::VulkanBase vkBase;
        vkBase.Init(config);

        std::cout << "Vulkan initialized successfully." << std::endl;

        std::cout << "Initializing demo scene..." << std::endl;
        // Initialize demo scene
        DemoScene scene(&vkBase, config.Width, config.Height);
        scene.initialize();
        std::cout << "Demo scene initialized successfully." << std::endl;

        std::cout << "Creating scheduler..." << std::endl;
        // Create scheduler
        auto scheduler = std::make_shared<DAGScheduler>(scene.getRenderGraph());
        scheduler->setMainWindow(vkBase.GetWindow());
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
        if (!ui.initialize(&vkBase)) {
            throw std::runtime_error("Failed to initialize UI");
        }
        std::cout << "UI initialized successfully." << std::endl;
        std::cout << "Entering main loop..." << std::endl;

        // Main loop
        auto lastTime = std::chrono::high_resolution_clock::now();
        float deltaTime = 0.0f;

        while (!glfwWindowShouldClose(vkBase.GetWindow())) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - lastTime);
            deltaTime = duration.count() / 1000000.0f;
            lastTime = currentTime;

            glfwPollEvents();

            // Simple camera controls
            GLFWwindow* win = vkBase.GetWindow();
            if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos + scene.getCamera().getForward() * deltaTime * 10.0f);
            }
            if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos - scene.getCamera().getForward() * deltaTime * 10.0f);
            }
            if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                glm::vec3 right = glm::cross(scene.getCamera().getForward(), scene.getCamera().getUp());
                scene.getCamera().setPosition(pos - right * deltaTime * 10.0f);
            }
            if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                glm::vec3 right = glm::cross(scene.getCamera().getForward(), scene.getCamera().getUp());
                scene.getCamera().setPosition(pos + right * deltaTime * 10.0f);
            }
            if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos + glm::vec3(0, 1, 0) * deltaTime * 10.0f);
            }
            if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS) {
                glm::vec3 pos = scene.getCamera().getPosition();
                scene.getCamera().setPosition(pos - glm::vec3(0, 1, 0) * deltaTime * 10.0f);
            }

            // Handle resize via VulkanBase (it recreates swapchain internally)
            int fbWidth = vkBase.GetWidth();
            int fbHeight = vkBase.GetHeight();
            if (fbWidth > 0 && fbHeight > 0) {
                scene.getCamera().setAspect((float)fbWidth / (float)fbHeight);
            }

            // Begin Vulkan frame (acquires swapchain image, begins command buffer)
            if (!vkBase.BeginFrame()) {
                continue; // Swapchain out of date or minimized
            }

            VkCommandBuffer cmd = vkBase.GetCurrentCommandBuffer();

            // Begin frame profiling
            Profiler::instance().beginFrame();
            ui.beginFrame();

            // Update scene
            scene.update(deltaTime);

            // Execute render graph via scheduler (CPU + CUDA tasks)
            try {
                scheduler->executeFrame();
            } catch (const std::exception& e) {
                std::cerr << "Error in scheduler execution: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown error in scheduler execution" << std::endl;
            }

            // Record Vulkan commands for the deferred passes into the main command buffer.
            // The scheduler already ran CPU/CUDA tasks; now we record GPU draw commands
            // inside VulkanBase's render pass (which targets the swapchain framebuffer).
            {
                VkRenderPassBeginInfo rpInfo{};
                rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                rpInfo.renderPass = vkBase.GetRenderPass();
                rpInfo.framebuffer = vkBase.GetCurrentFramebuffer();
                rpInfo.renderArea.offset = {0, 0};
                rpInfo.renderArea.extent = vkBase.GetSwapchainExtent();

                VkClearValue clearValues[2]{};
                clearValues[0].color = {{0.1f, 0.1f, 0.2f, 1.0f}};
                clearValues[1].depthStencil = {1.0f, 0};
                rpInfo.clearValueCount = 2;
                rpInfo.pClearValues = clearValues;

                vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = 0.0f;
                viewport.width = static_cast<float>(vkBase.GetSwapchainExtent().width);
                viewport.height = static_cast<float>(vkBase.GetSwapchainExtent().height);
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.offset = {0, 0};
                scissor.extent = vkBase.GetSwapchainExtent();
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                // Record the deferred pass outputs into the swapchain render pass
                scene.recordPostFXPass(cmd);

                // Render ImGui on top
                ui.render(scheduler.get(), &controller, scene.getRenderGraph().get(),
                          Profiler::instance().getFrameTime(),
                          Profiler::instance().getFPS());
                ui.endFrame(cmd);

                vkCmdEndRenderPass(cmd);
            }

            // Update pipeline controller
            float frameTime = Profiler::instance().getFrameTime();
            float cpuTime = scheduler->getCPUTime();
            float gpuTime = scheduler->getGPUTime();
            float gpuUtil = std::min(1.0f, gpuTime / std::max(frameTime, 0.001f));
            controller.updateFrame(cpuTime, gpuTime, frameTime, gpuUtil);

            // End frame profiling
            Profiler::instance().endFrame();

            // Submit and present
            vkBase.EndFrame();
        }

        vkDeviceWaitIdle(vkBase.GetDevice());
        ui.shutdown();
        vkBase.Shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwTerminate();
    return 0;
}
