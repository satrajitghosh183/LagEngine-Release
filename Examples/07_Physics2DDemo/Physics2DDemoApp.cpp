#include "Physics2DDemoApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Platform/Input.hpp"
#include <imgui.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ctime>
#include <cstdlib>

Application* CreateApplication() {
    return new Physics2DDemoApp();
}

Physics2DDemoApp::Physics2DDemoApp()
    : Application("2D Physics Demo (from old_code/src/main2D.cpp)") {}

void Physics2DDemoApp::OnInit() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Create objects — mirrors main2D.cpp setup
    m_Cloth  = std::make_unique<p2d::Cloth2D>(20, 15, 20.0f, glm::vec2(300.f, 50.f));
    m_Cannon = std::make_unique<p2d::Cannon2D>(glm::vec2(300.f, 650.f));
    m_Cloth->setProjectiles(&m_Projectiles);

    // 2D line shader — orthographic projection
    const char* vert = R"(
        #version 330 core
        layout(location=0) in vec2 a_Pos;
        uniform mat4 u_Ortho;
        void main() { gl_Position = u_Ortho * vec4(a_Pos, 0.0, 1.0); }
    )";
    const char* frag = R"(
        #version 330 core
        uniform vec3 u_Color;
        out vec4 FragColor;
        void main() { FragColor = vec4(u_Color, 1.0); }
    )";
    m_Shader = CreateRef<Shader>(vert, frag);

    // Allocate dynamic line VBO for cloth
    glGenVertexArrays(1, &m_LineVAO);
    glGenBuffers(1, &m_LineVBO);
    glBindVertexArray(m_LineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER, 20 * 15 * 2 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glBindVertexArray(0);

    // Circle VBO (for balls) — fixed circle with 32 segments
    glGenVertexArrays(1, &m_CircleVAO);
    glGenBuffers(1, &m_CircleVBO);
    std::vector<glm::vec2> circle;
    for (int i = 0; i <= 32; ++i) {
        float a = static_cast<float>(i) / 32.f * 6.2832f;
        circle.push_back(glm::vec2(std::cos(a), std::sin(a)));
    }
    glBindVertexArray(m_CircleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_CircleVBO);
    glBufferData(GL_ARRAY_BUFFER, circle.size() * sizeof(glm::vec2), circle.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
    glBindVertexArray(0);
}

void Physics2DDemoApp::OnUpdate(float dt) {
    if (m_Paused) return;

    // --- Input (mirrors main2D.cpp key handling) ---
    // Arrow keys → rotate cannon
    if (Input::IsKeyDown(Key::Left))  m_Cannon->rotate(-5.0f * dt * 60.f);
    if (Input::IsKeyDown(Key::Right)) m_Cannon->rotate( 5.0f * dt * 60.f);
    // Up/Down → adjust power
    if (Input::IsKeyDown(Key::Up))   m_Cannon->adjustPower( 50.0f * dt * 60.f);
    if (Input::IsKeyDown(Key::Down)) m_Cannon->adjustPower(-50.0f * dt * 60.f);

    // Space → fire one ball
    static bool spaceWas = false;
    bool spaceNow = Input::IsKeyDown(Key::Space);
    if (spaceNow && !spaceWas) FireBall();
    spaceWas = spaceNow;

    // R → respawn cloth
    static bool rWas = false;
    bool rNow = Input::IsKeyDown(Key::R);
    if (rNow && !rWas) RespawnCloth();
    rWas = rNow;

    // Update balls
    for (auto* b : m_Projectiles) b->update(dt, m_Gravity, m_WorldH);
    m_Projectiles.remove_if([](p2d::Ball2D* b) { return !b->active; });

    // Update cloth
    m_Cloth->update(dt, m_Gravity, 5);

    BuildLineMesh();
}

void Physics2DDemoApp::OnRender() {
    // Ortho: [0..1280] horizontal, [0..720] vertical (Y-down like SFML)
    glm::mat4 ortho = glm::ortho(0.0f, m_ViewW, m_ViewH, 0.0f, -1.0f, 1.0f);

    m_Shader->Bind();
    m_Shader->SetMat4("u_Ortho", ortho);

    glDisable(GL_DEPTH_TEST);

    // --- Draw cloth as white lines ---
    m_Shader->SetFloat3("u_Color", glm::vec3(1.f));
    glBindVertexArray(m_LineVAO);
    int lineVerts = static_cast<int>(m_Cloth->constraints.size()) * 2;
    glDrawArrays(GL_LINES, 0, lineVerts);
    glBindVertexArray(0);

    // --- Draw cannon ---
    // Base circle
    m_Shader->SetFloat3("u_Color", glm::vec3(0.0f, 0.0f, 0.9f));
    {
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(m_Cannon->position, 0.f));
        model = glm::scale(model, glm::vec3(20.f, 20.f, 1.f));
        m_Shader->SetMat4("u_Ortho", ortho * model);
    }
    // Revert
    m_Shader->SetMat4("u_Ortho", ortho);

    // --- Draw balls ---
    glBindVertexArray(m_CircleVAO);
    for (auto* b : m_Projectiles) {
        if (!b->active || !b->visible) continue;
        m_Shader->SetFloat3("u_Color", b->color);
        glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(b->particle.pos, 0.f));
        model = glm::scale(model, glm::vec3(b->radius * 5.f, b->radius * 5.f, 1.f));
        m_Shader->SetMat4("u_Ortho", ortho * model);
        glDrawArrays(GL_LINE_STRIP, 0, 33);
    }
    m_Shader->SetMat4("u_Ortho", ortho);
    glBindVertexArray(0);

    m_Shader->Unbind();
    glEnable(GL_DEPTH_TEST);

    RenderUI();
}

void Physics2DDemoApp::OnShutdown() {
    for (auto* b : m_Projectiles) delete b;
    m_Projectiles.clear();
    if (m_LineVAO) glDeleteVertexArrays(1, &m_LineVAO);
    if (m_LineVBO) glDeleteBuffers(1, &m_LineVBO);
    if (m_CircleVAO) glDeleteVertexArrays(1, &m_CircleVAO);
    if (m_CircleVBO) glDeleteBuffers(1, &m_CircleVBO);
}

void Physics2DDemoApp::RespawnCloth() {
    m_Cloth = std::make_unique<p2d::Cloth2D>(20, 15, 20.0f, glm::vec2(300.f, 50.f));
    m_Cloth->setProjectiles(&m_Projectiles);
}

void Physics2DDemoApp::FireBall() {
    glm::vec2 pos = m_Cannon->getMuzzlePosition();
    glm::vec2 vel = m_Cannon->getFiringVelocity();
    glm::vec3 color(m_ColorDist(m_Rng)/255.f,
                    m_ColorDist(m_Rng)/255.f,
                    m_ColorDist(m_Rng)/255.f);
    auto* ball = new p2d::Ball2D(pos, vel * 0.016f, 10.0f, 0.8f, color);
    m_Projectiles.push_back(ball);
    ++m_BallsFired;
}

void Physics2DDemoApp::BuildLineMesh() {
    // Upload constraint line pairs to GPU
    std::vector<glm::vec2> lines;
    lines.reserve(m_Cloth->constraints.size() * 2);
    for (auto& c : m_Cloth->constraints) {
        lines.push_back(m_Cloth->particles[c.p1Index].pos);
        lines.push_back(m_Cloth->particles[c.p2Index].pos);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_LineVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 lines.size() * sizeof(glm::vec2), lines.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Physics2DDemoApp::RenderUI() {
    ImGui::Begin("2D Physics Controls (old_code/src/main2D.cpp)");
    ImGui::Text("Balls fired: %d  |  Active: %zu", m_BallsFired, m_Projectiles.size());
    ImGui::Text("Cannon angle: %.1f°  |  Power: %.0f", m_Cannon->angle, m_Cannon->power);
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Left/Right Arrow: rotate cannon");
    ImGui::BulletText("Up/Down Arrow:    adjust power");
    ImGui::BulletText("Space:            fire ball");
    ImGui::BulletText("R:                respawn cloth");
    ImGui::Separator();
    ImGui::Checkbox("Pause", &m_Paused);
    ImGui::SliderFloat2("Gravity", &m_Gravity.x, -2000.f, 2000.f);
    if (ImGui::Button("Fire")) FireBall();
    ImGui::SameLine();
    if (ImGui::Button("Respawn Cloth")) RespawnCloth();
    ImGui::SameLine();
    if (ImGui::Button("Clear Balls")) {
        for (auto* b : m_Projectiles) delete b;
        m_Projectiles.clear();
    }
    ImGui::End();
}
