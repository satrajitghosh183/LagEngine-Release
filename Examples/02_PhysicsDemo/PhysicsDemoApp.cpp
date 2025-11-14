#include "PhysicsDemoApp.hpp"
#include <GameEngine/Scene/Components/TransformComponent.hpp>
#include <GameEngine/Scene/Components/MeshRendererComponent.hpp>
#include <GameEngine/Scene/Components/RigidBodyComponent.hpp>
#include <GameEngine/Scene/Components/ColliderComponent.hpp>
#include <GameEngine/Scene/Components/CameraComponent.hpp>
#include <GameEngine/Graphics/Material.hpp>
#include <GameEngine/Graphics/Shader.hpp>
#include <GameEngine/UI/UIRenderer.hpp>
#include <GameEngine/Platform/Input.hpp>
#include <GameEngine/Core/Time.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

PhysicsDemoApp::PhysicsDemoApp() : Application("Physics Demo") {
}

void PhysicsDemoApp::OnInit() {
    // Initialize UI
    UIRenderer::Init();
    
    // Create scene
    m_Scene = CreateRef<Scene>("PhysicsDemoScene");
    GetSceneManager()->SetActiveScene(m_Scene);
    
    // Create basic shader
    const char* vertexSrc = R"(
        #version 450 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        
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
        #version 450 core
        layout(location = 0) out vec4 FragColor;
        
        in vec3 v_Normal;
        in vec3 v_Position;
        
        uniform vec3 u_Color;
        uniform vec3 u_LightDir;
        uniform vec3 u_LightColor;
        
        void main() {
            vec3 normal = normalize(v_Normal);
            float diff = max(dot(normal, -u_LightDir), 0.2);
            vec3 color = u_Color * diff * u_LightColor;
            FragColor = vec4(color, 1.0);
        }
    )";
    
    m_BasicShader = CreateRef<Shader>("Basic", vertexSrc, fragmentSrc);
    
    // Generate meshes
    m_CubeMesh = MeshGenerator3D::CreateCube(1.0f);
    m_SphereMesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
    m_PlaneMesh = MeshGenerator3D::CreatePlane(20.0f, 20.0f, 1, 1);
    
    // Setup scene
    SetupScene();
    
    GE_INFO("Physics Demo initialized!");
}

void PhysicsDemoApp::OnUpdate(float deltaTime) {
    UpdateCamera(deltaTime);
    
    // Spawn objects on spacebar
    if (Input::IsKeyPressed(KeyCode::Space)) {
        CreateFallingObjects();
    }
}

void PhysicsDemoApp::OnRender() {
    // UI
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Physics Demo");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Text("Entities: %zu", m_Scene->GetEntityCount());
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move camera");
    ImGui::BulletText("Mouse - Rotate camera");
    ImGui::BulletText("Space - Spawn objects");
    ImGui::BulletText("R - Reset scene");
    ImGui::Separator();
    ImGui::SliderFloat("Camera Distance", &m_CameraDistance, 5.0f, 50.0f);
    ImGui::SliderFloat("Camera Pitch", &m_CameraPitch, -89.0f, 89.0f);
    ImGui::SliderFloat("Camera Yaw", &m_CameraYaw, -180.0f, 180.0f);
    
    if (ImGui::Button("Reset Scene")) {
        SetupScene();
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void PhysicsDemoApp::OnShutdown() {
    UIRenderer::Shutdown();
}

void PhysicsDemoApp::SetupScene() {
    // Clear existing entities (except camera)
    auto entities = m_Scene->GetAllEntities();
    for (auto& entity : entities) {
        if (entity != m_CameraEntity) {
            m_Scene->DestroyEntity(entity);
        }
    }
    
    // Setup camera
    SetupCamera();
    
    // Create ground
    CreateGround();
    
    // Create initial falling objects
    CreateFallingObjects();
}

void PhysicsDemoApp::CreateGround() {
    Entity ground = m_Scene->CreateEntity("Ground");
    
    auto& transform = ground.AddComponent<TransformComponent>();
    transform.Position = glm::vec3(0, 0, 0);
    transform.Scale = glm::vec3(1, 1, 1);
    
    auto material = CreateRef<Material>(m_BasicShader);
    material->SetVec3("u_Color", glm::vec3(0.3f, 0.5f, 0.3f));
    material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    ground.AddComponent<MeshRendererComponent>(m_PlaneMesh, material);
    
    auto& rb = ground.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Static;
    rb.UseGravity = false;
    
    auto& collider = ground.AddComponent<ColliderComponent>();
    collider.Shape = ColliderComponent::ColliderShape::Box;
    collider.Size = glm::vec3(20, 0.1f, 20);
}

void PhysicsDemoApp::CreateFallingObjects() {
    // Spawn a few random objects
    for (int i = 0; i < 3; ++i) {
        Entity obj = m_Scene->CreateEntity("FallingObject_" + std::to_string(i));
        
        auto& transform = obj.AddComponent<TransformComponent>();
        float x = (rand() % 10 - 5) * 0.5f;
        float z = (rand() % 10 - 5) * 0.5f;
        transform.Position = glm::vec3(x, 10.0f + i * 2.0f, z);
        transform.Rotation = glm::quat(glm::vec3(
            (rand() % 360) * 3.14159f / 180.0f,
            (rand() % 360) * 3.14159f / 180.0f,
            (rand() % 360) * 3.14159f / 180.0f
        ));
        
        // Random color
        glm::vec3 color(
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f,
            (rand() % 100) / 100.0f
        );
        
        auto material = CreateRef<Material>(m_BasicShader);
        material->SetVec3("u_Color", color);
        material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        
        // Random shape
        Ref<Mesh3D> mesh;
        if (rand() % 2 == 0) {
            mesh = m_CubeMesh;
        } else {
            mesh = m_SphereMesh;
        }
        
        obj.AddComponent<MeshRendererComponent>(mesh, material);
        
        auto& rb = obj.AddComponent<RigidBodyComponent>();
        rb.Type = RigidBodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f + (rand() % 5);
        rb.UseGravity = true;
        
        auto& collider = obj.AddComponent<ColliderComponent>();
        if (mesh == m_CubeMesh) {
            collider.Shape = ColliderComponent::ColliderShape::Box;
            collider.Size = glm::vec3(1.0f);
        } else {
            collider.Shape = ColliderComponent::ColliderShape::Sphere;
            collider.Radius = 0.5f;
        }
    }
}

void PhysicsDemoApp::SetupCamera() {
    if (!m_CameraEntity.IsValid()) {
        m_CameraEntity = m_Scene->CreateEntity("MainCamera");
        m_CameraEntity.AddComponent<CameraComponent>();
        m_Scene->SetMainCamera(m_CameraEntity);
    }
    
    auto& transform = m_CameraEntity.GetComponent<TransformComponent>();
    UpdateCamera(0.0f);
}

void PhysicsDemoApp::UpdateCamera(float deltaTime) {
    // Handle input
    float moveSpeed = 10.0f * deltaTime;
    float rotateSpeed = 50.0f * deltaTime;
    
    if (Input::IsKeyPressed(KeyCode::W)) {
        m_CameraYaw += rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::S)) {
        m_CameraYaw -= rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::A)) {
        m_CameraPitch += rotateSpeed;
    }
    if (Input::IsKeyPressed(KeyCode::D)) {
        m_CameraPitch -= rotateSpeed;
    }
    
    // Clamp pitch
    m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);
    
    // Calculate camera position
    float yawRad = glm::radians(m_CameraYaw);
    float pitchRad = glm::radians(m_CameraPitch);
    
    glm::vec3 offset;
    offset.x = m_CameraDistance * cos(pitchRad) * cos(yawRad);
    offset.y = m_CameraDistance * sin(pitchRad);
    offset.z = m_CameraDistance * cos(pitchRad) * sin(yawRad);
    
    auto& transform = m_CameraEntity.GetComponent<TransformComponent>();
    transform.Position = m_CameraTarget + offset;
    transform.LookAt(m_CameraTarget);
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new PhysicsDemoApp();
}

