#include "CharacterControllerApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Core/RandomUtils.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include <imgui.h>
#include <glm/glm.hpp>
#include <cmath>
#include <filesystem>

CharacterControllerApp::CharacterControllerApp()
    : Application("Character Controller Demo") {
}

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

void CharacterControllerApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    // Lock cursor for FPS controls
    Input::SetCursorLocked(true);
    Input::SetCursorVisible(false);
    
    CreateScene();
    
    GE_INFO("Character Controller initialized!");
    GE_INFO("Controls:");
    GE_INFO("  WASD: Move");
    GE_INFO("  Space: Jump");
    GE_INFO("  Mouse: Look around");
    GE_INFO("  V: Toggle first/third person");
    GE_INFO("  ESC: Show cursor");
}

void CharacterControllerApp::CreateScene() {
    m_Scene = CreateRef<Scene>("CharacterScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    auto basicShader = CreateBasicShader();
    
    // Create character controller
    m_CharacterController = CreateRef<Physics::CharacterController>(0.3f, 1.8f);
    m_CharacterController->SetPosition(glm::vec3(0, 2, 0));
    m_CharacterController->SetPhysicsWorld(m_Scene->GetPhysicsWorld().get());
    
    // Create player entity (visual representation)
    m_PlayerEntity = m_Scene->CreateEntity("Player");
    auto& playerTransform = m_PlayerEntity.AddComponent<TransformComponent>();
    playerTransform.Position = glm::vec3(0, 2, 0);
    
    auto playerMesh = MeshGenerator3D::CreateCapsule(0.3f, 1.8f, 16, 8);
    auto playerMaterial = CreateRef<Material>(basicShader);
    playerMaterial->SetVec3("u_Color", glm::vec3(0.2f, 0.6f, 0.8f));
    playerMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    m_PlayerEntity.AddComponent<MeshRendererComponent>(playerMesh, playerMaterial);
    
    // Create camera
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    // Create ground
    m_Ground = m_Scene->CreateEntity("Ground");
    auto& groundTransform = m_Ground.AddComponent<TransformComponent>();
    groundTransform.Scale = glm::vec3(50, 0.5f, 50);
    
    auto groundMesh = MeshGenerator3D::CreateCube(1.0f);
    auto groundMaterial = CreateRef<Material>(basicShader);
    groundMaterial->SetVec3("u_Color", glm::vec3(0.3f, 0.3f, 0.3f));
    groundMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    m_Ground.AddComponent<MeshRendererComponent>(groundMesh, groundMaterial);
    
    // Create obstacles
    for (int i = 0; i < 10; i++) {
        Entity obstacle = m_Scene->CreateEntity("Obstacle_" + std::to_string(i));
        
        auto& transform = obstacle.AddComponent<TransformComponent>();
        transform.Position = glm::vec3(
            Random::Range(-20.0f, 20.0f),
            1.0f,
            Random::Range(-20.0f, 20.0f)
        );
        transform.Scale = glm::vec3(
            Random::Range(1.0f, 3.0f),
            Random::Range(2.0f, 5.0f),
            Random::Range(1.0f, 3.0f)
        );
        
        auto obstacleMesh = MeshGenerator3D::CreateCube(1.0f);
        auto obstacleMaterial = CreateRef<Material>(basicShader);
        obstacleMaterial->SetVec3("u_Color", Random::Global.Color());
        obstacleMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        obstacle.AddComponent<MeshRendererComponent>(obstacleMesh, obstacleMaterial);
        
        m_Obstacles.push_back(obstacle);
    }
    
    // Create stairs
    for (int i = 0; i < 10; i++) {
        Entity step = m_Scene->CreateEntity("Step_" + std::to_string(i));
        
        auto& transform = step.AddComponent<TransformComponent>();
        transform.Position = glm::vec3(-10.0f + i * 0.5f, 0.3f * i, -10.0f);
        transform.Scale = glm::vec3(2.0f, 0.3f, 5.0f);
        
        auto stepMesh = MeshGenerator3D::CreateCube(1.0f);
        auto stepMaterial = CreateRef<Material>(basicShader);
        stepMaterial->SetVec3("u_Color", glm::vec3(0.6f, 0.4f, 0.2f));
        stepMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        step.AddComponent<MeshRendererComponent>(stepMesh, stepMaterial);
        
        m_Obstacles.push_back(step);
    }
}

void CharacterControllerApp::OnUpdate(float deltaTime) {
    HandleMovement(deltaTime);
    HandleCamera();
    
    // Update character controller physics
    m_CharacterController->Update(deltaTime);
    
    // Update player visual position
    auto& playerTransform = m_PlayerEntity.GetComponent<TransformComponent>();
    playerTransform.Position = m_CharacterController->GetPosition();
    
    m_Scene->Update(deltaTime);
}

void CharacterControllerApp::HandleMovement(float deltaTime) {
    // Get input
    float horizontal = 0.0f;
    float vertical = 0.0f;
    
    if (Input::IsKeyPressed(KeyCode::A)) horizontal -= 1.0f;
    if (Input::IsKeyPressed(KeyCode::D)) horizontal += 1.0f;
    if (Input::IsKeyPressed(KeyCode::W)) vertical += 1.0f;
    if (Input::IsKeyPressed(KeyCode::S)) vertical -= 1.0f;
    
    // Calculate movement direction relative to camera
    float yawRad = glm::radians(m_CameraYaw);
    glm::vec3 forward = glm::vec3(
        sin(yawRad),
        0,
        cos(yawRad)
    );
    glm::vec3 right = glm::cross(forward, glm::vec3(0, 1, 0));
    
    glm::vec3 moveDir = (forward * vertical + right * horizontal);
    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir);
    }
    
    glm::vec3 velocity = moveDir * m_MoveSpeed;
    m_CharacterController->Move(velocity);
    
    // Jump
    if (Input::IsKeyJustPressed(KeyCode::Space)) {
        m_CharacterController->Jump(m_JumpForce);
    }
    
    // Toggle camera mode
    if (Input::IsKeyJustPressed(KeyCode::V)) {
        m_IsFirstPerson = !m_IsFirstPerson;
    }
    
    // Toggle cursor
    if (Input::IsKeyJustPressed(KeyCode::Escape)) {
        static bool cursorLocked = true;
        cursorLocked = !cursorLocked;
        Input::SetCursorLocked(cursorLocked);
        Input::SetCursorVisible(!cursorLocked);
    }
}

void CharacterControllerApp::HandleCamera() {
    // Mouse look
    glm::vec2 mouseDelta = Input::GetMouseDelta();
    m_CameraYaw += mouseDelta.x * m_MouseSensitivity;
    m_CameraPitch -= mouseDelta.y * m_MouseSensitivity;
    m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);
    
    // Update camera position
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    glm::vec3 playerPos = m_CharacterController->GetPosition();
    
    if (m_IsFirstPerson) {
        // First person - camera at eye level
        glm::vec3 eyePos = playerPos + glm::vec3(0, 0.7f, 0);
        camTransform.Position = eyePos;
        
        float pitchRad = glm::radians(m_CameraPitch);
        float yawRad = glm::radians(m_CameraYaw);
        glm::vec3 forward = glm::vec3(
            cos(pitchRad) * sin(yawRad),
            sin(pitchRad),
            cos(pitchRad) * cos(yawRad)
        );
        camTransform.LookAt(eyePos + forward);
    } else {
        // Third person - camera behind player
        float yawRad = glm::radians(m_CameraYaw);
        glm::vec3 offset = glm::vec3(
            m_ThirdPersonDistance * sin(yawRad),
            m_ThirdPersonDistance * 0.3f,
            m_ThirdPersonDistance * cos(yawRad)
        );
        camTransform.Position = playerPos - offset + glm::vec3(0, 2, 0);
        camTransform.LookAt(playerPos + glm::vec3(0, 1, 0));
    }
}

void CharacterControllerApp::OnRender() {
    RenderCommand::SetClearColor({0.53f, 0.81f, 0.92f, 1.0f}); // Sky blue
    RenderCommand::Clear();
    
    // Don't render player in first person
    if (m_IsFirstPerson) {
        auto& playerRenderer = m_PlayerEntity.GetComponent<MeshRendererComponent>();
        playerRenderer.Enabled = false;
    } else {
        auto& playerRenderer = m_PlayerEntity.GetComponent<MeshRendererComponent>();
        playerRenderer.Enabled = true;
    }
    
    // Debug draw
    DebugDraw::DrawGrid(50.0f, 50);
    
    // Draw character controller capsule
    if (!m_IsFirstPerson) {
        auto capsule = m_CharacterController->GetCapsuleShape();
        if (capsule) {
            DebugDraw::DrawCapsule(
                m_CharacterController->GetPosition(),
                capsule->GetRadius(),
                capsule->GetHeight(),
                glm::quat(1, 0, 0, 0),
                glm::vec3(1, 1, 0)
            );
        }
    }
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }

    RenderUI();
}

void CharacterControllerApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Character Controller");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Separator();
    
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", 
               m_CharacterController->GetPosition().x,
               m_CharacterController->GetPosition().y,
               m_CharacterController->GetPosition().z);
    ImGui::Text("Velocity: (%.1f, %.1f, %.1f)",
               m_CharacterController->GetVelocity().x,
               m_CharacterController->GetVelocity().y,
               m_CharacterController->GetVelocity().z);
    ImGui::Text("Grounded: %s", m_CharacterController->IsGrounded() ? "Yes" : "No");
    
    ImGui::Separator();
    ImGui::SliderFloat("Move Speed", &m_MoveSpeed, 1.0f, 20.0f);
    ImGui::SliderFloat("Jump Force", &m_JumpForce, 5.0f, 15.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &m_MouseSensitivity, 0.05f, 0.5f);
    
    ImGui::Separator();
    ImGui::Checkbox("First Person", &m_IsFirstPerson);
    if (!m_IsFirstPerson) {
        ImGui::SliderFloat("Camera Distance", &m_ThirdPersonDistance, 2.0f, 10.0f);
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void CharacterControllerApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new CharacterControllerApp();
}
