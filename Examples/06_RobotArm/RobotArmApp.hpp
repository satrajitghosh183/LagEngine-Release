#pragma once

#include <GameEngine/Core/Application.hpp>
#include <GameEngine/Scene/Scene.hpp>
#include <GameEngine/UI/UIRenderer.hpp>

using namespace GameEngine;

/**
 * @brief Simple IK chain link
 */
struct IKLink {
    glm::vec3 Position;
    float Length;
    float Angle;
    Entity Visual;
};

/**
 * @brief Robot arm with inverse kinematics
 */
class RobotArmApp : public Application {
public:
    RobotArmApp();
    
protected:
    void OnInit() override;
    void OnUpdate(float deltaTime) override;
    void OnRender() override;
    void OnShutdown() override;
    
private:
    void CreateRobotArm();
    void SolveIK(const glm::vec3& target);
    void UpdateArmVisuals();
    void HandleInput();
    void RenderUI();
    
    // FABRIK (Forward And Backward Reaching Inverse Kinematics)
    void ForwardPass(const glm::vec3& target);
    void BackwardPass(const glm::vec3& base);
    
private:
    Ref<Scene> m_Scene;
    Entity m_CameraEntity;
    Entity m_TargetEntity;
    
    std::vector<IKLink> m_ArmLinks;
    glm::vec3 m_BasePosition;
    glm::vec3 m_TargetPosition;
    
    int m_NumLinks = 5;
    float m_LinkLength = 1.0f;
    int m_IKIterations = 10;
    
    // Camera
    float m_CameraYaw = 45.0f;
    float m_CameraPitch = 30.0f;
    float m_CameraDistance = 10.0f;
};