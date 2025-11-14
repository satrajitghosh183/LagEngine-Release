#include "CharacterControllerApp.hpp"
#include <GameEngine/Graphics/MeshGenerator3D.hpp>
#include <GameEngine/Utilities/Debug/DebugDraw.hpp>
#include <GameEngine/Platform/Input.hpp>
#include <GameEngine/Core/MathUtils.hpp>

CharacterControllerApp::CharacterControllerApp()
    : Application("Character Controller Demo") {
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
    
    // Create character controller
    m_CharacterController = CreateRef<Physics::CharacterController>(0.3f, 1.8f);
    m_CharacterController->SetPosition(glm::vec3(0, 2, 0));
    
    // Create player entity (visual representation)
    m_PlayerEntity = m_Scene->CreateEntity("Player");
    auto& playerMesh = m_PlayerEntity.AddComponent<MeshRendererComponent>();
    playerMesh.Mesh = MeshGenerator3D::CreateCapsule(0.3f, 1.8f);
    playerMesh.Material = CreateRef<Material>();
    playerMesh.Material->Albedo = glm::vec3(0.2f, 0.6f, 0.8f);
    
    // Create camera
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    
    // Create ground
    m_Ground = m_Scene->CreateEntity("Ground");
    auto& groundTransform = m_Ground.GetComponent<TransformComponent>();
    groundTransform.Scale = glm::vec3(50, 0.5f, 50);
    
    auto& groundMesh = m_Ground.AddComponent<MeshRendererComponent>();
    groundMesh.Mesh = MeshGenerator3D::CreateCube();
    groundMesh.Material = CreateRef<Material>();
    groundMesh.Material->Albedo = glm::vec3(0.3f, 0.3f, 0.3f);
    
    // Create obstacles
    for (int i = 0; i < 10; i++) {
        Entity obstacle = m_Scene->CreateEntity("Obstacle_" + std::to_string(i));
        
        auto& transform = obstacle.GetComponent<TransformComponent>();
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
        
        auto& mesh = obstacle.AddComponent<MeshRendererComponent>();
        mesh.Mesh = MeshGenerator3D::CreateCube();
        mesh.Material = CreateRef<Material>();
        mesh.Material->Albedo = Random::Color();
        
        m_Obstacles.push_back(obstacle);
    }
    
    // Create stairs
    for (int i = 0; i < 10; i++) {
        Entity step = m_Scene->CreateEntity("Step_" + std::to_string(i));
        
        auto& transform = step.GetComponent<TransformComponent>();
        transform.Position = glm::vec3(-10.0f + i * 0.5f, 0.3f * i, -10.0f);
        transform.Scale = glm::vec3(2.0f, 0.3f, 5.0f);
        
        auto& mesh = step.AddComponent<MeshRendererComponent>();
        mesh.Mesh = MeshGenerator3D::CreateCube();
        mesh.Material = CreateRef<Material>();
        mesh.Material->Albedo = glm::vec3(0.6f, 0.4f, 0.2f);
        
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
    
    m_Scene->OnUpdate(deltaTime);
}

void CharacterControllerApp::HandleMovement(float deltaTime) {
    // Get input
    float horizontal = Input::GetAxis("Horizontal"); // A/D keys
    float vertical = Input::GetAxis("Vertical");     // W/S keys
    
    // Calculate movement direction relative to camera
    glm::vec3 forward = glm::vec3(
        sin(Math::ToRadians(m_CameraYaw)),
        0,
        cos(Math::ToRadians(m_CameraYaw))
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
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    glm::vec3 playerPos = m_CharacterController->GetPosition();
    
    if (m_IsFirstPerson) {
        // First person - camera at eye level
        glm::vec3 eyePos = playerPos + glm::vec3(0, 0.7f, 0);
        cam.SetPosition(eyePos);
        
        glm::vec3 forward = glm::vec3(
            cos(Math::ToRadians(m_CameraPitch)) * sin(Math::ToRadians(m_CameraYaw)),
            sin(Math::ToRadians(m_CameraPitch)),
            cos(Math::ToRadians(m_CameraPitch)) * cos(Math::ToRadians(m_CameraYaw))
        );
        cam.LookAt(eyePos + forward);
    } else {
        // Third person - camera behind player
        glm::vec3 offset = glm::vec3(
            m_ThirdPersonDistance * sin(Math::ToRadians(m_CameraYaw)),
            m_ThirdPersonDistance * 0.3f,
            m_ThirdPersonDistance * cos(Math::ToRadians(m_CameraYaw))
        );
        cam.SetPosition(playerPos - offset + glm::vec3(0, 2, 0));
        cam.LookAt(playerPos + glm::vec3(0, 1, 0));
    }
}

void CharacterControllerApp::OnRender() {
    RenderCommand::SetClearColor({0.53f, 0.81f, 0.92f, 1.0f}); // Sky blue
    RenderCommand::Clear();
    
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    
    // Don't render player in first person
    if (m_IsFirstPerson) {
        m_PlayerEntity.GetComponent<MeshRendererComponent>().Enabled = false;
    } else {
        m_PlayerEntity.GetComponent<MeshRendererComponent>().Enabled = true;
    }
    
    m_Scene->OnRender(cam);
    
    // Debug draw
    DebugDraw::DrawGrid(50.0f, 50);
    
    // Draw character controller capsule
    if (!m_IsFirstPerson) {
        auto capsule = m_CharacterController->GetCapsuleShape();
        DebugDraw::DrawCapsule(
            m_CharacterController->GetPosition(),
            capsule->GetRadius(),
            capsule->GetHeight(),
            glm::quat(1, 0, 0, 0),
            glm::vec3(1, 1, 0)
        );
    }
    
    DebugDraw::Update(Time::GetDeltaTime());
    DebugDraw::Render(cam);
    
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
    // Map input axes
    Input::MapAxis("Horizontal", KeyCode::D, KeyCode::A);
    Input::MapAxis("Vertical", KeyCode::W, KeyCode::S);
    
    return new CharacterControllerApp();
}