#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

#include "../src/gl_utils.hpp"
#include "../src/renderer.hpp"
#include "../src/cuda/sim.cuh"

struct Stats {
    double cudaTime = 0, cpuTime = 0, uploadTime = 0, renderTime = 0, totalTime = 0;
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

static void cudaCheck(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

static void buildViewProj(float* m, float aspect, float t) {
    float scale = 5.0f;
    m[0] = 1.0f/scale; m[1] = 0; m[2] = 0; m[3] = 0;
    m[4] = 0; m[5] = 1.0f/scale; m[6] = 0; m[7] = 0;
    m[8] = 0; m[9] = 0; m[10] = -0.01f; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
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
    file << "  \"avg_cpu_time_ms\": " << (stats.cpuTime / stats.frames) << ",\n";
    file << "  \"avg_upload_time_ms\": " << (stats.uploadTime / stats.frames) << ",\n";
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
        
        if(!glfwInit()) throw std::runtime_error("Failed to init GLFW");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Headless mode

        GLFWwindow* win = glfwCreateWindow(1280, 720, "CUDA Particles RL", nullptr, nullptr);
        if(!win) throw std::runtime_error("Failed to create window");
        glfwMakeContextCurrent(win);
        glfwSwapInterval(0);

        if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to load GL");

        cudaSetDevice(0);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        
        Renderer renderer;
        renderer.init(params.particleCount);

        float4 *dPos, *dVel, *dCol;
        cudaCheck(cudaMalloc(&dPos, params.particleCount * sizeof(float4)), "alloc pos");
        cudaCheck(cudaMalloc(&dVel, params.particleCount * sizeof(float4)), "alloc vel");
        cudaCheck(cudaMalloc(&dCol, params.particleCount * sizeof(float4)), "alloc col");

        std::vector<float4> hPos(params.particleCount), hCol(params.particleCount);

        SimParams simParams;
        simParams.dt = 0.016f;
        simParams.frame = 0;

        launchInit(dPos, dVel, dCol, params.particleCount, simParams, 0, params.blockSize);
        cudaDeviceSynchronize();

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);

        GLint uViewProj = glGetUniformLocation(renderer.program, "uViewProj");

        Stats stats;
        double start = glfwGetTime();
        
        while(!glfwWindowShouldClose(win) && stats.frames < params.numFrames) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            glfwPollEvents();
            int w = 1280, h = 720;
            glViewport(0,0,w,h);
            glClear(GL_COLOR_BUFFER_BIT);

            auto t1 = std::chrono::high_resolution_clock::now();
            simParams.frame++;
            
            // Use custom block size
            launchStep(dPos, dVel, dCol, params.particleCount, simParams, 0, params.blockSize);
            cudaDeviceSynchronize();
            auto t2 = std::chrono::high_resolution_clock::now();

            cudaMemcpy(hPos.data(), dPos, params.particleCount * sizeof(float4), cudaMemcpyDeviceToHost);
            cudaMemcpy(hCol.data(), dCol, params.particleCount * sizeof(float4), cudaMemcpyDeviceToHost);
            auto t3 = std::chrono::high_resolution_clock::now();

            glBindBuffer(GL_ARRAY_BUFFER, renderer.vboPositions);
            glBufferSubData(GL_ARRAY_BUFFER, 0, params.particleCount * sizeof(float4), hPos.data());
            glBindBuffer(GL_ARRAY_BUFFER, renderer.vboColors);
            glBufferSubData(GL_ARRAY_BUFFER, 0, params.particleCount * sizeof(float4), hCol.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            auto t4 = std::chrono::high_resolution_clock::now();

            float t = (float)(glfwGetTime() - start);
            float vp[16]; buildViewProj(vp, (float)w/(float)h, t);
            glUseProgram(renderer.program);
            glUniformMatrix4fv(uViewProj, 1, GL_FALSE, vp);
            
            renderer.draw(params.particleCount);
            
            auto t5 = std::chrono::high_resolution_clock::now();

            glfwSwapBuffers(win);
            auto frameEnd = std::chrono::high_resolution_clock::now();

            stats.cudaTime += std::chrono::duration<double, std::milli>(t2-t1).count();
            stats.cpuTime += std::chrono::duration<double, std::milli>(t3-t2).count();
            stats.uploadTime += std::chrono::duration<double, std::milli>(t4-t3).count();
            stats.renderTime += std::chrono::duration<double, std::milli>(t5-t4).count();
            stats.totalTime += std::chrono::duration<double, std::milli>(frameEnd-frameStart).count();
            stats.frames++;
        }

        stats.avgFPS = stats.frames / (stats.totalTime/1000.0);
        stats.avgFrameTime = stats.totalTime / stats.frames;
        
        writeMetricsJSON(stats, params, params.outputFile);

        cudaFree(dPos);
        cudaFree(dVel);
        cudaFree(dCol);
        renderer.shutdown();
        glfwDestroyWindow(win);
        glfwTerminate();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}

