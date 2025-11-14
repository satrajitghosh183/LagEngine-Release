#pragma once

#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <GameEngine/Physics/Character/CharacterController.hpp>
#include <GameEngine/UI/UIRenderer.hpp>

using namespace GameEngine;

class CharacterControllerApp : public Application {
public:
    CharacterControllerApp();
    
protected:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    
private:
    void CreateScene();
    void HandleMovement(float deltaTime);
    void HandleCamera();
    void RenderUI();
    
private:
    Ref<Scene> m_Scene;
    Ref<Physics::CharacterController> m_CharacterController;
    
    Entity m_PlayerEntity;
    Entity m_CameraEntity;
    Entity m_Ground;
    
    std::vector<Entity> m_Obstacles;
    
    // Movement
    float m_MoveSpeed = 5.0f;
    float m_JumpForce = 7.0f;
    bool m_IsFirstPerson = true;
    
    // Camera
    float m_CameraYaw = 0.0f;
    float m_CameraPitch = 0.0f;
    float m_MouseSensitivity = 0.1f;
    float m_ThirdPersonDistance = 5.0f;
};