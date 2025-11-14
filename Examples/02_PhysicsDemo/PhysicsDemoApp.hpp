#pragma once

#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <GameEngine/Graphics/MeshGenerator3D.hpp>
#include <GameEngine/Graphics/Shader.hpp>
#include <GameEngine/Graphics/Material.hpp>
#include <memory>

using namespace GameEngine;

/**
 * @brief Physics demo application
 * 
 * Demonstrates:
 * - 3D physics simulation
 * - Rigid body dynamics
 * - Camera controls
 * - Scene management
 * - Material and shader usage
 */
class PhysicsDemoApp : public Application {
public:
    PhysicsDemoApp();
    virtual ~PhysicsDemoApp() = default;
    
protected:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    
private:
    void SetupScene();
    void CreateGround();
    void CreateFallingObjects();
    void SetupCamera();
    void UpdateCamera(float deltaTime);
    
private:
    Ref<Scene> m_Scene;
    Entity m_CameraEntity;
    
    // Camera control
    float m_CameraYaw = -90.0f;
    float m_CameraPitch = 20.0f;
    float m_CameraDistance = 20.0f;
    glm::vec3 m_CameraTarget = glm::vec3(0, 2, 0);
    
    // Resources
    Ref<Shader> m_BasicShader;
    Ref<Mesh3D> m_CubeMesh;
    Ref<Mesh3D> m_SphereMesh;
    Ref<Mesh3D> m_PlaneMesh;
};