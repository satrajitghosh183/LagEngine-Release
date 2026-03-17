#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "Physics2D.hpp"
#include <list>
#include <random>
#include <memory>

using namespace GameEngine;

/**
 * @brief 2D Physics Demo (from old_code/src/main2D.cpp)
 *
 * Physics: Verlet cloth + projectile balls — logic 1:1 from old_code/engine.
 *   Cloth2D::update()  →  Verlet integration + constraint satisfaction
 *   Ball2D::update()   →  gravity + floor bounce + restitution
 *   Cannon2D          →  fires balls at the cloth
 *   checkCollisionAndTear() →  ball-cloth interaction
 *
 * Rendering in a 2D orthographic projection (XY plane), replacing SFML draws
 * with OpenGL lines/circles.
 */
class Physics2DDemoApp : public Application {
public:
    Physics2DDemoApp();
    ~Physics2DDemoApp() = default;

protected:
    void OnInit()           override;
    void OnUpdate(float dt) override;
    void OnRender()         override;
    void OnShutdown()       override;

private:
    void RespawnCloth();
    void FireBall();
    void BuildLineMesh();
    void RenderUI();

private:
    // Simulation objects (mirrors main2D.cpp globals)
    std::unique_ptr<p2d::Cloth2D>   m_Cloth;
    std::unique_ptr<p2d::Cannon2D>  m_Cannon;
    std::list<p2d::Ball2D*>         m_Projectiles;
    int m_BallsFired = 0;

    // Simulation settings (mirrors main2D.cpp defaults)
    glm::vec2 m_Gravity    = { 0.f, 980.f };
    float     m_WorldH     = 700.0f;   // canvas height (like SFML 720 window)
    bool      m_Paused     = false;

    // Rendering
    Ref<Shader>  m_Shader;
    unsigned int m_LineVAO = 0, m_LineVBO = 0;
    unsigned int m_CircleVAO = 0, m_CircleVBO = 0;

    // Ortho projection — [0..1280] x [0..720]
    float m_ViewW = 1280.f;
    float m_ViewH = 720.f;

    // Random color generator
    std::mt19937 m_Rng{ std::random_device{}() };
    std::uniform_int_distribution<int> m_ColorDist{ 100, 255 };
};
