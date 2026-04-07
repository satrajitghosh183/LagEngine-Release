#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "FlagPhysics.hpp"
#include <memory>

using namespace GameEngine;

/**
 * @brief Flag / Cloth Simulation Demo
 *
 * Physics from old_code/cloth-simulation/src/flag.cpp — copied 1:1 into
 * FlagPhysics.hpp (hookForce, brakeForce, repulseForce, Flag struct with
 * applyInternalForces / applyExternalForce / applySphereCollision / update).
 *
 * PartyKel SDL window and Octree replaced by GameEngine Application + OpenGL
 * line mesh.  All spring stiffness / damping values default to the same values
 * as the original demo.
 */
class FlagSimulationApp : public Application {
public:
    FlagSimulationApp();
    ~FlagSimulationApp() = default;

protected:
    void OnInit()           override;
    void OnUpdate(float dt) override;
    void OnRender()         override;
    void OnShutdown()       override;

private:
    void BuildLineMesh();
    void RenderUI();

private:
    // Flag physics (1:1 from flag.cpp defaults)
    Flag          m_Flag{ 1.0f, 8.0f, 3.0f, 70, 30 };
    SphereHandler m_Spheres;

    float m_SphereCollMult  = 1.5f;
    float m_RadiusDelta     = 0.15f;
    float m_MaxDstRepulse   = 0.17f;
    float m_MultRepulse     = 0.1f;
    float m_WindVelocity    = 0.025f;
    float m_TargetWind      = 0.025f;
    bool  m_ActiveSpheres   = true;
    bool  m_ActiveAutoCol   = true;
    bool  m_Wireframe       = false;
    bool  m_Paused          = false;

    glm::vec3 m_Gravity     = { 0.f, -0.05f, 0.f };

    // Moving sphere target (mirrors SDL arrow keys)
    float m_SpherePosZ;
    float m_SpherePosY;
    float m_MoveStep = 1.0f;

    // Rendering
    Ref<Shader>  m_Shader;
    unsigned int m_LineVAO  = 0;
    unsigned int m_LineVBO  = 0;

    // Camera orbit
    float m_CamYaw   = 0.0f;
    float m_CamPitch = 0.3f;
    float m_CamDist  = 12.0f;
};
