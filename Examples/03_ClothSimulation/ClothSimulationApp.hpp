#pragma once

#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Physics/SoftBody/SoftBody.hpp"
#include "../../Engine/UI/UIRenderer.hpp"
#include "../../Engine/Graphics/Mesh3D.hpp"
#include "../../Engine/Graphics/Texture2D.hpp"
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
    Ref<Shader> m_PBRShader;
    Ref<Material> m_ClothMaterial;
    
    // Textures for cloth
    Ref<Texture2D> m_AlbedoTexture;
    Ref<Texture2D> m_NormalTexture;
    
    // XPBD Solver parameters (like feather engine)
    int m_Width = 32;
    int m_Height = 32;
    float m_Spacing = 0.1f;
    float m_StretchCompliance = 0.0001f;
    float m_BendCompliance = 0.01f;
    float m_Damping = 0.99f;
    int m_SolverIterations = 4;
    
    // Wind
    bool m_WindEnabled = true;
    glm::vec3 m_WindDirection = glm::vec3(1, 0, 0.5f);
    float m_WindStrength = 0.3f;
    
    // Collision sphere
    glm::vec3 m_SpherePosition = glm::vec3(0, 0.5f, 0);
    float m_SphereRadius = 0.6f;
    
    // Camera (orbit style like feather engine)
    float m_CameraYaw = -0.785f;
    float m_CameraPitch = 1.047f;
    float m_CameraDistance = 20.0f;
    glm::vec3 m_CameraTarget = glm::vec3(0, 5, 0);
    
    // Mouse state for orbit
    bool m_LeftMouseDown = false;
    bool m_RightMouseDown = false;
    glm::vec2 m_LastMousePos = glm::vec2(0.0f);
    
    // Pause
    bool m_Paused = false;
    
    // Rendering options
    bool m_Wireframe = false;
    bool m_UseTextures = true;
    glm::vec3 m_ClothColor = glm::vec3(2.0f, 2.0f, 2.0f);
    float m_Metallic = 0.0f;
    float m_Roughness = 0.8f;
};
