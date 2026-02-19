#include "PhysicsDemoApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../../Engine/Scene/Components/ColliderComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Physics/Shapes/BoxShape.hpp"
#include "../../Engine/Physics/Shapes/SphereShape.hpp"
#include "../../Engine/Physics/Shapes/PlaneShape.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <filesystem>

PhysicsDemoApp::PhysicsDemoApp() : Application("Physics Demo") {
}

void PhysicsDemoApp::OnInit() {
    
    // Create scene
    m_Scene = CreateRef<Scene>("PhysicsDemoScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    // Load basic shader from external files (with inline fallback)
    std::string vertPath = "Assets/Shaders/basic.vert";
    std::string fragPath = "Assets/Shaders/basic.frag";
    if (std::filesystem::exists(vertPath) && std::filesystem::exists(fragPath)) {
        m_BasicShader = CreateRef<Shader>(vertPath, fragPath);
    } else {
        const char* vertexSrc = R"(
            #version 420 core
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
            #version 420 core
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
    }
    
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
    // Simple render pass - UI disabled for now
    // Scene rendering is handled by the Application base class
}

void PhysicsDemoApp::OnShutdown() {
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
    
    // Use PlaneShape for ground
    auto& collider = ground.AddComponent<ColliderComponent>();
    auto planeShape = CreateRef<Physics::PlaneShape>(glm::vec3(0, 1, 0), 0.0f);
    collider.SetShape(planeShape);
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
        bool useCube = (rand() % 2 == 0);
        if (useCube) {
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
        if (useCube) {
            auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(0.5f, 0.5f, 0.5f));
            collider.SetShape(boxShape);
        } else {
            auto sphereShape = CreateRef<Physics::SphereShape>(0.5f);
            collider.SetShape(sphereShape);
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
