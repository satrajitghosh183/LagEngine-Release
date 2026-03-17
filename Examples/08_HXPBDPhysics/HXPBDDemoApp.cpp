#include "HXPBDDemoApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

GameEngine::Application* GameEngine::CreateApplication() {
    return new HXPBDDemoApp();
}

HXPBDDemoApp::HXPBDDemoApp()
    : Application("HXPBD Cloth Physics (from old_code/feather)")
{
    // Pin top edge — mirrors feather main.cpp cloth->addFixedConstraint for top row
    m_Cloth.pinTopEdge();

    // Gravity — matches feather main.cpp "gravity" field value
    m_Cloth.accelerations.push_back(m_Gravity);
}

void HXPBDDemoApp::OnInit() {
    const char* vert = R"(
        #version 330 core
        layout(location=0) in vec3 a_Pos;
        uniform mat4 u_VP;
        uniform vec3 u_Color;
        out vec3 v_Color;
        void main() {
            v_Color = u_Color;
            gl_Position = u_VP * vec4(a_Pos, 1.0);
        }
    )";
    const char* frag = R"(
        #version 330 core
        in vec3 v_Color;
        out vec4 FragColor;
        void main() { FragColor = vec4(v_Color, 1.0); }
    )";
    m_Shader = CreateRef<Shader>(vert, frag);

    // Build static triangle index buffer from cloth grid
    int W = m_Cloth.gridW, H = m_Cloth.gridH;
    for (int j = 0; j < H - 1; ++j) {
        for (int i = 0; i < W - 1; ++i) {
            unsigned int k = j * W + i;
            m_Indices.push_back(k);
            m_Indices.push_back(k + 1);
            m_Indices.push_back(k + W);
            m_Indices.push_back(k + 1);
            m_Indices.push_back(k + W + 1);
            m_Indices.push_back(k + W);
        }
    }

    // Allocate GPU buffers
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 m_Cloth.particles.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 m_Indices.size() * sizeof(unsigned int), m_Indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);

    UploadMesh();
}

void HXPBDDemoApp::OnUpdate(float dt) {
    if (m_Paused) return;

    // Sync gravity/wind to solver each frame (UI may have changed)
    m_Cloth.accelerations.clear();
    m_Cloth.accelerations.push_back(m_Gravity);
    if (glm::length2(m_Wind) > 0.f)
        m_Cloth.accelerations.push_back(m_Wind);

    m_Cloth.solve(dt * m_TimeScale);
    UploadMesh();
}

void HXPBDDemoApp::OnRender() {
    // Orbit camera
    float cy = std::cos(m_CamYaw),   sy = std::sin(m_CamYaw);
    float cp = std::cos(m_CamPitch), sp = std::sin(m_CamPitch);
    glm::vec3 eye = glm::vec3(m_CamDist * cy * cp,
                              m_CamDist * sp,
                              m_CamDist * sy * cp);
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0, 5, 0), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 200.0f);
    glm::mat4 vp   = proj * view;

    m_Shader->Bind();
    m_Shader->SetUniformMat4("u_VP", vp);

    glEnable(GL_DEPTH_TEST);

    if (m_Wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    // Draw cloth triangles
    m_Shader->SetUniformVec3("u_Color", glm::vec3(0.3f, 0.6f, 0.9f));
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_Indices.size()), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_Shader->Unbind();

    RenderUI();
}

void HXPBDDemoApp::OnShutdown() {
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
}

void HXPBDDemoApp::UploadMesh() {
    // Upload current particle positions to GPU
    std::vector<glm::vec3> positions;
    positions.reserve(m_Cloth.particles.size());
    for (auto& p : m_Cloth.particles) {
        positions.push_back(p.position);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    positions.size() * sizeof(glm::vec3), positions.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void HXPBDDemoApp::RenderUI() {
    ImGui::Begin("HXPBD Cloth Simulation");
    ImGui::Text("Particles: %zu  |  Constraints: %zu",
                m_Cloth.particles.size(), m_Cloth.distConstraints.size());
    ImGui::Separator();

    ImGui::SliderFloat3("Gravity", &m_Gravity.x, -20.0f, 5.0f);
    ImGui::SliderFloat3("Wind",    &m_Wind.x,    -5.0f,  5.0f);
    ImGui::SliderInt("Iterations", &m_Cloth.iterations, 1, 16);
    ImGui::SliderFloat("Damping",  &m_Cloth.damping, 0.95f, 1.0f);
    ImGui::SliderFloat("Time Scale", &m_TimeScale, 0.1f, 3.0f);

    ImGui::Separator();
    ImGui::Checkbox("Pause",     &m_Paused);
    ImGui::Checkbox("Wireframe", &m_Wireframe);

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::SliderFloat("Yaw",   &m_CamYaw,   -3.14f, 3.14f);
    ImGui::SliderFloat("Pitch", &m_CamPitch, -1.5f,  1.5f);
    ImGui::SliderFloat("Dist",  &m_CamDist,   5.0f,  80.0f);

    if (ImGui::Button("Reset Cloth")) {
        m_Cloth = HXPBDClothSim(32, 32, 0.33f, glm::vec3(-5.3f, 10.f, -5.3f));
        m_Cloth.pinTopEdge();
        m_Cloth.accelerations.push_back(m_Gravity);
    }

    ImGui::End();
}
