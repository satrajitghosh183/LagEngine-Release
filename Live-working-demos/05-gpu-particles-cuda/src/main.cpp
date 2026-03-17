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

#include "gl_utils.hpp"
#include "renderer.hpp"
#include "cuda/sim.cuh"

struct Stats {
    double cudaTime = 0, cpuTime = 0, uploadTime = 0, renderTime = 0, totalTime = 0;
    int frames = 0;
};

static void cudaCheck(cudaError_t err, const char* msg) {
    if (err != cudaSuccess) {
        throw std::runtime_error(std::string(msg) + ": " + cudaGetErrorString(err));
    }
}

static void buildViewProj(float* m, float aspect, float t) {
    // Super simple orthographic view
    float scale = 5.0f;
    m[0] = 1.0f/scale; m[1] = 0; m[2] = 0; m[3] = 0;
    m[4] = 0; m[5] = 1.0f/scale; m[6] = 0; m[7] = 0;
    m[8] = 0; m[9] = 0; m[10] = -0.01f; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

void printStats(const Stats& stats) {
    std::cout << "\n========== PERFORMANCE STATISTICS ==========\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Frames:     " << stats.frames << "\n";
    std::cout << "Avg FPS:          " << stats.frames / (stats.totalTime/1000.0) << "\n";
    std::cout << "Avg Frame Time:   " << stats.totalTime / stats.frames << " ms\n";
    std::cout << "  - CUDA Sim:     " << stats.cudaTime / stats.frames << " ms\n";
    std::cout << "  - CPU Copy:     " << stats.cpuTime / stats.frames << " ms\n";
    std::cout << "  - GL Upload:    " << stats.uploadTime / stats.frames << " ms\n";
    std::cout << "  - Rendering:    " << stats.renderTime / stats.frames << " ms\n";
    std::cout << "===========================================\n\n";
}

int main(int argc, char** argv) {
    try {
        bool benchmark = (argc > 1 && std::string(argv[1]) == "--benchmark");
        
        if(!glfwInit()) throw std::runtime_error("Failed to init GLFW");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);

        GLFWwindow* win = glfwCreateWindow(1280, 720, "CUDA Particles", nullptr, nullptr);
        if(!win) throw std::runtime_error("Failed to create window");
        glfwMakeContextCurrent(win);
        glfwSwapInterval(0);

        if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            throw std::runtime_error("Failed to load GL");

        cudaSetDevice(0);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        std::cout << "GPU: " << prop.name << "\n";
        
        const size_t N = 100000;
        std::cout << "Particles: " << N << "\n\n";
        
        Renderer renderer;
        renderer.init(N);

        float4 *dPos, *dVel, *dCol;
        cudaCheck(cudaMalloc(&dPos, N * sizeof(float4)), "alloc pos");
        cudaCheck(cudaMalloc(&dVel, N * sizeof(float4)), "alloc vel");
        cudaCheck(cudaMalloc(&dCol, N * sizeof(float4)), "alloc col");

        std::vector<float4> hPos(N), hCol(N);

        SimParams params;
        params.dt = 0.016f;
        params.frame = 0;

        launchInit(dPos, dVel, dCol, N, params);
        cudaDeviceSynchronize();

        // Check particles
        cudaMemcpy(hPos.data(), dPos, 10 * sizeof(float4), cudaMemcpyDeviceToHost);
        cudaMemcpy(hCol.data(), dCol, 10 * sizeof(float4), cudaMemcpyDeviceToHost);
        std::cout << "Sample particles (pos, color):\n";
        for(int i=0; i<5; i++) {
            std::cout << "  " << i << ": pos(" << hPos[i].x << ", " << hPos[i].y << ", " << hPos[i].z 
                     << ") col(" << hCol[i].x << ", " << hCol[i].y << ", " << hCol[i].z << ")\n";
        }
        std::cout << "\n";

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);  // Dark blue so we can see red

        GLint uViewProj = glGetUniformLocation(renderer.program, "uViewProj");
        std::cout << "Uniform location uViewProj: " << uViewProj << "\n\n";

        Stats stats;
        double lastPrint = glfwGetTime();
        double start = glfwGetTime();
        int frameCount = 0;
        
        std::cout << "Running... (ESC to exit)\n";
        std::cout << "You should see BRIGHT RED dots!\n\n";

        while(!glfwWindowShouldClose(win)) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            
            if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
            
            glfwPollEvents();
            int w, h; glfwGetFramebufferSize(win, &w, &h);
            glViewport(0,0,w,h);
            glClear(GL_COLOR_BUFFER_BIT);

            auto t1 = std::chrono::high_resolution_clock::now();
            params.frame++;
            launchStep(dPos, dVel, dCol, N, params);
            cudaDeviceSynchronize();
            auto t2 = std::chrono::high_resolution_clock::now();

            cudaMemcpy(hPos.data(), dPos, N * sizeof(float4), cudaMemcpyDeviceToHost);
            cudaMemcpy(hCol.data(), dCol, N * sizeof(float4), cudaMemcpyDeviceToHost);
            auto t3 = std::chrono::high_resolution_clock::now();

            glBindBuffer(GL_ARRAY_BUFFER, renderer.vboPositions);
            glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(float4), hPos.data());
            glBindBuffer(GL_ARRAY_BUFFER, renderer.vboColors);
            glBufferSubData(GL_ARRAY_BUFFER, 0, N * sizeof(float4), hCol.data());
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            auto t4 = std::chrono::high_resolution_clock::now();

            float t = (float)(glfwGetTime() - start);
            float vp[16]; buildViewProj(vp, (float)w/(float)h, t);
            glUseProgram(renderer.program);
            glUniformMatrix4fv(uViewProj, 1, GL_FALSE, vp);
            
            renderer.draw(N);
            
            GLenum err = glGetError();
            if (err != GL_NO_ERROR && stats.frames < 5) {
                std::cout << "GL Error: " << err << "\n";
            }
            
            auto t5 = std::chrono::high_resolution_clock::now();

            glfwSwapBuffers(win);
            auto frameEnd = std::chrono::high_resolution_clock::now();

            stats.cudaTime += std::chrono::duration<double, std::milli>(t2-t1).count();
            stats.cpuTime += std::chrono::duration<double, std::milli>(t3-t2).count();
            stats.uploadTime += std::chrono::duration<double, std::milli>(t4-t3).count();
            stats.renderTime += std::chrono::duration<double, std::milli>(t5-t4).count();
            stats.totalTime += std::chrono::duration<double, std::milli>(frameEnd-frameStart).count();
            stats.frames++;
            frameCount++;

            double now = glfwGetTime();
            if (now - lastPrint >= 1.0) {
                double fps = frameCount / (now - lastPrint);
                std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps 
                         << " | Sample pos: (" << hPos[0].x << "," << hPos[0].y << "," << hPos[0].z << ")   \r" << std::flush;
                lastPrint = now;
                frameCount = 0;
            }

            if (benchmark && stats.frames >= 500) break;
        }

        std::cout << "\n";
        printStats(stats);

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
