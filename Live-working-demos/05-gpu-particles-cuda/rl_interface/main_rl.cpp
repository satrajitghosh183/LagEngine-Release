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
#include <fstream>
#include <sstream>
#include <cstring>

#include "../src/renderer.hpp"
#include "../src/cuda/sim.cuh"

struct Stats {
    double cudaTime = 0, renderTime = 0, totalTime = 0;
    int frames = 0;
    double avgFPS = 0;
    double avgFrameTime = 0;
};

struct RLParams {
    size_t particleCount = 100000;
    int blockSize = 256;
    int numFrames = 100;
    std::string outputFile = "metrics.json";
};

static void cudaCheckRL(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

static void buildViewProj(float* m, float aspect, float t) {
    float scale = 5.0f;
    std::memset(m, 0, sizeof(float) * 16);
    m[0]  = 1.0f / scale;
    m[5]  = 1.0f / scale;
    m[10] = -0.01f;
    m[15] = 1.0f;
}

void writeMetricsJSON(const Stats& stats, const RLParams& params, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open " << filename << " for writing\n";
        return;
    }

    file << std::fixed << std::setprecision(4);
    file << "{\n";
    file << "  \"particle_count\": " << params.particleCount << ",\n";
    file << "  \"block_size\": " << params.blockSize << ",\n";
    file << "  \"frames\": " << stats.frames << ",\n";
    file << "  \"avg_fps\": " << stats.avgFPS << ",\n";
    file << "  \"avg_frame_time_ms\": " << stats.avgFrameTime << ",\n";
    file << "  \"avg_cuda_time_ms\": " << (stats.cudaTime / stats.frames) << ",\n";
    file << "  \"avg_render_time_ms\": " << (stats.renderTime / stats.frames) << ",\n";
    file << "  \"total_time_ms\": " << stats.totalTime << "\n";
    file << "}\n";
    file.close();
}

RLParams parseArgs(int argc, char** argv) {
    RLParams params;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--particles" && i + 1 < argc) {
            params.particleCount = std::stoull(argv[++i]);
        } else if (arg == "--blocksize" && i + 1 < argc) {
            params.blockSize = std::stoi(argv[++i]);
        } else if (arg == "--frames" && i + 1 < argc) {
            params.numFrames = std::stoi(argv[++i]);
        } else if (arg == "--output" && i + 1 < argc) {
            params.outputFile = argv[++i];
        }
    }
    return params;
}

int main(int argc, char** argv) {
    try {
        RLParams params = parseArgs(argc, argv);

        // ── Init CUDA ──
        cudaSetDevice(0);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);

        // ── Init Vulkan (headless-ish: window hidden) ──
        vkdemo::VulkanBase vkBase;
        vkdemo::AppConfig config;
        config.Title  = "CUDA Particles RL (Vulkan)";
        config.Width  = 1280;
        config.Height = 720;
        config.EnableValidation = false;
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

        // Hide window for headless benchmarking
        glfwHideWindow(vkBase.GetWindow());

        // ── Init renderer ──
        Renderer renderer;
        renderer.init(vkBase, params.particleCount);

        // ── CUDA simulation buffers ──
        float4* dVel = nullptr;
        cudaCheckRL(cudaMalloc(&dVel, params.particleCount * sizeof(float4)), "alloc vel");

        float4* dPos = renderer.getCudaPosPtr();
        float4* dCol = renderer.getCudaColPtr();

        SimParams simParams;
        simParams.dt    = 0.016f;
        simParams.frame = 0;

        launchInit(dPos, dVel, dCol, params.particleCount, simParams, 0, params.blockSize);
        cudaDeviceSynchronize();

        Stats stats;
        double start = glfwGetTime();

        while (!glfwWindowShouldClose(vkBase.GetWindow()) && stats.frames < params.numFrames) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            if (!vkBase.BeginFrame()) break;

            // ── CUDA step ──
            auto t1 = std::chrono::high_resolution_clock::now();
            simParams.frame++;
            launchStep(dPos, dVel, dCol, params.particleCount, simParams, 0, params.blockSize);
            cudaDeviceSynchronize();
            auto t2 = std::chrono::high_resolution_clock::now();

            // ── Vulkan draw ──
            VkCommandBuffer cmd = vkBase.GetCurrentCommandBuffer();
            float vp[16];
            float t = static_cast<float>(glfwGetTime() - start);
            buildViewProj(vp, 1280.0f / 720.0f, t);

            renderer.draw(cmd, params.particleCount, vp);
            auto t3 = std::chrono::high_resolution_clock::now();

            vkBase.EndFrame();
            auto frameEnd = std::chrono::high_resolution_clock::now();

            stats.cudaTime   += std::chrono::duration<double, std::milli>(t2 - t1).count();
            stats.renderTime += std::chrono::duration<double, std::milli>(t3 - t2).count();
            stats.totalTime  += std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
            stats.frames++;
        }

        stats.avgFPS       = stats.frames / (stats.totalTime / 1000.0);
        stats.avgFrameTime = stats.totalTime / stats.frames;

        writeMetricsJSON(stats, params, params.outputFile);

        cudaFree(dVel);
        renderer.shutdown();
        vkBase.Shutdown();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}
