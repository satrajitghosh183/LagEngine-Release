#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/Camera3D.hpp"
#include "Particle.h"
#include "Floor.h"
#include <list>
#include <memory>

using namespace GameEngine;

/**
 * @brief Legacy Particle System Demo
 *
 * Physics from old_code/particle-sim/ — copied 1:1 (Particle.h, Floor.h, Line.h).
 * Gravity, friction, floor-bounce, inter-particle collision, and life-cycle
 * logic are unchanged. Only window management and drawing have been swapped
 * from FreeGLUT to GameEngine's OpenGL renderer.
 */
class LegacyParticleDemoApp : public Application {
public:
    LegacyParticleDemoApp();
    ~LegacyParticleDemoApp() = default;

protected:
    void OnInit()     override;
    void OnUpdate(float dt) override;
    void OnRender()   override;
    void OnShutdown() override;

private:
    // --- simulation helpers (same logic as Source.cpp) ---
    void     AddParticle();
    float    FloorCollision(Particle& p);
    void     ParticleCollision(Particle& p);
    void     MoveParticles();
    void     RemoveRecord();

    // --- rendering ---
    void BuildFloorMesh();
    void BuildParticleMesh();
    void RenderUI();

private:
    // Physics state (mirrors Source.cpp globals)
    std::list<Particle> m_Particles;
    std::list<Floor>    m_Floors;
    int    m_ParticleCount  = 0;
    float  m_FirePosition[3] = { 0.0f, 15.0f, 0.0f };

    // Environment (mirrors Source.cpp defaults)
    double m_Gravity        = 0.1;
    double m_Friction       = 0.2;
    int    m_NumFloors      = 5;
    float  m_ScaleFactor    = 0.25f;
    double m_SpreadRandom   = 0.2;
    bool   m_RemoveParticles = true;
    bool   m_ConstantFire   = false;
    bool   m_RandSpeed      = false;
    bool   m_ParticleBumping = false;
    int    m_AppType        = 3;   // 0=wire-cube 1=solid-cube 2=wire-sphere 3=solid-sphere

    // Rendering
    Ref<Shader>   m_Shader;
    Ref<Camera3D> m_Camera;
    float m_CamYaw   = 212.5f;
    float m_CamPitch = 25.0f;
    float m_CamZoom  = 50.0f;
    bool  m_Paused   = false;

    // Color table (cyan / yellow / magenta) - same as cArr in Source.cpp
    static constexpr int COLOR_TABLE[3][3] = {
        { 0, 162, 211 },  // cyan
        { 250, 224, 20 }, // yellow
        { 224, 8, 133 }   // magenta
    };
};
