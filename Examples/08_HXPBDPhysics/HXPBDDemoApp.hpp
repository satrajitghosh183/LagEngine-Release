#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "HXPBDClothSim.hpp"
#include <memory>

using namespace GameEngine;

/**
 * @brief HXPBD Cloth Physics Demo
 *
 * XPBD algorithm from old_code/feather/core/physics/HXPbdSolver.h — copied 1:1
 * into HXPBDClothSim.hpp.  feather's Mesh/Scene coupling replaced by flat
 * glm::vec3 arrays; physics equations and iteration scheme are unchanged.
 *
 * Reference: HXPbdSolver::solve() (feather), implementing Macklin et al. 2016.
 */
class HXPBDDemoApp : public Application {
public:
    HXPBDDemoApp();
    ~HXPBDDemoApp() = default;

protected:
    void OnInit()           override;
    void OnUpdate(float dt) override;
    void OnRender()         override;
    void OnShutdown()       override;

private:
    void BuildMeshBuffers();
    void UploadMesh();
    void RenderUI();

private:
    // Simulations (two cloths — mirrors feather main.cpp "cloth" and "cloth2")
    HXPBDClothSim m_Cloth{ 32, 32, 0.33f, glm::vec3(-5.3f, 10.f, -5.3f) };

    // Physics settings
    glm::vec3 m_Gravity    = { 0.f, -9.81f, 0.f };
    glm::vec3 m_Wind       = { 0.f, 0.f,    0.f };
    bool      m_Paused     = false;
    bool      m_Wireframe  = false;
    float     m_TimeScale  = 1.0f;

    // Rendering
    Ref<Shader>  m_Shader;
    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;
    unsigned int m_LineVAO = 0, m_LineVBO = 0;
    std::vector<unsigned int> m_Indices;   // triangle indices (static)

    // Camera orbit
    float m_CamYaw   = 0.3f;
    float m_CamPitch = 0.5f;
    float m_CamDist  = 20.0f;
};
