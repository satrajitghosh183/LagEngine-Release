#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Physics/Fluids/SPHSolver.hpp"
#include "../../Engine/Physics/Fluids/FluidRenderer.hpp"
#include "../../Engine/UI/UIRenderer.hpp"

using namespace GameEngine;

class FluidSimulationApp : public Application {
public:
    FluidSimulationApp();
    
protected:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    
private:
    void CreateScene();
    void SpawnFluidParticles();
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Ref<Physics::SPHSolver> m_SPHSolver;
    Physics::FluidRenderer m_FluidRenderer;
    
    Entity m_CameraEntity;
    
    Ref<Shader> m_BasicShader;
    
    // Simulation parameters (tuned like old sph_water.cpp)
    float m_ParticleRadius = 0.014f;
    float m_SmoothingRadius = 0.045f;
    float m_RestDensity = 1000.0f;
    float m_GasConstant = 1600.0f;
    float m_Viscosity = 0.15f;
    float m_BoundsDamping = 0.6f;
    
    // Domain (matches old sph_water.cpp)
    glm::vec3 m_BoundsMin = glm::vec3(-0.6f, 0.0f, -0.3f);
    glm::vec3 m_BoundsMax = glm::vec3(0.6f, 1.0f, 0.3f);
    
    // UI controls
    bool m_Paused = false;
    int m_Substeps = 2;
    
    // Camera (orbit style like old code)
    glm::vec3 m_CameraTarget = glm::vec3(0.0f, 0.5f, 0.0f);
    float m_CameraYaw = 1.9f;
    float m_CameraPitch = -0.25f;
    float m_CameraDistance = 2.2f;
    float m_CameraFOV = 45.0f;
    
    // Mouse state for orbit/pan
    bool m_LeftMouseDown = false;
    bool m_RightMouseDown = false;
    glm::vec2 m_LastMousePos = glm::vec2(0.0f);
};
