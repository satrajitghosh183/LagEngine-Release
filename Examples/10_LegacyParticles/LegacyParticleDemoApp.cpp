#include "LegacyParticleDemoApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Platform/Input.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>

// ---------------------------------------------------------------------------
// Entry point (GameEngine factory — replaces GLUT main())
// ---------------------------------------------------------------------------
GameEngine::Application* GameEngine::CreateApplication() {
    return new LegacyParticleDemoApp();
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
LegacyParticleDemoApp::LegacyParticleDemoApp()
    : Application("Legacy Particle System (from old_code/particle-sim)") {}

// ---------------------------------------------------------------------------
// OnInit  — mirrors Source.cpp global setup + floor pyramid construction
// ---------------------------------------------------------------------------
void LegacyParticleDemoApp::OnInit() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Build staircase of floors exactly as in Source.cpp (numFloors = 5)
    for (int i = 0; i < m_NumFloors; ++i) {
        float pos  = -20.0f + static_cast<float>(i) * 5.0f; // y positions
        float size = static_cast<float>(m_NumFloors - i) * 10.0f;
        m_Floors.push_back(Floor(pos, size));
    }

    // Camera
    m_Camera = CreateRef<Camera3D>(74.0f, 1.0f, 2.0f, 300.0f);

    // Inline shader (no asset files needed)
    const char* vert = R"(
        #version 330 core
        layout(location=0) in vec3 a_Pos;
        layout(location=1) in vec3 a_Normal;
        uniform mat4 u_VP;
        uniform mat4 u_Model;
        out vec3 v_Normal;
        out vec3 v_Pos;
        void main(){
            v_Pos    = vec3(u_Model * vec4(a_Pos, 1.0));
            v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
            gl_Position = u_VP * vec4(v_Pos, 1.0);
        }
    )";
    const char* frag = R"(
        #version 330 core
        in vec3 v_Normal;
        in vec3 v_Pos;
        uniform vec3  u_Color;
        uniform float u_Alpha;
        uniform vec3  u_LightDir;
        out vec4 FragColor;
        void main(){
            vec3  n    = normalize(v_Normal);
            float diff = max(dot(n, normalize(u_LightDir)), 0.0);
            vec3  col  = u_Color * (0.3 + 0.7 * diff);
            FragColor  = vec4(col, u_Alpha);
        }
    )";
    m_Shader = CreateRef<Shader>(vert, frag);
}

// ---------------------------------------------------------------------------
// OnUpdate — mirrors moveParticles() / addParticle() / removeRecord()
// ---------------------------------------------------------------------------
void LegacyParticleDemoApp::OnUpdate(float /*dt*/) {
    if (m_Paused) return;

    if (m_ConstantFire) {
        AddParticle();
    }

    // Space bar — fire one particle (mirrors GLUT 'f' key)
    if (Input::IsKeyPressed(KeyCode::Space)) {
        AddParticle();
    }

    MoveParticles();
    RemoveRecord();
}

// ---------------------------------------------------------------------------
// OnRender
// ---------------------------------------------------------------------------
void LegacyParticleDemoApp::OnRender() {
    // Camera orbit (mirrors gluLookAt in Source.cpp)
    float yawRad   = glm::radians(m_CamYaw);
    float xCam     = m_CamZoom * std::cos(yawRad);
    float zCam     = m_CamZoom * std::sin(yawRad);
    glm::vec3 eye  = { xCam, static_cast<float>(m_CamPitch), zCam };
    glm::vec3 tgt  = { 0.0f, -20.0f, 0.0f };
    glm::mat4 view = glm::lookAt(eye, tgt, glm::vec3(0,1,0));
    glm::mat4 proj = glm::perspective(glm::radians(74.0f), 1.0f, 2.0f, 300.0f);
    glm::mat4 vp   = proj * view;

    m_Shader->Bind();
    m_Shader->SetUniformMat4("u_VP", vp);
    glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.5f, 1.0f));
    m_Shader->SetUniformVec3("u_LightDir", lightDir);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Draw floors (purple, alpha pyramid) ---
    auto floorMesh = MeshGenerator3D::CreateCube(1.0f);
    int  i = static_cast<int>(m_Floors.size()) - 1;
    for (auto& f : m_Floors) {
        float blend = 1.0f - (static_cast<float>(i--) / static_cast<float>(m_Floors.size()));
        glm::vec3 floorColor = { 186.0f/255.0f, 126.0f/255.0f, 207.0f/255.0f };
        m_Shader->SetUniformVec3("u_Color", floorColor);
        m_Shader->SetUniformFloat("u_Alpha", blend);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, f.getPos(), 0.0f));
        model = glm::scale(model, glm::vec3(f.getSize(), 0.1f, f.getSize()));
        m_Shader->SetUniformMat4("u_Model", model);
        floorMesh->Draw();
    }

    // --- Draw particles ---
    auto sphereMesh = MeshGenerator3D::CreateSphere(1.0f, 10, 15);
    auto cubeMesh   = MeshGenerator3D::CreateCube(1.0f);

    for (auto& p : m_Particles) {
        if (p.getLife() <= 0) continue;

        int col = p.getCol();
        glm::vec3 color = {
            COLOR_TABLE[col][0] / 255.0f,
            COLOR_TABLE[col][1] / 255.0f,
            COLOR_TABLE[col][2] / 255.0f
        };
        float alpha = (p.getLife() < 100) ? (static_cast<float>(p.getLife()) / 100.0f) : 1.0f;

        m_Shader->SetUniformVec3("u_Color", color);
        m_Shader->SetUniformFloat("u_Alpha", alpha);

        auto pos = p.getPos();
        float sz = p.getSize() * 5.0f;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(pos[0], pos[1], pos[2]));
        model = glm::scale(model, glm::vec3(sz, sz, sz));
        m_Shader->SetUniformMat4("u_Model", model);

        bool useSphere = (m_AppType == 2 || m_AppType == 3);
        Ref<Mesh3D>& mesh = useSphere ? sphereMesh : cubeMesh;

        if (m_AppType == 0 || m_AppType == 2) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        mesh->Draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    m_Shader->Unbind();
    glDisable(GL_BLEND);

    RenderUI();
}

void LegacyParticleDemoApp::OnShutdown() {}

// ---------------------------------------------------------------------------
// Physics helpers — copied 1:1 from Source.cpp, only std::list iterators
// ---------------------------------------------------------------------------
void LegacyParticleDemoApp::AddParticle() {
    Particle p(m_FirePosition, m_SpreadRandom, m_ScaleFactor,
                ++m_ParticleCount, m_RandSpeed);
    m_Particles.push_back(p);
}

float LegacyParticleDemoApp::FloorCollision(Particle& p) {
    float y = p.getPos()[1], x = p.getPos()[0], z = p.getPos()[2];
    for (auto& f : m_Floors) {
        if ((y < (f.getPos() + (5 * p.getSize())))
            && (x > (-f.getSize() - (5 * p.getSize())))
            && (x < ( f.getSize() + (5 * p.getSize())))
            && (z > (-f.getSize() - (5 * p.getSize())))
            && (z < ( f.getSize() + (5 * p.getSize())))) {
            return f.getPos();
        }
    }
    return 0;
}

void LegacyParticleDemoApp::ParticleCollision(Particle& p) {
    for (auto& q : m_Particles) {
        if (p.getId() != q.getId()) {
            if (q.getCol() == 2 && p.getCol() == 1) {
                double dx = (double)q.getPos()[0] - (double)p.getPos()[0];
                double dy = (double)q.getPos()[1] - (double)p.getPos()[1];
                double dz = (double)q.getPos()[2] - (double)p.getPos()[2];
                double d  = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (d <= ((p.getSize() * 5) + (q.getSize() * 5))) {
                    bool bx = (p.getPos()[0] > q.getPos()[0]);
                    bool by = (p.getPos()[1] > q.getPos()[1]);
                    bool bz = (p.getPos()[2] > q.getPos()[2]);
                    p.changeDirection(bx, bz, by);
                }
            }
        }
    }
}

void LegacyParticleDemoApp::MoveParticles() {
    for (auto& p : m_Particles) {
        if (p.getSpeed() != 0) {
            p.move(m_Gravity);
        }
        float collisionPos = FloorCollision(p);
        if (m_NumFloors != 0) {
            if (collisionPos != 0) {
                if (p.getCol() == 0) p.changeColor();
                p.bounce(collisionPos, m_Friction);
                if (p.checkDead(m_Gravity)) {
                    if (p.getCol() == 1) p.changeColor();
                }
            }
            p.checkOffPyramid(m_RemoveParticles, m_Floors.back().getPos());
        }
        if (m_ParticleBumping) {
            ParticleCollision(p);
        }
    }
}

void LegacyParticleDemoApp::RemoveRecord() {
    m_Particles.remove_if([](Particle& p) { return p.getLife() <= 0; });
}

// ---------------------------------------------------------------------------
// ImGui UI — mirrors the GLUT keyboard controls from Source.cpp
// ---------------------------------------------------------------------------
void LegacyParticleDemoApp::RenderUI() {
    ImGui::Begin("Particle System Controls");

    ImGui::Text("Particles: %zu", m_Particles.size());
    ImGui::Separator();

    // Gravity (mirrors '1'-'5' keys in Source.cpp)
    ImGui::Text("Gravity");
    if (ImGui::Button("0x"))     m_Gravity = 0.0;
    ImGui::SameLine();
    if (ImGui::Button("0.25x"))  m_Gravity = 0.025;
    ImGui::SameLine();
    if (ImGui::Button("1x"))     m_Gravity = 0.1;
    ImGui::SameLine();
    if (ImGui::Button("4x"))     m_Gravity = 0.4;
    ImGui::SameLine();
    if (ImGui::Button("16x"))    m_Gravity = 1.6;

    // Friction (mirrors 'q'-'e' keys in Source.cpp)
    ImGui::Text("Friction");
    if (ImGui::Button("None"))    m_Friction = 0.0;
    ImGui::SameLine();
    if (ImGui::Button("Low"))     m_Friction = 0.05;
    ImGui::SameLine();
    if (ImGui::Button("Med"))     m_Friction = 0.2;
    ImGui::SameLine();
    if (ImGui::Button("High"))    m_Friction = 0.8;
    ImGui::SameLine();
    if (ImGui::Button("Max"))     m_Friction = 3.2;

    ImGui::Separator();

    // Appearance (mirrors 'a'-'d' keys)
    ImGui::Text("Appearance");
    if (ImGui::RadioButton("Wire Cube",   m_AppType == 0)) m_AppType = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("Solid Cube",  m_AppType == 1)) m_AppType = 1;
    ImGui::SameLine();
    if (ImGui::RadioButton("Wire Sphere", m_AppType == 2)) m_AppType = 2;
    ImGui::SameLine();
    if (ImGui::RadioButton("Solid Sphere",m_AppType == 3)) m_AppType = 3;

    ImGui::Separator();

    ImGui::Checkbox("Constant Fire",      &m_ConstantFire);
    ImGui::Checkbox("Random Speed",       &m_RandSpeed);
    ImGui::Checkbox("Particle Bumping",   &m_ParticleBumping);
    ImGui::Checkbox("Remove Dead",        &m_RemoveParticles);
    ImGui::Checkbox("Pause",              &m_Paused);

    ImGui::Separator();
    if (ImGui::Button("Fire One (Space)")) AddParticle();
    if (ImGui::Button("Clear All"))        m_Particles.clear();

    ImGui::Separator();
    ImGui::SliderFloat("Spread",    (float*)&m_SpreadRandom,  0.0f, 1.0f);
    ImGui::SliderFloat3("Canon Pos", m_FirePosition, -50.0f, 50.0f);
    ImGui::SliderFloat("Camera Yaw",   &m_CamYaw,   0.0f, 360.0f);
    ImGui::SliderFloat("Camera Pitch", &m_CamPitch, -90.0f, 90.0f);
    ImGui::SliderFloat("Camera Zoom",  &m_CamZoom,  5.0f, 200.0f);

    ImGui::End();
}
