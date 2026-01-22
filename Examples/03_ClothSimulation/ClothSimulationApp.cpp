#include "ClothSimulationApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/UI/UIRenderer.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

ClothSimulationApp::ClothSimulationApp() : Application("Cloth Simulation") {
}

void ClothSimulationApp::OnInit() {
    UIRenderer::Init();
    
    m_Scene = CreateRef<Scene>("ClothScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    // Create shader
    const char* vertexSrc = R"(
        #version 450 core
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
        #version 450 core
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
    
    m_ClothShader = CreateRef<Shader>("Cloth", vertexSrc, fragmentSrc);
    
    m_ClothMaterial = CreateRef<Material>(m_ClothShader);
    m_ClothMaterial->SetVec3("u_Color", glm::vec3(0.8f, 0.2f, 0.2f));
    m_ClothMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    CreateScene();
    CreateCloth();
    
    GE_INFO("Cloth Simulation initialized!");
}

void ClothSimulationApp::OnUpdate(float deltaTime) {
    UpdateCloth(deltaTime);
    
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
    
    auto& transform = m_CameraEntity.GetComponent<TransformComponent>();
    transform.Position = m_CameraTarget + offset;
    transform.LookAt(m_CameraTarget);
}

void ClothSimulationApp::OnRender() {
    RenderUI();
}

void ClothSimulationApp::OnShutdown() {
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
    
    // Ground
    m_Ground = m_Scene->CreateEntity("Ground");
    auto& groundTransform = m_Ground.AddComponent<TransformComponent>();
    groundTransform.Position = glm::vec3(0, -2, 0);
    
    auto groundMesh = MeshGenerator3D::CreatePlane(10.0f, 10.0f);
    auto groundMaterial = CreateRef<Material>(m_ClothShader);
    groundMaterial->SetVec3("u_Color", glm::vec3(0.3f, 0.3f, 0.3f));
    groundMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    m_Ground.AddComponent<MeshRendererComponent>(groundMesh, groundMaterial);
}

void ClothSimulationApp::CreateCloth() {
    // Create mesh from vertices
    m_ClothMesh = MeshGenerator3D::CreatePlane(
        (m_Width - 1) * m_Spacing,
        (m_Height - 1) * m_Spacing,
        m_Width - 1,
        m_Height - 1
    );
    
    // Create cloth entity
    Entity cloth = m_Scene->CreateEntity("Cloth");
    auto& transform = cloth.AddComponent<TransformComponent>();
    transform.Position = glm::vec3(0, 2, 0);
    
    cloth.AddComponent<MeshRendererComponent>(m_ClothMesh, m_ClothMaterial);
}

void ClothSimulationApp::UpdateCloth(float deltaTime) {
    // Update cloth physics - placeholder
}

void ClothSimulationApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Cloth Simulation");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Separator();
    ImGui::Text("Cloth Parameters:");
    ImGui::SliderInt("Width", &m_Width, 5, 50);
    ImGui::SliderInt("Height", &m_Height, 5, 50);
    ImGui::SliderFloat("Spacing", &m_Spacing, 0.05f, 0.5f);
    ImGui::SliderFloat("Stiffness", &m_Stiffness, 0.0f, 1.0f);
    ImGui::SliderFloat("Damping", &m_Damping, 0.0f, 1.0f);
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Rotate camera");
    
    if (ImGui::Button("Reset Cloth")) {
        CreateCloth();
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new ClothSimulationApp();
}
