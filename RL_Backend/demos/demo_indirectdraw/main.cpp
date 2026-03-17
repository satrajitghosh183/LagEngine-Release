/**
 * RL_Backend - Indirect Draw
 * Workers build visibility/instance buffers; render thread does MultiDrawIndirect
 * Shows CPU prep scaling with thread count
 */

#include "rldemo/Window.hpp"
#include "rldemo/Shader.hpp"
#include "rldemo/JobSystem.hpp"
#include "rldemo/IndirectDrawBuilder.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <memory>
#include <random>

using namespace rldemo;

// Unit cube vertices (position only) + indices
static const float kCubeVertices[] = {
    -0.5f,-0.5f,-0.5f, 0.5f,-0.5f,-0.5f, 0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f,
    -0.5f,-0.5f, 0.5f, 0.5f,-0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
};
static const uint32_t kCubeIndices[] = {
    0,1,2, 0,2,3, 4,5,6, 4,6,7, 0,4,5, 0,5,1,
    2,6,7, 2,7,3, 0,4,7, 0,7,3, 1,5,6, 1,6,2
};

int main() {
    spdlog::set_level(spdlog::level::info);
    uint32_t workerCount = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
    JobSystem::Initialize(workerCount);

    auto window = CreateRef<Window>("RL_Backend - Indirect Draw", 1280, 720);
    if (!window->GetNative()) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    const char* vs = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in mat4 aModel;
        layout(location=5) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec4 vColor;
        void main() {
            vColor = aColor;
            gl_Position = uViewProj * aModel * vec4(aPos, 1.0);
        }
    )";
    const char* fs = R"(
        #version 330 core
        in vec4 vColor;
        out vec4 FragColor;
        void main() { FragColor = vColor; }
    )";
    Shader shader(vs, fs);
    if (!shader.GetId()) return 1;

    const uint32_t kMaxCubes = 100000;
    auto builder = CreateRef<IndirectDrawBuilder>(kMaxCubes);

    uint32_t vao, vbo, ebo, instanceVBO;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVertices), kCubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kCubeIndices), kCubeIndices, GL_STATIC_DRAW);

    // Instance attributes (will be bound from builder's VBO each frame)
    glBindBuffer(GL_ARRAY_BUFFER, builder->GetInstanceVBO());
    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(i * 4 * sizeof(float)));
        glVertexAttribDivisor(1 + i, 1);
        glEnableVertexAttribArray(1 + i);
    }
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(16 * sizeof(float)));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(5);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    int cubeCount = 10000;
    float camAngle = 0.f;
    double lastTime = glfwGetTime();
    float frameTimeMs = 0.f;
    float cpuPrepMs = 0.f;
    std::mt19937 rng(42);

    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 50), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(45.f), window->GetAspect(), 0.1f, 1000.f);

    while (!window->ShouldClose()) {
        double now = glfwGetTime();
        frameTimeMs = static_cast<float>((now - lastTime) * 1000.0);
        lastTime = now;

        builder->BeginFrame();

        auto prepStart = std::chrono::high_resolution_clock::now();
        // Distribute work across workers
        const int batchSize = std::max(1, cubeCount / static_cast<int>(workerCount));
        std::vector<JobHandle> jobs;
        for (uint32_t w = 0; w < workerCount; ++w) {
            int start = w * batchSize;
            int end = (w == workerCount - 1) ? cubeCount : (w + 1) * batchSize;
            auto h = JobSystem::Enqueue([&builder, start, end, camAngle]() {
                std::mt19937 rng(start + 12345);
                std::uniform_real_distribution<float> dist(-50.f, 50.f);
                for (int i = start; i < end; ++i) {
                    float x = dist(rng), y = dist(rng), z = dist(rng);
                    glm::mat4 m = glm::translate(glm::mat4(1), glm::vec3(x, y, z));
                    m = glm::rotate(m, camAngle, glm::vec3(0, 1, 0));
                    glm::vec4 c((i % 100) / 100.f, 0.5f, 0.8f, 1.f);
                    builder->AddInstance(m, c);
                }
            }, {});
            jobs.push_back(h);
        }
        JobSystem::WaitForAll();
        auto prepEnd = std::chrono::high_resolution_clock::now();
        cpuPrepMs = std::chrono::duration<float, std::milli>(prepEnd - prepStart).count();

        uint32_t drawCount = builder->EndFrame();

        view = glm::lookAt(glm::vec3(50 * sinf(camAngle), 10, 50 * cosf(camAngle)), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
        glm::mat4 vp = proj * view;

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shader.Bind();
        shader.SetMat4("uViewProj", &vp[0][0]);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, builder->GetInstanceVBO());
        for (int i = 0; i < 4; ++i) {
            glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(i * 4 * sizeof(float)));
            glVertexAttribDivisor(1 + i, 1);
        }
        glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(16 * sizeof(float)));
        glVertexAttribDivisor(5, 1);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, drawCount);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::Begin("Indirect Draw", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Frame: %.2f ms | FPS: %.0f", frameTimeMs, 1000.f / (frameTimeMs > 0 ? frameTimeMs : 1));
        ImGui::Text("CPU prep: %.2f ms", cpuPrepMs);
        ImGui::Text("Draw calls: %u | Cubes: %d", drawCount, cubeCount);
        ImGui::Text("Workers: %u", workerCount);
        auto stats = JobSystem::GetStats();
        ImGui::Text("Jobs: pushes=%llu steals=%llu", (unsigned long long)stats.pushes, (unsigned long long)stats.steals);
        ImGui::SliderInt("Cubes", &cubeCount, 100, 100000);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        camAngle += 0.005f;
        window->PollEvents();
        window->SwapBuffers();
    }

    JobSystem::Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    return 0;
}
