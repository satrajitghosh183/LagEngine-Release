#include <VulkanBase.hpp>

#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

#include <cuda_runtime.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <array>
#include <chrono>
#include <iomanip>
#include <cstring>

#include "renderer.hpp"
#include "cuda/sim.cuh"

struct Stats {
    double cudaTime = 0, renderTime = 0, totalTime = 0;
    int frames = 0;
};

static void cudaCheckMain(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

static void buildViewProj(float* m, float aspect, float t) {
    // Simple orthographic view
    float scale = 5.0f;
    std::memset(m, 0, sizeof(float) * 16);
    m[0]  = 1.0f / scale;
    m[5]  = 1.0f / scale;
    m[10] = -0.01f;
    m[15] = 1.0f;
}

void printStats(const Stats& stats) {
    std::cout << "\n========== PERFORMANCE STATISTICS ==========\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Frames:     " << stats.frames << "\n";
    std::cout << "Avg FPS:          " << stats.frames / (stats.totalTime / 1000.0) << "\n";
    std::cout << "Avg Frame Time:   " << stats.totalTime / stats.frames << " ms\n";
    std::cout << "  - CUDA Sim:     " << stats.cudaTime / stats.frames << " ms\n";
    std::cout << "  - Rendering:    " << stats.renderTime / stats.frames << " ms\n";
    std::cout << "===========================================\n\n";
}

int main(int argc, char** argv) {
    try {
        bool benchmark = (argc > 1 && std::string(argv[1]) == "--benchmark");

        // ── Init CUDA ──
        cudaSetDevice(0);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        std::cout << "GPU: " << prop.name << "\n";

        const size_t N = 100000;
        std::cout << "Particles: " << N << "\n\n";

        // ── Init Vulkan via VulkanBase ──
        vkdemo::VulkanBase vkBase;
        vkdemo::AppConfig config;
        config.Title  = "CUDA Particles (Vulkan)";
        config.Width  = 1280;
        config.Height = 720;
        config.EnableValidation = true;
        config.EnableImGui      = false;

        // Request external memory extensions for CUDA-Vulkan interop
        config.AdditionalDeviceExtensions = {
            VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
#ifdef _WIN32
            VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME,
#else
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
#endif
        };
        config.AdditionalInstanceExtensions = {
            VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        };

        vkBase.Init(config);

        // ESC to close
        vkBase.OnKey = [](GLFWwindow* win, int key, int, int action, int) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
                glfwSetWindowShouldClose(win, GLFW_TRUE);
        };

        // ── Init renderer (creates pipeline + shared buffers) ──
        Renderer renderer;
        renderer.init(vkBase, N);

        // ── Init CUDA simulation using the shared buffers ──
        // Allocate velocity on CUDA side only (not shared with Vulkan)
        float4* dVel = nullptr;
        cudaCheckMain(cudaMalloc(&dVel, N * sizeof(float4)), "alloc vel");

        SimParams params;
        params.dt    = 0.016f;
        params.frame = 0;

        // Get CUDA device pointers from the interop buffers
        float4* dPos = renderer.getCudaPosPtr();
        float4* dCol = renderer.getCudaColPtr();

        launchInit(dPos, dVel, dCol, N, params);
        cudaDeviceSynchronize();

        // Verify particles
        std::vector<float4> hPos(10), hCol(10);
        cudaMemcpy(hPos.data(), dPos, 10 * sizeof(float4), cudaMemcpyDeviceToHost);
        cudaMemcpy(hCol.data(), dCol, 10 * sizeof(float4), cudaMemcpyDeviceToHost);
        std::cout << "Sample particles (pos, color):\n";
        for (int i = 0; i < 5; i++) {
            std::cout << "  " << i << ": pos(" << hPos[i].x << ", " << hPos[i].y << ", " << hPos[i].z
                     << ") col(" << hCol[i].x << ", " << hCol[i].y << ", " << hCol[i].z << ")\n";
        }
        std::cout << "\n";

        Stats stats;
        double lastPrint = glfwGetTime();
        double start     = glfwGetTime();
        int frameCount   = 0;

        std::cout << "Running... (ESC to exit)\n";
        std::cout << "You should see BRIGHT RED dots!\n\n";

        while (true) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            // BeginFrame handles glfwPollEvents, acquires swapchain image,
            // begins command buffer and render pass
            if (!vkBase.BeginFrame()) break;

            // ── CUDA simulation step ──
            auto t1 = std::chrono::high_resolution_clock::now();
            params.frame++;
            launchStep(dPos, dVel, dCol, N, params);
            cudaDeviceSynchronize();
            auto t2 = std::chrono::high_resolution_clock::now();

            // No CPU copy or upload needed -- CUDA writes directly
            // to the shared Vulkan vertex buffer via interop.

            // ── Vulkan draw ──
            VkCommandBuffer cmd = vkBase.GetCurrentCommandBuffer();
            float vp[16];
            float t = static_cast<float>(glfwGetTime() - start);
            float aspect = static_cast<float>(vkBase.GetWidth()) /
                           static_cast<float>(vkBase.GetHeight());
            buildViewProj(vp, aspect, t);

            renderer.draw(cmd, N, vp);

            auto t3 = std::chrono::high_resolution_clock::now();

            // EndFrame ends render pass, submits, presents
            vkBase.EndFrame();

            auto frameEnd = std::chrono::high_resolution_clock::now();

            stats.cudaTime   += std::chrono::duration<double, std::milli>(t2 - t1).count();
            stats.renderTime += std::chrono::duration<double, std::milli>(t3 - t2).count();
            stats.totalTime  += std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
            stats.frames++;
            frameCount++;

            double now = glfwGetTime();
            if (now - lastPrint >= 1.0) {
                double fps = frameCount / (now - lastPrint);

                // Quick readback for diagnostics (first particle only)
                float4 samplePos;
                cudaMemcpy(&samplePos, dPos, sizeof(float4), cudaMemcpyDeviceToHost);

                std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps
                         << " | Sample pos: (" << samplePos.x << ","
                         << samplePos.y << "," << samplePos.z << ")   \r" << std::flush;
                lastPrint  = now;
                frameCount = 0;
            }

            if (benchmark && stats.frames >= 500) break;
        }

        std::cout << "\n";
        printStats(stats);

        // ── Cleanup ──
        cudaFree(dVel);
        renderer.shutdown();
        vkBase.Shutdown();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
