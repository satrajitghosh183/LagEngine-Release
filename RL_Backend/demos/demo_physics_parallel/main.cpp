/**
 * RL_Backend - Stage 2: Parallelized physics, serial GL draw.
 * Gravity, collision, integration, and instance build run on JobSystem workers;
 * only glDrawElementsInstanced and swap run on the main thread.
 */

#include "rldemo/Window.hpp"
#include "rldemo/Shader.hpp"
#include "rldemo/JobSystem.hpp"
#include "rldemo/IndirectDrawBuilder.hpp"
#include "rldemo/NBodySim.hpp"
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
#include <cstring>
#include <cstdio>
#include <deque>

using namespace rldemo;

static const float kCubeVertices[] = {
    -0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,
};
static const uint32_t kCubeIndices[] = {
    0,1,2, 0,2,3,
    5,4,7, 5,7,6,
    4,0,3, 4,3,7,
    1,5,6, 1,6,2,
    3,2,6, 3,6,7,
    4,5,1, 4,1,0,
};

static constexpr size_t kFrameHistorySize = 120;

static const char* kVertSrc = R"(
    #version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in mat4 aModel;
    layout(location=5) in vec4 aColor;
    uniform mat4 uViewProj;
    out vec3 vWorldPos;
    out vec3 vNormal;
    out vec4 vColor;
    void main() {
        vec4 worldPos = aModel * vec4(aPos, 1.0);
        vWorldPos = worldPos.xyz;
        vNormal = normalize(mat3(aModel) * aPos);
        vColor = aColor;
        gl_Position = uViewProj * worldPos;
    }
)";

static const char* kFragSrc = R"(
    #version 330 core
    in vec3 vWorldPos;
    in vec3 vNormal;
    in vec4 vColor;
    out vec4 FragColor;

    vec3 hsv2rgb(float h, float s, float v) {
        vec3 c = vec3(h * 6.0, s, v);
        vec3 rgb = clamp(abs(mod(c.x + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
        return v * mix(vec3(1.0), rgb, s);
    }

    void main() {
        vec3 baseColor = hsv2rgb(vColor.r, vColor.g, vColor.b);
        vec3 N = normalize(vNormal);
        vec3 L = normalize(vec3(0.5, 1.0, 0.3));
        float diff = max(dot(N, L), 0.0) * 0.6 + 0.4;
        FragColor = vec4(baseColor * diff, 1.0);
    }
)";

int main() {
    spdlog::set_level(spdlog::level::info);
    uint32_t workerCount = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
    JobSystem::Initialize(workerCount);

    auto window = CreateRef<Window>("RL_Backend - Physics Parallel (Stage 2)", 1280, 720);
    if (!window->GetNative()) { JobSystem::Shutdown(); return 1; }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader(kVertSrc, kFragSrc);
    if (!shader.GetId()) { JobSystem::Shutdown(); return 1; }

    const uint32_t kMaxBodies = 10000;
    NBodySim sim(kMaxBodies);
    sim.Resize(2500);
    sim.RandomizePositionsAndVelocities(40.f, 1.f);

    auto builder = CreateRef<IndirectDrawBuilder>(kMaxBodies);

    uint32_t vao, vbo, ebo;
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

    glBindBuffer(GL_ARRAY_BUFFER, builder->GetInstanceVBO());
    for (int i = 0; i < 4; ++i) {
        glVertexAttribPointer(1 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              (void*)(static_cast<size_t>(i) * 4 * sizeof(float)));
        glEnableVertexAttribArray(1 + i);
        glVertexAttribDivisor(1 + i, 1);
    }
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*)(16 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int bodyCount = 2500;
    int prevBodyCount = bodyCount;
    float camAngle = 0.f;
    float camHeight = 20.f;
    float camDist = 80.f;
    double lastTime = glfwGetTime();
    float frameTimeMs = 0.f;
    std::deque<float> frameHistory;
    float gravityMs = 0.f, collisionMs = 0.f, integrateBuildMs = 0.f, drawMs = 0.f;

    while (!window->ShouldClose()) {
        double now = glfwGetTime();
        frameTimeMs = static_cast<float>((now - lastTime) * 1000.0);
        lastTime = now;
        if (frameHistory.size() >= kFrameHistorySize) frameHistory.pop_front();
        frameHistory.push_back(frameTimeMs);
        const float dt = 0.016f;

        if (bodyCount != prevBodyCount) {
            sim.Resize(static_cast<uint32_t>(bodyCount));
            prevBodyCount = bodyCount;
        }

        auto& bodies = sim.GetBodies();
        const int n = bodyCount;

        builder->BeginFrame();
        builder->PrepareForInstances(static_cast<uint32_t>(n));
        InstanceData* instanceData = builder->GetInstanceStagingData();
        if (!instanceData) { window->PollEvents(); window->SwapBuffers(); continue; }

        JobSystem::ResetStats();

        auto t0 = std::chrono::high_resolution_clock::now();
        const int batchG = std::max(1, n / static_cast<int>(workerCount));
        for (uint32_t w = 0; w < workerCount; ++w) {
            int start = w * batchG;
            int end = (w == workerCount - 1) ? n : (w + 1) * batchG;
            if (start >= end) continue;
            JobSystem::Enqueue([&sim, &bodies, start, end]() {
                sim.ComputeGravityRange(start, end, bodies);
            }, {});
        }
        JobSystem::WaitForAll();
        auto t1 = std::chrono::high_resolution_clock::now();

        const int batchC = std::max(1, n / static_cast<int>(workerCount));
        for (uint32_t w = 0; w < workerCount; ++w) {
            int start = w * batchC;
            int end = (w == workerCount - 1) ? n : (w + 1) * batchC;
            if (start >= end) continue;
            JobSystem::Enqueue([&sim, &bodies, start, end]() {
                sim.DetectAndResolveCollisionsRange(start, end, bodies);
            }, {});
        }
        JobSystem::WaitForAll();
        auto t2 = std::chrono::high_resolution_clock::now();

        const int batchI = std::max(1, n / static_cast<int>(workerCount));
        for (uint32_t w = 0; w < workerCount; ++w) {
            int start = w * batchI;
            int end = (w == workerCount - 1) ? n : (w + 1) * batchI;
            if (start >= end) continue;
            JobSystem::Enqueue([&bodies, instanceData, start, end, dt]() {
                for (int i = start; i < end; ++i) {
                    auto& b = bodies[i];
                    b.vel += b.acc * dt;
                    b.pos += b.vel * dt;
                    glm::mat4 model = glm::translate(glm::mat4(1.f), b.pos);
                    model = glm::scale(model, glm::vec3(b.radius * 2.f));
                    memcpy(instanceData[i].model, &model[0][0], 16 * sizeof(float));
                    float hue = (i % 100) / 100.f;
                    instanceData[i].color[0] = hue;
                    instanceData[i].color[1] = 0.7f;
                    instanceData[i].color[2] = 0.9f;
                    instanceData[i].color[3] = 1.f;
                }
            }, {});
        }
        JobSystem::WaitForAll();
        auto t3 = std::chrono::high_resolution_clock::now();

        gravityMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        collisionMs = std::chrono::duration<float, std::milli>(t2 - t1).count();
        integrateBuildMs = std::chrono::duration<float, std::milli>(t3 - t2).count();

        uint32_t drawCount = builder->EndFrame();
        auto stats = JobSystem::GetStats();
        auto workerStats = JobSystem::GetWorkerStats();

        glm::mat4 proj = glm::perspective(glm::radians(45.f), window->GetAspect(), 0.1f, 500.f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(camDist * sinf(camAngle), camHeight, camDist * cosf(camAngle)),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0));
        glm::mat4 vp = proj * view;

        auto t4 = std::chrono::high_resolution_clock::now();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shader.Bind();
        shader.SetMat4("uViewProj", &vp[0][0]);
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, drawCount);
        glBindVertexArray(0);
        auto t5 = std::chrono::high_resolution_clock::now();
        drawMs = std::chrono::duration<float, std::milli>(t5 - t4).count();

        float totalPhysicsMs = gravityMs + collisionMs + integrateBuildMs;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(370, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Stage 2 - Parallel Physics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextColored(ImVec4(0.4f, 1.f, 0.4f, 1.f), "PARALLELIZED - %u worker threads", workerCount);
        ImGui::Separator();

        float fps = (frameTimeMs > 0.f) ? (1000.f / frameTimeMs) : 0.f;
        ImGui::Text("Frame: %.2f ms  |  FPS: %.0f", frameTimeMs, fps);
        ImGui::Text("Bodies: %d  |  Draw count: %u", bodyCount, drawCount);
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.f, 1.f), "Physics Breakdown (parallel):");
        ImGui::Text("  Gravity  (O(n^2)):  %7.2f ms", gravityMs);
        ImGui::Text("  Collision (O(n^2)): %7.2f ms", collisionMs);
        ImGui::Text("  Integ+Build:        %7.2f ms", integrateBuildMs);
        ImGui::Text("  GL draw (serial):   %7.2f ms", drawMs);
        ImGui::Separator();

        char totalBuf[32];
        snprintf(totalBuf, sizeof(totalBuf), "Total physics: %.1f ms", totalPhysicsMs);
        float fraction = std::min(totalPhysicsMs / 50.f, 1.f);
        ImGui::ProgressBar(fraction, ImVec2(-1, 0), totalBuf);
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "JobSystem Stats:");
        ImGui::Text("  Pushes: %llu  Steals: %llu  Attempts: %llu",
                    (unsigned long long)stats.pushes,
                    (unsigned long long)stats.steals,
                    (unsigned long long)stats.stealAttempts);
        ImGui::Separator();

        ImGui::Text("Per-worker utilization:");
        for (size_t w = 0; w < workerStats.size(); ++w) {
            const auto& ws = workerStats[w];
            uint64_t total = ws.totalTimeNs + ws.idleNs;
            float execFrac = (total > 0) ? (static_cast<float>(ws.totalTimeNs) / static_cast<float>(total)) : 0.f;
            char overlay[32];
            snprintf(overlay, sizeof(overlay), "W%zu: %.0f%%", w, execFrac * 100.f);
            ImGui::ProgressBar(execFrac, ImVec2(-1, 0), overlay);
        }
        ImGui::Separator();

        ImGui::SliderInt("Bodies", &bodyCount, 100, 8000);
        ImGui::SliderFloat("Cam Height", &camHeight, -30.f, 60.f);
        ImGui::SliderFloat("Cam Distance", &camDist, 20.f, 200.f);

        if (frameHistory.size() >= 2) {
            ImGui::Separator();
            ImGui::Text("Frame time history:");
            float vals[kFrameHistorySize];
            size_t count = frameHistory.size();
            for (size_t i = 0; i < count; ++i) vals[i] = frameHistory[i];
            ImGui::PlotLines("##ftHist", vals, static_cast<int>(count), 0, nullptr, 0.f, 50.f, ImVec2(-1, 80));
        }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        camAngle += 0.003f;
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
