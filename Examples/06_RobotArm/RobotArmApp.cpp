#include "RobotArmApp.hpp"
#include <GameEngine/Graphics/MeshGenerator3D.hpp>
#include <GameEngine/Utilities/Debug/DebugDraw.hpp>
#include <GameEngine/Platform/Input.hpp>
#include <GameEngine/Core/MathUtils.hpp>

RobotArmApp::RobotArmApp()
    : Application("Robot Arm - Inverse Kinematics") {
}

void RobotArmApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_Scene = CreateRef<Scene>("RobotArmScene");
    m_BasePosition = glm::vec3(0, 0, 0);
    m_TargetPosition = glm::vec3(3, 2, 0);
    
    CreateRobotArm();
    
    // Create camera
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    
    // Create target visualization
    m_TargetEntity = m_Scene->CreateEntity("Target");
    auto& targetTransform = m_TargetEntity.GetComponent<TransformComponent>();
    targetTransform.Position = m_TargetPosition;
    targetTransform.Scale = glm::vec3(0.3f);
    
    auto& targetMesh = m_TargetEntity.AddComponent<MeshRendererComponent>();
    targetMesh.Mesh = MeshGenerator3D::CreateSphere(0.5f);
    targetMesh.Material = CreateRef<Material>();
    targetMesh.Material->Albedo = glm::vec3(1, 0, 0);
    
    GE_INFO("Robot Arm IK initialized!");
}

void RobotArmApp::CreateRobotArm() {
    m_ArmLinks.clear();
    
    glm::vec3 currentPos = m_BasePosition;
    
    for (int i = 0; i < m_NumLinks; i++) {
        IKLink link;
        link.Position = currentPos;
        link.Length = m_LinkLength;
        link.Angle = 0.0f;
        
        // Create visual
        link.Visual = m_Scene->CreateEntity("Link_" + std::to_string(i));
        auto& transform = link.Visual.GetComponent<TransformComponent>();
        transform.Position = currentPos;
        transform.Scale = glm::vec3(0.2f, m_LinkLength, 0.2f);
        
        auto& mesh = link.Visual.AddComponent<MeshRendererComponent>();
        mesh.Mesh = MeshGenerator3D::CreateCylinder(0.5f, 1.0f);
        mesh.Material = CreateRef<Material>();
        
        // Color gradient
        float t = (float)i / m_NumLinks;
        mesh.Material->Albedo = glm::mix(glm::vec3(0.2f, 0.6f, 0.8f), glm::vec3(0.8f, 0.3f, 0.2f), t);
        
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
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    float camX = m_CameraDistance * cos(Math::ToRadians(m_CameraYaw)) * cos(Math::ToRadians(m_CameraPitch));
    float camY = m_CameraDistance * sin(Math::ToRadians(m_CameraPitch));
    float camZ = m_CameraDistance * sin(Math::ToRadians(m_CameraYaw)) * cos(Math::ToRadians(m_CameraPitch));
    cam.SetPosition(glm::vec3(camX, camY, camZ));
    cam.LookAt(glm::vec3(0, m_LinkLength * m_NumLinks * 0.5f, 0));
    
    m_Scene->OnUpdate(deltaTime);
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
    
    for (int i = m_ArmLinks.size() - 2; i >= 0; i--) {
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
    
    for (int i = 1; i < m_ArmLinks.size(); i++) {
        glm::vec3& current = m_ArmLinks[i].Position;
        const glm::vec3& prev = m_ArmLinks[i - 1].Position;
        float length = m_ArmLinks[i - 1].Length;
        
        glm::vec3 direction = glm::normalize(current - prev);
        current = prev + direction * length;
    }
}

void RobotArmApp::UpdateArmVisuals() {
    for (int i = 0; i < m_ArmLinks.size(); i++) {
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
        
        if (abs(glm::dot(direction, up)) < 0.999f) {
            glm::vec3 right = glm::normalize(glm::cross(up, direction));
            glm::vec3 newUp = glm::cross(direction, right);
            
            glm::mat3 rotMatrix;
            rotMatrix[0] = right;
            rotMatrix[1] = direction;
            rotMatrix[2] = newUp;
            
            transform.Rotation = glm::degrees(glm::eulerAngles(glm::quat_cast(rotMatrix)));
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
    
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    m_Scene->OnRender(cam);
    
    // Debug draw
    DebugDraw::DrawGrid(10.0f, 10);
    
    // Draw arm chain
    for (int i = 0; i < m_ArmLinks.size(); i++) {
        glm::vec3 start = m_ArmLinks[i].Position;
        glm::vec3 end = (i < m_ArmLinks.size() - 1) ? m_ArmLinks[i + 1].Position : m_TargetPosition;
        
        DebugDraw::DrawLine(start, end, glm::vec3(0, 1, 0), 0.0f);
        DebugDraw::DrawSphere(start, 0.15f, glm::vec3(1, 1, 0), 0.0f);
    }
    
    // Draw target
    DebugDraw::DrawCross(m_TargetPosition, 0.5f, glm::vec3(1, 0, 0), 0.0f);
    
    DebugDraw::Update(Time::GetDeltaTime());
    DebugDraw::Render(cam);
    
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