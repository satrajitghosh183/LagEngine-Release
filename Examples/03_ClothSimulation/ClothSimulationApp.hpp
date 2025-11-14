#pragma once

#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <GameEngine/Physics/SoftBody/SoftBody.hpp>
#include <GameEngine/UI/UIRenderer.hpp>
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
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Ref<Physics::SoftBody> m_Cloth;
    
    Entity m_CameraEntity;
    Entity m_Ground;
    
    Ref<Mesh3D> m_ClothMesh;
    Ref<Shader> m_ClothShader;
    Ref<Material> m_ClothMaterial;
    
    // Cloth parameters
    int m_Width = 20;
    int m_Height = 20;
    float m_Spacing = 0.1f;
    float m_Stiffness = 0.9f;
    float m_Damping = 0.1f;
    
    // Camera
    float m_CameraYaw = 45.0f;
    float m_CameraPitch = 30.0f;
    float m_CameraDistance = 5.0f;
    glm::vec3 m_CameraTarget = glm::vec3(0, 1, 0);
};

