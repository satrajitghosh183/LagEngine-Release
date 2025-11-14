#pragma once

#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <GameEngine/Physics/Fluids/SPHSolver.hpp>
#include <GameEngine/UI/UIRenderer.hpp>

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
    void RenderFluidParticles(const Camera3D& camera);
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Ref<Physics::SPHSolver> m_SPHSolver;
    
    Entity m_CameraEntity;
    Entity m_Container;
    
    Ref<Mesh3D> m_ParticleMesh;
    Ref<Shader> m_ParticleShader;
    Ref<Material> m_FluidMaterial;
    
    // UI controls
    bool m_Paused = false;
    int m_ParticleCount = 0;
    
    // Camera
    float m_CameraYaw = 45.0f;
    float m_CameraPitch = 30.0f;
    float m_CameraDistance = 15.0f;
};