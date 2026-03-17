/**
 * RL_Backend - Stage 1: Slow single-threaded physics + graphics.
 * All physics (gravity, collision, integration) and instance build on main thread;
 * only GL draw is separate. Demonstrates baseline slowness before parallelization.
 */

#include "rldemo/Window.hpp"
#include "rldemo/Shader.hpp"
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
#include <cstdio>
#include <deque>

using namespace rldemo;

static const float kCubeVertices[] = {
    // front
    -0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,
    // back
    -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,
};
static const uint32_t kCubeIndices[] = {
    0,1,2, 0,2,3,   // front
    5,4,7, 5,7,6,   // back
    4,0,3, 4,3,7,   // left
    1,5,6, 1,6,2,   // right
    3,2,6, 3,6,7,   // top
    4,5,1, 4,1,0,   // bottom
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

    auto window = CreateRef<Window>("RL_Backend - Physics Slow (Stage 1)", 1280, 720);
    if (!window->GetNative()) return 1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader(kVertSrc, kFragSrc);
    if (!shader.GetId()) return 1;

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
    float gravityMs = 0.f, collisionMs = 0.f, integrationMs = 0.f, buildMs = 0.f, drawMs = 0.f;

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

        auto t0 = std::chrono::high_resolution_clock::now();
        sim.ComputeGravityRange(0, bodyCount, sim.GetBodies());
        auto t1 = std::chrono::high_resolution_clock::now();
        sim.DetectAndResolveCollisionsRange(0, bodyCount, sim.GetBodies());
        auto t2 = std::chrono::high_resolution_clock::now();
        sim.IntegrateRange(0, bodyCount, sim.GetBodies(), dt);
        auto t3 = std::chrono::high_resolution_clock::now();

        builder->BeginFrame();
        const auto& bodies = sim.GetBodies();
        for (int i = 0; i < bodyCount && i < static_cast<int>(bodies.size()); ++i) {
            const auto& b = bodies[i];
            glm::mat4 model = glm::translate(glm::mat4(1.f), b.pos);
            model = glm::scale(model, glm::vec3(b.radius * 2.f));
            float hue = (i % 100) / 100.f;
            builder->AddInstance(model, glm::vec4(hue, 0.7f, 0.9f, 1.f));
        }
        auto t4 = std::chrono::high_resolution_clock::now();

        gravityMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        collisionMs = std::chrono::duration<float, std::milli>(t2 - t1).count();
        integrationMs = std::chrono::duration<float, std::milli>(t3 - t2).count();
        buildMs = std::chrono::duration<float, std::milli>(t4 - t3).count();

        uint32_t drawCount = builder->EndFrame();

        glm::mat4 proj = glm::perspective(glm::radians(45.f), window->GetAspect(), 0.1f, 500.f);
        glm::mat4 view = glm::lookAt(
            glm::vec3(camDist * sinf(camAngle), camHeight, camDist * cosf(camAngle)),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0));
        glm::mat4 vp = proj * view;

        auto t5 = std::chrono::high_resolution_clock::now();
        glClearColor(0.02f, 0.02f, 0.04f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        shader.Bind();
        shader.SetMat4("uViewProj", &vp[0][0]);
        glBindVertexArray(vao);
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr, drawCount);
        glBindVertexArray(0);
        auto t6 = std::chrono::high_resolution_clock::now();
        drawMs = std::chrono::duration<float, std::milli>(t6 - t5).count();

        float totalPhysicsMs = gravityMs + collisionMs + integrationMs;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(340, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Stage 1 - Single-Threaded Physics", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "SINGLE THREADED - NO PARALLELIZATION");
        ImGui::Separator();

        float fps = (frameTimeMs > 0.f) ? (1000.f / frameTimeMs) : 0.f;
        ImGui::Text("Frame: %.2f ms  |  FPS: %.0f", frameTimeMs, fps);
        ImGui::Text("Bodies: %d  |  Draw count: %u", bodyCount, drawCount);
        ImGui::Separator();

        ImGui::TextColored(ImVec4(1.f, 0.8f, 0.3f, 1.f), "Physics Breakdown:");
        ImGui::Text("  Gravity  (O(n^2)):  %7.2f ms", gravityMs);
        ImGui::Text("  Collision (O(n^2)): %7.2f ms", collisionMs);
        ImGui::Text("  Integration:        %7.2f ms", integrationMs);
        ImGui::Text("  Instance build:     %7.2f ms", buildMs);
        ImGui::Text("  GL draw (serial):   %7.2f ms", drawMs);
        ImGui::Separator();

        char totalBuf[32];
        snprintf(totalBuf, sizeof(totalBuf), "Total physics: %.1f ms", totalPhysicsMs);
        float fraction = std::min(totalPhysicsMs / 50.f, 1.f);
        ImGui::ProgressBar(fraction, ImVec2(-1, 0), totalBuf);
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
            ImGui::PlotLines("##ftHist", vals, static_cast<int>(count), 0, nullptr, 0.f, 100.f, ImVec2(-1, 80));
        }

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        camAngle += 0.003f;
        window->PollEvents();
        window->SwapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &ebo);
    return 0;
}
