#include "RobotArmApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>

// Simple shader for rendering - loads from file with inline fallback
static Ref<Shader> CreateBasicShader() {
    std::string vertPath = "Assets/Shaders/basic.vert";
    std::string fragPath = "Assets/Shaders/basic.frag";
    if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
        return CreateRef<Shader>(vertPath, fragPath);
    }
    const char* vertexSrc = R"(
        #version 420 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        out vec3 v_Normal;
        out vec3 v_Position;
        void main() {
            v_Position = vec3(u_Transform * vec4(a_Position, 1.0));
            v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;
            gl_Position = u_ViewProjection * vec4(v_Position, 1.0);
        }
    )";
    const char* fragmentSrc = R"(
        #version 420 core
        layout(location = 0) out vec4 FragColor;
        in vec3 v_Normal;
        in vec3 v_Position;
        uniform vec3 u_Color;
        uniform vec3 u_LightDir;
        void main() {
            vec3 normal = normalize(v_Normal);
            float diff = max(dot(normal, -u_LightDir), 0.3);
            vec3 color = u_Color * diff;
            FragColor = vec4(color, 1.0);
        }
    )";
    return CreateRef<Shader>("Basic", vertexSrc, fragmentSrc);
}

RobotArmApp::RobotArmApp()
    : Application("Robot Arm - Inverse Kinematics") {
}

void RobotArmApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_Scene = CreateRef<Scene>("RobotArmScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    m_BasePosition = glm::vec3(0, 0, 0);
    m_TargetPosition = glm::vec3(3, 2, 0);
    
    auto basicShader = CreateBasicShader();
    
    CreateRobotArm();
    
    // Create camera
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    // Create target visualization
    m_TargetEntity = m_Scene->CreateEntity("Target");
    auto& targetTransform = m_TargetEntity.AddComponent<TransformComponent>();
    targetTransform.Position = m_TargetPosition;
    targetTransform.Scale = glm::vec3(0.3f);
    
    auto targetMesh = MeshGenerator3D::CreateSphere(0.5f, 16, 8);
    auto targetMaterial = CreateRef<Material>(basicShader);
    targetMaterial->SetVec3("u_Color", glm::vec3(1, 0, 0));
    targetMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    m_TargetEntity.AddComponent<MeshRendererComponent>(targetMesh, targetMaterial);
    
    GE_INFO("Robot Arm IK initialized!");
}

void RobotArmApp::CreateRobotArm() {
    auto basicShader = CreateBasicShader();
    
    // Clear old links
    for (auto& link : m_ArmLinks) {
        if (link.Visual.IsValid()) {
            m_Scene->DestroyEntity(link.Visual);
        }
    }
    m_ArmLinks.clear();
    
    glm::vec3 currentPos = m_BasePosition;
    
    for (int i = 0; i < m_NumLinks; i++) {
        IKLink link;
        link.Position = currentPos;
        link.Length = m_LinkLength;
        link.Angle = 0.0f;
        
        // Create visual
        link.Visual = m_Scene->CreateEntity("Link_" + std::to_string(i));
        auto& transform = link.Visual.AddComponent<TransformComponent>();
        transform.Position = currentPos;
        transform.Scale = glm::vec3(0.2f, m_LinkLength, 0.2f);
        
        auto linkMesh = MeshGenerator3D::CreateCylinder(0.5f, 1.0f, 16);
        auto linkMaterial = CreateRef<Material>(basicShader);
        
        // Color gradient
        float t = (float)i / m_NumLinks;
        linkMaterial->SetVec3("u_Color", glm::mix(glm::vec3(0.2f, 0.6f, 0.8f), glm::vec3(0.8f, 0.3f, 0.2f), t));
        linkMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        link.Visual.AddComponent<MeshRendererComponent>(linkMesh, linkMaterial);
        
        m_ArmLinks.push_back(link);
        currentPos.y += m_LinkLength;
    }
}

void RobotArmApp::OnUpdate(float deltaTime) {
    HandleInput();
    
    // Update target position
    m_TargetEntity.GetComponent<TransformComponent>().Position = m_TargetPosition;
    
    // Solve IK
    SolveIK(m_TargetPosition);
    
    // Update visuals
    UpdateArmVisuals();
    
    // Update camera
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    float camX = m_CameraDistance * cos(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    float camY = m_CameraDistance * sin(glm::radians(m_CameraPitch));
    float camZ = m_CameraDistance * sin(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    camTransform.Position = glm::vec3(camX, camY, camZ);
    camTransform.LookAt(glm::vec3(0, m_LinkLength * m_NumLinks * 0.5f, 0));
    
    m_Scene->Update(deltaTime);
}

void RobotArmApp::SolveIK(const glm::vec3& target) {
    // FABRIK algorithm
    for (int iteration = 0; iteration < m_IKIterations; iteration++) {
        ForwardPass(target);
        BackwardPass(m_BasePosition);
    }
}

void RobotArmApp::ForwardPass(const glm::vec3& target) {
    // Start from end effector, work towards base
    m_ArmLinks.back().Position = target;
    
    for (int i = static_cast<int>(m_ArmLinks.size()) - 2; i >= 0; i--) {
        glm::vec3& current = m_ArmLinks[i].Position;
        const glm::vec3& next = m_ArmLinks[i + 1].Position;
        float length = m_ArmLinks[i].Length;
        
        glm::vec3 direction = glm::normalize(current - next);
        current = next + direction * length;
    }
}

void RobotArmApp::BackwardPass(const glm::vec3& base) {
    // Start from base, work towards end effector
    m_ArmLinks[0].Position = base;
    
    for (size_t i = 1; i < m_ArmLinks.size(); i++) {
        glm::vec3& current = m_ArmLinks[i].Position;
        const glm::vec3& prev = m_ArmLinks[i - 1].Position;
        float length = m_ArmLinks[i - 1].Length;
        
        glm::vec3 direction = glm::normalize(current - prev);
        current = prev + direction * length;
    }
}

void RobotArmApp::UpdateArmVisuals() {
    for (size_t i = 0; i < m_ArmLinks.size(); i++) {
        auto& link = m_ArmLinks[i];
        auto& transform = link.Visual.GetComponent<TransformComponent>();
        
        // Calculate link center
        glm::vec3 start = link.Position;
        glm::vec3 end = (i < m_ArmLinks.size() - 1) ? m_ArmLinks[i + 1].Position : m_TargetPosition;
        glm::vec3 center = (start + end) * 0.5f;
        
        transform.Position = center;
        
        // Rotation to point towards next link
        glm::vec3 direction = glm::normalize(end - start);
        glm::vec3 up = glm::vec3(0, 1, 0);
        
        if (std::abs(glm::dot(direction, up)) < 0.999f) {
            glm::vec3 right = glm::normalize(glm::cross(up, direction));
            glm::vec3 newUp = glm::cross(direction, right);
            
            glm::mat3 rotMatrix;
            rotMatrix[0] = right;
            rotMatrix[1] = direction;
            rotMatrix[2] = newUp;
            
            transform.Rotation = glm::quat_cast(rotMatrix);
        }
    }
}

void RobotArmApp::HandleInput() {
    // Move target with arrow keys
    float moveSpeed = 5.0f * Time::GetDeltaTime();
    
    if (Input::IsKeyPressed(KeyCode::Up)) {
        m_TargetPosition.y += moveSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::Down)) {
        m_TargetPosition.y -= moveSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::Left)) {
        m_TargetPosition.x -= moveSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::Right)) {
        m_TargetPosition.x += moveSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::PageUp)) {
        m_TargetPosition.z += moveSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::PageDown)) {
        m_TargetPosition.z -= moveSpeed;
    }
    
    // Camera rotation
    if (Input::IsMouseButtonPressed(MouseButton::Middle)) {
        glm::vec2 delta = Input::GetMouseDelta();
        m_CameraYaw += delta.x * 0.5f;
        m_CameraPitch -= delta.y * 0.5f;
        m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);
    }
}

void RobotArmApp::OnRender() {
    RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    RenderCommand::Clear();
    
    // Debug draw
    DebugDraw::DrawGrid(10.0f, 10);
    
    // Draw arm chain
    for (size_t i = 0; i < m_ArmLinks.size(); i++) {
        glm::vec3 start = m_ArmLinks[i].Position;
        glm::vec3 end = (i < m_ArmLinks.size() - 1) ? m_ArmLinks[i + 1].Position : m_TargetPosition;
        
        DebugDraw::DrawLine(start, end, glm::vec3(0, 1, 0), 0.0f);
        DebugDraw::DrawSphere(start, 0.15f, glm::vec3(1, 1, 0), 0.0f);
    }
    
    // Draw target
    DebugDraw::DrawCross(m_TargetPosition, 0.5f, glm::vec3(1, 0, 0), 0.0f);
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }

    RenderUI();
}

void RobotArmApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Robot Arm IK");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Separator();
    
    ImGui::Text("Target Position:");
    if (ImGui::DragFloat3("##target", &m_TargetPosition.x, 0.1f)) {
        // Clamp to reachable distance
        float maxReach = m_NumLinks * m_LinkLength;
        float distance = glm::length(m_TargetPosition - m_BasePosition);
        if (distance > maxReach) {
            m_TargetPosition = m_BasePosition + glm::normalize(m_TargetPosition - m_BasePosition) * maxReach;
        }
    }
    
    ImGui::Separator();
    if (ImGui::SliderInt("Num Links", &m_NumLinks, 2, 10)) {
        CreateRobotArm();
    }
    
    if (ImGui::SliderFloat("Link Length", &m_LinkLength, 0.5f, 2.0f)) {
        CreateRobotArm();
    }
    
    ImGui::SliderInt("IK Iterations", &m_IKIterations, 1, 20);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Arrow Keys: Move target XY");
    ImGui::BulletText("Page Up/Down: Move target Z");
    ImGui::BulletText("Middle Mouse: Rotate camera");
    
    float maxReach = m_NumLinks * m_LinkLength;
    float currentReach = glm::length(m_TargetPosition - m_BasePosition);
    ImGui::Text("Reach: %.2f / %.2f", currentReach, maxReach);
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void RobotArmApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new RobotArmApp();
}
