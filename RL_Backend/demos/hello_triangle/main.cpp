/**
 * RL_Backend - Hello Triangle
 * Minimal window + OpenGL + ImGui overlay with frame time
 */

#include "rldemo/Window.hpp"
#include "rldemo/Shader.hpp"
#include "rldemo/JobSystem.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <chrono>
#include <memory>

using namespace rldemo;

int main() {
    spdlog::set_level(spdlog::level::info);
    auto window = CreateRef<Window>("RL_Backend - Hello Triangle", 1280, 720);
    if (!window->GetNative()) return 1;

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window->GetNative(), true);
    ImGui_ImplOpenGL3_Init("#version 330");

    const char* vs = R"(
        #version 330 core
        layout(location=0) in vec3 aPos;
        layout(location=1) in vec3 aColor;
        out vec3 vColor;
        void main() {
            vColor = aColor;
            gl_Position = vec4(aPos, 1.0);
        }
    )";
    const char* fs = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() { FragColor = vec4(vColor, 1.0); }
    )";
    Shader shader(vs, fs);
    if (!shader.GetId()) return 1;

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    uint32_t vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    double lastTime = glfwGetTime();
    float frameTimeMs = 0.f;

    while (!window->ShouldClose()) {
        double now = glfwGetTime();
        frameTimeMs = static_cast<float>((now - lastTime) * 1000.0);
        lastTime = now;

        window->PollEvents();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Bind();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
        ImGui::Begin("RL_Backend", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Hello Triangle");
        ImGui::Text("Frame time: %.2f ms", frameTimeMs);
        ImGui::Text("FPS: %.0f", 1000.0f / (frameTimeMs > 0 ? frameTimeMs : 1));
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window->SwapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    return 0;
}
