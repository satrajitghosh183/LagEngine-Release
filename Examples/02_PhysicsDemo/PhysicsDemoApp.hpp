#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/UI/UIRenderer.hpp"
#include <memory>
#include <vector>

using namespace GameEngine;

/**
 * @brief Physics demo application
 * 
 * Enhanced demo inspired by Bullet physics demos, demonstrating:
 * - Rigid body dynamics with multiple shapes
 * - Object stacking
 * - Force, torque, and impulse
 * - Interactive object spawning
 * - Camera controls
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
    void CreateStack(const glm::vec3& basePos, int layers);
    void CreatePyramid(const glm::vec3& basePos, int baseSize);
    void CreateDominos(const glm::vec3& startPos, int count);
    Entity SpawnBox(const glm::vec3& pos, const glm::vec3& size, const glm::vec3& color, float mass = 1.0f);
    Entity SpawnSphere(const glm::vec3& pos, float radius, const glm::vec3& color, float mass = 1.0f);
    Entity SpawnCylinder(const glm::vec3& pos, float radius, float height, const glm::vec3& color, float mass = 1.0f);
    void SetupCamera();
    void UpdateCamera(float deltaTime);
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Entity m_CameraEntity;
    std::vector<Entity> m_DynamicObjects;
    
    // Camera control (orbit style)
    float m_CameraYaw = 0.5f;
    float m_CameraPitch = 0.6f;
    float m_CameraDistance = 25.0f;
    glm::vec3 m_CameraTarget = glm::vec3(0, 3, 0);
    
    // Mouse state for orbit
    bool m_LeftMouseDown = false;
    bool m_RightMouseDown = false;
    glm::vec2 m_LastMousePos = glm::vec2(0.0f);
    
    // Demo modes
    int m_DemoMode = 0;
    bool m_Paused = false;
    float m_TimeScale = 1.0f;
    
    // Spawn settings
    int m_SpawnShape = 0;
    float m_SpawnMass = 1.0f;
    float m_ImpulseStrength = 10.0f;
    
    // Resources
    Ref<Shader> m_BasicShader;
    Ref<Mesh3D> m_CubeMesh;
    Ref<Mesh3D> m_SphereMesh;
    Ref<Mesh3D> m_CylinderMesh;
    Ref<Mesh3D> m_PlaneMesh;
};
