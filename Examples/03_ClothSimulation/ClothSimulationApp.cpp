#include "ClothSimulationApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/UI/UIRenderer.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

ClothSimulationApp::ClothSimulationApp() : Application("Cloth Simulation") {
}

void ClothSimulationApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_Scene = CreateRef<Scene>("ClothScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    // Load cloth shader from external files (with inline fallback)
    std::string vertPath = "Assets/Shaders/cloth.vert";
    std::string fragPath = "Assets/Shaders/cloth.frag";
    if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
        m_ClothShader = CreateRef<Shader>(vertPath, fragPath);
    } else {
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
                float NdotL = dot(normal, -u_LightDir);
                float diff = max(abs(NdotL), 0.3);
                vec3 color = u_Color * diff;
                FragColor = vec4(color, 1.0);
            }
        )";
        m_ClothShader = CreateRef<Shader>("Cloth", vertexSrc, fragmentSrc);
    }
    
    m_ClothMaterial = CreateRef<Material>(m_ClothShader);
    m_ClothMaterial->SetVec3("u_Color", glm::vec3(0.8f, 0.2f, 0.2f));
    m_ClothMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    CreateScene();
    CreateCloth();
    
    GE_INFO("Cloth Simulation initialized with {0}x{1} particles", m_Width, m_Height);
}

void ClothSimulationApp::OnUpdate(float deltaTime) {
    // Toggle pause
    if (Input::IsKeyJustPressed(KeyCode::Space)) {
        m_Paused = !m_Paused;
    }
    
    // Reset cloth
    if (Input::IsKeyJustPressed(KeyCode::R)) {
        CreateCloth();
    }
    
    // Update cloth physics
    if (!m_Paused) {
        UpdateCloth(deltaTime);
    }
    
    // Camera controls
    float rotateSpeed = 50.0f * deltaTime;
    if (Input::IsKeyPressed(KeyCode::A)) {
        m_CameraYaw -= rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::D)) {
        m_CameraYaw += rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::W)) {
        m_CameraPitch += rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::S)) {
        m_CameraPitch -= rotateSpeed;
    }
    
    m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);
    
    // Update camera position
    float yawRad = glm::radians(m_CameraYaw);
    float pitchRad = glm::radians(m_CameraPitch);
    
    glm::vec3 offset;
    offset.x = m_CameraDistance * cos(pitchRad) * cos(yawRad);
    offset.y = m_CameraDistance * sin(pitchRad);
    offset.z = m_CameraDistance * cos(pitchRad) * sin(yawRad);
    
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    camTransform.Position = m_CameraTarget + offset;
    camTransform.LookAt(m_CameraTarget);
    
    // Update collision sphere position (for interaction)
    if (m_CollisionSphere.IsValid()) {
        auto& sphereTransform = m_CollisionSphere.GetComponent<TransformComponent>();
        sphereTransform.Position = m_SpherePosition;
    }
    
    m_Scene->Update(deltaTime);
}

void ClothSimulationApp::OnRender() {
    RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    RenderCommand::Clear();
    
    // Debug draw
    DebugDraw::DrawGrid(10.0f, 10);
    
    // Draw collision sphere
    DebugDraw::DrawSphere(m_SpherePosition, m_SphereRadius, glm::vec3(0, 1, 1), 0.0f);
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }

    RenderUI();
}

void ClothSimulationApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

void ClothSimulationApp::CreateScene() {
    // Camera
    m_CameraEntity = m_Scene->CreateEntity("MainCamera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    float yawRad = glm::radians(m_CameraYaw);
    float pitchRad = glm::radians(m_CameraPitch);
    glm::vec3 offset;
    offset.x = m_CameraDistance * cos(pitchRad) * cos(yawRad);
    offset.y = m_CameraDistance * sin(pitchRad);
    offset.z = m_CameraDistance * cos(pitchRad) * sin(yawRad);
    camTransform.Position = m_CameraTarget + offset;
    camTransform.LookAt(m_CameraTarget);
    
    // Ground plane
    m_Ground = m_Scene->CreateEntity("Ground");
    auto& groundTransform = m_Ground.AddComponent<TransformComponent>();
    groundTransform.Position = glm::vec3(0, -1, 0);
    
    auto groundMesh = MeshGenerator3D::CreatePlane(10.0f, 10.0f);
    auto groundMaterial = CreateRef<Material>(m_ClothShader);
    groundMaterial->SetVec3("u_Color", glm::vec3(0.3f, 0.3f, 0.3f));
    groundMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    m_Ground.AddComponent<MeshRendererComponent>(groundMesh, groundMaterial);
    
    // Collision sphere for interaction
    m_CollisionSphere = m_Scene->CreateEntity("CollisionSphere");
    auto& sphereTransform = m_CollisionSphere.AddComponent<TransformComponent>();
    sphereTransform.Position = m_SpherePosition;
    sphereTransform.Scale = glm::vec3(m_SphereRadius * 2.0f);
    
    auto sphereMesh = MeshGenerator3D::CreateSphere(0.5f, 16, 16);
    auto sphereMaterial = CreateRef<Material>(m_ClothShader);
    sphereMaterial->SetVec3("u_Color", glm::vec3(0.2f, 0.6f, 0.9f));
    sphereMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    m_CollisionSphere.AddComponent<MeshRendererComponent>(sphereMesh, sphereMaterial);
}

void ClothSimulationApp::CreateCloth() {
    // Delete old cloth entity if exists
    if (m_ClothEntity.IsValid()) {
        m_Scene->DestroyEntity(m_ClothEntity);
    }
    
    // Create cloth physics body
    m_Cloth = CreateRef<Physics::ClothBody>(m_Width, m_Height, m_Spacing);
    
    // Configure solver
    m_Cloth->Damping = m_Damping;
    m_Cloth->SolverIterations = 5;
    
    // Pin top corners
    m_Cloth->PinCorner(2); // Top-left
    m_Cloth->PinCorner(3); // Top-right
    
    // Add ground plane collider
    m_Cloth->GetSolver().AddPlaneCollider(glm::vec3(0, 1, 0), -1.0f);
    
    // Add sphere collider
    m_Cloth->GetSolver().AddSphereCollider(m_SpherePosition, m_SphereRadius);
    
    // Create mesh - we'll update vertices each frame
    m_ClothMesh = MeshGenerator3D::CreatePlane(
        (m_Width - 1) * m_Spacing,
        (m_Height - 1) * m_Spacing,
        m_Width - 1,
        m_Height - 1
    );
    
    // Create cloth entity
    m_ClothEntity = m_Scene->CreateEntity("Cloth");
    auto& transform = m_ClothEntity.AddComponent<TransformComponent>();
    transform.Position = glm::vec3(0, 0, 0); // World space - particles have their own positions
    
    m_ClothEntity.AddComponent<MeshRendererComponent>(m_ClothMesh, m_ClothMaterial);
    
    GE_INFO("Created cloth with {0}x{1} = {2} particles", m_Width, m_Height, m_Width * m_Height);
}

void ClothSimulationApp::UpdateCloth(float deltaTime) {
    if (!m_Cloth) return;
    
    // Apply wind
    if (m_WindEnabled) {
        m_Cloth->ApplyWind(glm::normalize(m_WindDirection), m_WindStrength);
    }
    
    // Update cloth simulation
    m_Cloth->Update(deltaTime);
    
    // Update mesh from particle positions
    UpdateClothMesh();
}

void ClothSimulationApp::UpdateClothMesh() {
    if (!m_Cloth || !m_ClothMesh) return;
    
    const auto& positions = m_Cloth->GetPositions();
    const auto& normals = m_Cloth->GetNormals();
    
    // Create vertex data using Vertex3D structure
    std::vector<Vertex3D> vertices;
    vertices.reserve(m_Width * m_Height);
    
    for (int y = 0; y < m_Height; y++) {
        for (int x = 0; x < m_Width; x++) {
            int idx = y * m_Width + x;
            
            Vertex3D vertex;
            vertex.Position = positions[idx];
            vertex.Normal = normals[idx];
            vertex.TexCoords = glm::vec2(
                static_cast<float>(x) / (m_Width - 1),
                static_cast<float>(y) / (m_Height - 1)
            );
            
            vertices.push_back(vertex);
        }
    }
    
    // Update mesh vertex data
    m_ClothMesh->UpdateVertices(vertices);
}

void ClothSimulationApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Cloth Simulation");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Text("Particles: %d", m_Width * m_Height);
    
    if (m_Paused) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "PAUSED");
    }
    
    ImGui::Separator();
    
    ImGui::Text("Simulation:");
    ImGui::Checkbox("Paused (Space)", &m_Paused);
    ImGui::SliderFloat("Damping", &m_Damping, 0.9f, 1.0f);
    
    if (m_Cloth) {
        m_Cloth->Damping = m_Damping;
    }
    
    ImGui::Separator();
    
    ImGui::Text("Wind:");
    ImGui::Checkbox("Enable Wind", &m_WindEnabled);
    ImGui::SliderFloat("Wind Strength", &m_WindStrength, 0.0f, 2.0f);
    ImGui::SliderFloat3("Wind Direction", &m_WindDirection.x, -1.0f, 1.0f);
    
    ImGui::Separator();
    
    ImGui::Text("Collision Sphere:");
    if (ImGui::SliderFloat3("Position", &m_SpherePosition.x, -2.0f, 2.0f)) {
        // Update collider position in solver
        if (m_Cloth) {
            // Note: Would need to update existing collider - for now recreate
        }
    }
    ImGui::SliderFloat("Radius", &m_SphereRadius, 0.1f, 1.5f);
    
    ImGui::Separator();
    
    ImGui::Text("Cloth Size (requires reset):");
    ImGui::SliderInt("Width", &m_Width, 5, 50);
    ImGui::SliderInt("Height", &m_Height, 5, 50);
    ImGui::SliderFloat("Spacing", &m_Spacing, 0.02f, 0.2f);
    
    if (ImGui::Button("Reset Cloth (R)")) {
        CreateCloth();
    }
    
    ImGui::Separator();
    
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Rotate camera");
    ImGui::BulletText("Space - Pause/Resume");
    ImGui::BulletText("R - Reset cloth");
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new ClothSimulationApp();
}
