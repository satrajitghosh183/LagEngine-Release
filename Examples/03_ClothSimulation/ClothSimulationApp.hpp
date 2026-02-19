#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Physics/SoftBody/SoftBody.hpp"
#include "../../Engine/UI/UIRenderer.hpp"
#include "../../Engine/Graphics/Mesh3D.hpp"
#include "../../Engine/Graphics/VertexArray.hpp"
#include <memory>
#include <vector>

using namespace GameEngine;

class ClothSimulationApp : public Application {
public:
    ClothSimulationApp();
    
protected:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    
private:
    void CreateCloth();
    void CreateScene();
    void UpdateCloth(float deltaTime);
    void UpdateClothMesh();
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Ref<Physics::ClothBody> m_Cloth;
    Entity m_ClothEntity;
    
    Entity m_CameraEntity;
    Entity m_Ground;
    Entity m_CollisionSphere;
    
    Ref<Mesh3D> m_ClothMesh;
    Ref<Shader> m_ClothShader;
    Ref<Material> m_ClothMaterial;
    
    // Cloth parameters
    int m_Width = 25;
    int m_Height = 25;
    float m_Spacing = 0.08f;
    float m_Stiffness = 0.9f;
    float m_Damping = 0.99f;
    
    // Wind
    bool m_WindEnabled = true;
    glm::vec3 m_WindDirection = glm::vec3(1, 0, 0.5f);
    float m_WindStrength = 0.5f;
    
    // Collision sphere
    glm::vec3 m_SpherePosition = glm::vec3(0, 0, 0);
    float m_SphereRadius = 0.5f;
    
    // Camera
    float m_CameraYaw = 45.0f;
    float m_CameraPitch = 30.0f;
    float m_CameraDistance = 5.0f;
    glm::vec3 m_CameraTarget = glm::vec3(0, 1, 0);
    
    // Pause
    bool m_Paused = false;
};
