#include "FlagSimulationApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include <imgui.h>
// glad include removed (Vulkan migration)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>

GameEngine::Application* GameEngine::CreateApplication() {
    return new FlagSimulationApp();
}

FlagSimulationApp::FlagSimulationApp()
    : Application("Flag Simulation (from old_code/cloth-simulation)")
{
    // Sphere initial positions — 1:1 from flag.cpp main()
    m_Spheres.positions = {
        glm::vec3(0.0f, -3.0f,  2.0f),
        glm::vec3(1.5f, -3.5f,  0.7f),
        glm::vec3(3.0f, -2.0f, -1.5f)
    };
    m_Spheres.radius = { 2.0f, 1.5f, 0.8f };
    m_Spheres.colors = {
        glm::vec3(1,0,0),
        glm::vec3(1,1,0),
        glm::vec3(0,1,0)
    };
    m_SpherePosZ = m_Spheres.positions[0].z;
    m_SpherePosY = m_Spheres.positions[0].y;
}

void FlagSimulationApp::OnInit() {
    // Inline shader — draws colored lines and solid spheres
    const char* vert = R"(
        #version 330 core
        layout(location=0) in vec3 a_Pos;
        uniform mat4 u_VP;
        uniform mat4 u_Model;
        void main() { gl_Position = u_VP * u_Model * vec4(a_Pos, 1.0); }
    )";
    const char* frag = R"(
        #version 330 core
        uniform vec3 u_Color;
        out vec4 FragColor;
        void main() { FragColor = vec4(u_Color, 1.0); }
    )";
    m_Shader = CreateRef<Shader>(vert, frag);

    // Allocate a dynamic line VBO for the flag grid
    int lineCount = (m_Flag.gridWidth - 1) * m_Flag.gridHeight
                  + m_Flag.gridWidth * (m_Flag.gridHeight - 1);
    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineCount * 2 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);
    glBindVertexArray(0);
}

void FlagSimulationApp::OnUpdate(float dt) {
    if (m_Paused || dt <= 0.f) return;

    // Keyboard: mirror SDL arrow key controls from flag.cpp
    if (Input::IsKeyPressed(KeyCode::Up))    m_SpherePosY += m_MoveStep * dt * 30.0f;
    if (Input::IsKeyPressed(KeyCode::Down))  m_SpherePosY -= m_MoveStep * dt * 30.0f;
    if (Input::IsKeyPressed(KeyCode::Right)) m_SpherePosZ -= m_MoveStep * dt * 30.0f;
    if (Input::IsKeyPressed(KeyCode::Left))  m_SpherePosZ += m_MoveStep * dt * 30.0f;

    // Toggle wireframe on Space — mirrors SDL SDLK_SPACE
    static bool spaceWas = false;
    bool spaceNow = Input::IsKeyPressed(KeyCode::Space);
    if (spaceNow && !spaceWas) m_Wireframe = !m_Wireframe;
    spaceWas = spaceNow;

    // Smooth sphere movement — glm::mix as in flag.cpp
    m_Spheres.positions[0].z = glm::mix(m_Spheres.positions[0].z, m_SpherePosZ, 0.08f);
    m_Spheres.positions[0].y = glm::mix(m_Spheres.positions[0].y, m_SpherePosY, 0.08f);
    m_WindVelocity            = glm::mix(m_WindVelocity, m_TargetWind, 0.08f);

    // --- Simulation loop (mirrors flag.cpp while body) ---
    m_Flag.applyExternalForce(m_Gravity);
    m_Flag.applyExternalForce(glm::sphericalRand(m_WindVelocity));
    m_Flag.applyInternalForces(dt);

    if (m_ActiveSpheres)
        m_Flag.applySphereCollision(m_Spheres, m_SphereCollMult, m_RadiusDelta);

    if (m_ActiveAutoCol)
        m_Flag.applyRepulseForces(m_MaxDstRepulse, m_MultRepulse);

    m_Flag.update(dt);

    // Upload updated positions to GPU line VBO
    BuildLineMesh();
}

void FlagSimulationApp::OnRender() {
    // Camera orbit
    float cy = std::cos(m_CamYaw),   sy = std::sin(m_CamYaw);
    float cp = std::cos(m_CamPitch), sp = std::sin(m_CamPitch);
    glm::vec3 eye = glm::vec3(m_CamDist * cy * cp,
                              m_CamDist * sp,
                              m_CamDist * sy * cp);
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(70.0f),
                                      16.0f / 9.0f, 0.1f, 100.0f);
    glm::mat4 vp   = proj * view;

    m_Shader->Bind();
    m_Shader->SetUniformMat4("u_VP", vp);
    m_Shader->SetUniformMat4("u_Model", glm::mat4(1.0f));

    if (m_Wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Draw flag grid as lines
    m_Shader->SetUniformVec3("u_Color", glm::vec3(0.9f, 0.9f, 0.9f));
    glBindVertexArray(m_LineVAO);
    int lineCount = (m_Flag.gridWidth - 1) * m_Flag.gridHeight
                  + m_Flag.gridWidth * (m_Flag.gridHeight - 1);
    glDrawArrays(GL_LINES, 0, lineCount * 2);
    glBindVertexArray(0);

    // Draw spheres
    if (m_ActiveSpheres) {
        auto sphereMesh = MeshGenerator3D::CreateSphere(1.0f, 16, 16);
        for (size_t i = 0; i < m_Spheres.positions.size(); ++i) {
            m_Shader->SetUniformVec3("u_Color", m_Spheres.colors[i]);
            glm::mat4 model = glm::translate(glm::mat4(1.0f), m_Spheres.positions[i]);
            model = glm::scale(model, glm::vec3(m_Spheres.radius[i]));
            m_Shader->SetUniformMat4("u_Model", model);
            sphereMesh->Draw();
        }
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    m_Shader->Unbind();

    RenderUI();
}

void FlagSimulationApp::OnShutdown() {
    if (m_LineVAO) glDeleteVertexArrays(1, &m_LineVAO);
    if (m_LineVBO) glDeleteBuffers(1, &m_LineVBO);
}

void FlagSimulationApp::BuildLineMesh() {
    // Build line segment pairs for horizontal and vertical grid connections
    std::vector<glm::vec3> lines;
    lines.reserve(
        ((m_Flag.gridWidth - 1) * m_Flag.gridHeight
       + m_Flag.gridWidth * (m_Flag.gridHeight - 1)) * 2);

    for (int j = 0; j < m_Flag.gridHeight; ++j) {
        for (int i = 0; i < m_Flag.gridWidth - 1; ++i) {
            lines.push_back(m_Flag.positionArray[j * m_Flag.gridWidth + i]);
            lines.push_back(m_Flag.positionArray[j * m_Flag.gridWidth + i + 1]);
        }
    }
    for (int j = 0; j < m_Flag.gridHeight - 1; ++j) {
        for (int i = 0; i < m_Flag.gridWidth; ++i) {
            lines.push_back(m_Flag.positionArray[j * m_Flag.gridWidth + i]);
            lines.push_back(m_Flag.positionArray[(j+1) * m_Flag.gridWidth + i]);
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    lines.size() * sizeof(glm::vec3), lines.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FlagSimulationApp::RenderUI() {
    ImGui::Begin("Flag Simulation Controls");

    ImGui::Text("Particles: %d", m_Flag.nbParticles);
    ImGui::Separator();

    // Spring/damping — same parameters as AntTweakBar in flag.cpp
    ImGui::Text("Spring Stiffness");
    ImGui::SliderFloat("K0 (straight)",  &m_Flag.K0, 0.0f, 5.0f);
    ImGui::SliderFloat("K1 (diagonal)",  &m_Flag.K1, 0.0f, 5.0f);
    ImGui::SliderFloat("K2 (long-range)",&m_Flag.K2, 0.0f, 5.0f);

    ImGui::Text("Damping");
    ImGui::SliderFloat("V0", &m_Flag.V0, 0.0f, 1.0f);
    ImGui::SliderFloat("V1", &m_Flag.V1, 0.0f, 1.0f);
    ImGui::SliderFloat("V2", &m_Flag.V2, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::SliderFloat("Wind velocity", &m_TargetWind, 0.0f, 0.5f);
    ImGui::SliderFloat("Sphere coll mult", &m_SphereCollMult, 0.0f, 5.0f);
    ImGui::SliderFloat("Sphere radius", &m_Spheres.radius[0], 0.1f, 5.0f);
    ImGui::SliderFloat("Sphere x", &m_Spheres.positions[0].x, -10.0f, 10.0f);
    ImGui::SliderFloat("Radius delta", &m_RadiusDelta, 0.0f, 1.0f);
    ImGui::SliderFloat("Max dst repulse", &m_MaxDstRepulse, 0.0f, 1.0f);
    ImGui::SliderFloat("Mult repulse",    &m_MultRepulse, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::Checkbox("Active Spheres",     &m_ActiveSpheres);
    ImGui::Checkbox("Self Collision",     &m_ActiveAutoCol);
    ImGui::Checkbox("Wireframe (Space)",  &m_Wireframe);
    ImGui::Checkbox("Pause",              &m_Paused);

    ImGui::Separator();
    ImGui::Text("Camera");
    ImGui::SliderFloat("Yaw",   &m_CamYaw,   -3.14f, 3.14f);
    ImGui::SliderFloat("Pitch", &m_CamPitch, -1.5f,  1.5f);
    ImGui::SliderFloat("Dist",  &m_CamDist,   1.0f,  50.0f);

    if (ImGui::Button("Reset Flag")) {
        float k0 = m_Flag.K0, k1 = m_Flag.K1, k2 = m_Flag.K2;
        float v0 = m_Flag.V0, v1 = m_Flag.V1, v2 = m_Flag.V2;
        m_Flag = Flag(1.0f, 8.0f, 3.0f, 70, 30);
        m_Flag.K0 = k0; m_Flag.K1 = k1; m_Flag.K2 = k2;
        m_Flag.V0 = v0; m_Flag.V1 = v1; m_Flag.V2 = v2;
        BuildLineMesh();
    }

    ImGui::End();
}
