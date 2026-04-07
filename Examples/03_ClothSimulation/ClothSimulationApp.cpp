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
#include <glad/glad.h>
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>

ClothSimulationApp::ClothSimulationApp() : Application("XPBD Cloth Simulation") {
}

void ClothSimulationApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_Scene = CreateRef<Scene>("ClothScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    std::string pbrVertPath = "Assets/Shaders/default_pbr.vert";
    std::string pbrFragPath = "Assets/Shaders/default_pbr.frag";
    if (std::filesystem::exists(pbrVertPath) && std::filesystem::exists(pbrFragPath)) {
        m_PBRShader = CreateRef<Shader>(pbrVertPath, pbrFragPath);
        GE_INFO("PBR shader loaded successfully");
    }
    
    const char* vertexSrc = R"(
        #version 420 core
        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Normal;
        layout(location = 2) in vec2 a_TexCoord;
        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;
        out vec3 v_Normal;
        out vec3 v_Position;
        out vec2 v_TexCoord;
        void main() {
            v_Position = vec3(u_Transform * vec4(a_Position, 1.0));
            v_Normal = mat3(transpose(inverse(u_Transform))) * a_Normal;
            v_TexCoord = a_TexCoord;
            gl_Position = u_ViewProjection * vec4(v_Position, 1.0);
        }
    )";
    const char* fragmentSrc = R"(
        #version 420 core
        layout(location = 0) out vec4 FragColor;
        in vec3 v_Normal;
        in vec3 v_Position;
        in vec2 v_TexCoord;
        
        uniform vec3 u_Color;
        uniform vec3 u_LightDir;
        uniform vec3 u_CameraPosition;
        
        uniform sampler2D u_AlbedoMap;
        uniform sampler2D u_NormalMap;
        uniform int u_HasAlbedoMap;
        uniform int u_HasNormalMap;
        
        void main() {
            vec3 normal = normalize(v_Normal);
            
            vec3 baseColor = u_Color;
            if (u_HasAlbedoMap != 0) {
                baseColor *= texture(u_AlbedoMap, v_TexCoord).rgb;
            }
            
            // Two-sided lighting
            float NdotL = dot(normal, -u_LightDir);
            float diff = max(abs(NdotL), 0.2);
            
            // Simple specular
            vec3 viewDir = normalize(u_CameraPosition - v_Position);
            vec3 halfDir = normalize(viewDir - u_LightDir);
            float spec = pow(max(dot(normal, halfDir), 0.0), 32.0) * 0.3;
            
            vec3 color = baseColor * diff + vec3(spec);
            
            // Gamma correction
            color = pow(color, vec3(1.0/2.2));
            
            FragColor = vec4(color, 1.0);
        }
    )";
    m_ClothShader = CreateRef<Shader>("Cloth", vertexSrc, fragmentSrc);
    
    std::string albedoPath = "Assets/Textures/carpet.jpg";
    std::string normalPath = "Assets/Textures/carpet_normal.png";
    if (std::filesystem::exists(albedoPath)) {
        m_AlbedoTexture = CreateRef<Texture2D>(albedoPath);
        m_AlbedoTexture->GenerateMipmaps();
        GE_INFO("Loaded cloth albedo texture: {}", albedoPath);
    }
    if (std::filesystem::exists(normalPath)) {
        m_NormalTexture = CreateRef<Texture2D>(normalPath);
        m_NormalTexture->GenerateMipmaps();
        GE_INFO("Loaded cloth normal texture: {}", normalPath);
    }
    
    m_ClothMaterial = CreateRef<Material>(m_ClothShader);
    m_ClothMaterial->SetVec3("u_Color", m_ClothColor);
    m_ClothMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    
    CreateScene();
    CreateCloth();
    
    GE_INFO("XPBD Cloth Simulation initialized with {}x{} particles", m_Width, m_Height);
}

void ClothSimulationApp::OnUpdate(float deltaTime) {
    if (Input::IsKeyJustPressed(KeyCode::Space)) {
        m_Paused = !m_Paused;
    }
    
    if (Input::IsKeyJustPressed(KeyCode::R)) {
        CreateCloth();
    }
    
    if (Input::IsKeyJustPressed(KeyCode::W)) {
        m_Wireframe = !m_Wireframe;
    }
    
    glm::vec2 currentMousePos = Input::GetMousePosition();
    glm::vec2 mouseDelta = currentMousePos - m_LastMousePos;
    
    if (Input::IsMouseButtonPressed(MouseButton::Left) && !ImGui::GetIO().WantCaptureMouse) {
        if (!m_LeftMouseDown) {
            m_LeftMouseDown = true;
        } else {
            m_CameraYaw -= mouseDelta.x * 0.005f;
            m_CameraPitch += mouseDelta.y * 0.005f;
            m_CameraPitch = glm::clamp(m_CameraPitch, 0.1f, 3.0f);
        }
    } else {
        m_LeftMouseDown = false;
    }
    
    if (Input::IsMouseButtonPressed(MouseButton::Right) && !ImGui::GetIO().WantCaptureMouse) {
        if (!m_RightMouseDown) {
            m_RightMouseDown = true;
        } else {
            glm::vec3 forward(std::cos(m_CameraYaw), 0, std::sin(m_CameraYaw));
            glm::vec3 right(-forward.z, 0, forward.x);
            m_CameraTarget = m_CameraTarget + right * (-mouseDelta.x * 0.01f * m_CameraDistance)
                           + glm::vec3(0, 1, 0) * (mouseDelta.y * 0.01f * m_CameraDistance);
        }
    } else {
        m_RightMouseDown = false;
    }
    
    float scrollDelta = Input::GetMouseScroll().y;
    if (std::abs(scrollDelta) > 0.001f) {
        m_CameraDistance *= std::pow(0.9f, scrollDelta);
        m_CameraDistance = glm::clamp(m_CameraDistance, 5.0f, 50.0f);
    }
    
    m_LastMousePos = currentMousePos;
    
    float cy = std::cos(m_CameraYaw), sy = std::sin(m_CameraYaw);
    float cp = std::cos(m_CameraPitch), sp = std::sin(m_CameraPitch);
    glm::vec3 dir(cp * cy, sp, cp * sy);
    glm::vec3 eyePos = m_CameraTarget + dir * m_CameraDistance;
    
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    camTransform.Position = eyePos;
    camTransform.LookAt(m_CameraTarget);
    
    if (!m_Paused) {
        UpdateCloth(deltaTime);
    }
    
    if (m_CollisionSphere.IsValid()) {
        auto& sphereTransform = m_CollisionSphere.GetComponent<TransformComponent>();
        sphereTransform.Position = m_SpherePosition;
        sphereTransform.Scale = glm::vec3(m_SphereRadius * 2.0f);
    }
    
    m_Scene->Update(deltaTime);
}

void ClothSimulationApp::OnRender() {
    RenderCommand::SetClearColor({0.15f, 0.15f, 0.2f, 1.0f});
    RenderCommand::Clear();
    
    if (m_Wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    
    DebugDraw::DrawGrid(20.0f, 20);
    
    DebugDraw::DrawSphere(m_SpherePosition, m_SphereRadius, glm::vec3(0, 1, 1), 0.0f);
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }
    
    if (m_Wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    RenderUI();
}

void ClothSimulationApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

void ClothSimulationApp::CreateScene() {
    m_CameraEntity = m_Scene->CreateEntity("MainCamera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    float cy = std::cos(m_CameraYaw), sy = std::sin(m_CameraYaw);
    float cp = std::cos(m_CameraPitch), sp = std::sin(m_CameraPitch);
    glm::vec3 dir(cp * cy, sp, cp * sy);
    camTransform.Position = m_CameraTarget + dir * m_CameraDistance;
    camTransform.LookAt(m_CameraTarget);
    
    m_Ground = m_Scene->CreateEntity("Ground");
    auto& groundTransform = m_Ground.AddComponent<TransformComponent>();
    groundTransform.Position = glm::vec3(0, 0, 0);
    groundTransform.Scale = glm::vec3(40.0f, 1.0f, 40.0f);
    
    auto groundMesh = MeshGenerator3D::CreatePlane(1.0f, 1.0f);
    auto groundMaterial = CreateRef<Material>(m_ClothShader);
    groundMaterial->SetVec3("u_Color", glm::vec3(0.5f, 0.5f, 0.5f));
    groundMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    groundMaterial->SetInt("u_HasAlbedoMap", 0);
    groundMaterial->SetInt("u_HasNormalMap", 0);
    
    m_Ground.AddComponent<MeshRendererComponent>(groundMesh, groundMaterial);
    
    m_CollisionSphere = m_Scene->CreateEntity("CollisionSphere");
    auto& sphereTransform = m_CollisionSphere.AddComponent<TransformComponent>();
    sphereTransform.Position = m_SpherePosition;
    sphereTransform.Scale = glm::vec3(m_SphereRadius * 2.0f);
    
    auto sphereMesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
    auto sphereMaterial = CreateRef<Material>(m_ClothShader);
    sphereMaterial->SetVec3("u_Color", glm::vec3(0.2f, 0.6f, 0.9f));
    sphereMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    sphereMaterial->SetInt("u_HasAlbedoMap", 0);
    sphereMaterial->SetInt("u_HasNormalMap", 0);
    
    m_CollisionSphere.AddComponent<MeshRendererComponent>(sphereMesh, sphereMaterial);
}

void ClothSimulationApp::CreateCloth() {
    if (m_ClothEntity.IsValid()) {
        m_Scene->DestroyEntity(m_ClothEntity);
    }
    
    m_Cloth = CreateRef<Physics::ClothBody>(m_Width, m_Height, m_Spacing);
    
    m_Cloth->Damping = m_Damping;
    m_Cloth->SolverIterations = m_SolverIterations;
    
    m_Cloth->PinCorner(2);
    m_Cloth->PinCorner(3);
    
    m_Cloth->GetSolver().AddPlaneCollider(glm::vec3(0, 1, 0), 0.0f);
    
    m_Cloth->GetSolver().AddSphereCollider(m_SpherePosition, m_SphereRadius);
    
    m_ClothMesh = MeshGenerator3D::CreatePlane(
        (m_Width - 1) * m_Spacing,
        (m_Height - 1) * m_Spacing,
        m_Width - 1,
        m_Height - 1
    );
    
    m_ClothEntity = m_Scene->CreateEntity("Cloth");
    auto& transform = m_ClothEntity.AddComponent<TransformComponent>();
    transform.Position = glm::vec3(0, 7, 0);
    
    m_ClothMaterial->SetVec3("u_Color", m_ClothColor);
    if (m_UseTextures && m_AlbedoTexture) {
        m_ClothMaterial->SetInt("u_HasAlbedoMap", 1);
        m_ClothMaterial->SetTexture("u_AlbedoMap", m_AlbedoTexture);
    } else {
        m_ClothMaterial->SetInt("u_HasAlbedoMap", 0);
    }
    if (m_UseTextures && m_NormalTexture) {
        m_ClothMaterial->SetInt("u_HasNormalMap", 1);
        m_ClothMaterial->SetTexture("u_NormalMap", m_NormalTexture);
    } else {
        m_ClothMaterial->SetInt("u_HasNormalMap", 0);
    }
    
    m_ClothEntity.AddComponent<MeshRendererComponent>(m_ClothMesh, m_ClothMaterial);
    
    GE_INFO("Created cloth with {}x{} = {} particles", m_Width, m_Height, m_Width * m_Height);
}

void ClothSimulationApp::UpdateCloth(float deltaTime) {
    if (!m_Cloth) return;
    
    if (m_WindEnabled) {
        m_Cloth->ApplyWind(glm::normalize(m_WindDirection), m_WindStrength);
    }
    
    m_Cloth->Update(deltaTime);
    
    UpdateClothMesh();
}

void ClothSimulationApp::UpdateClothMesh() {
    if (!m_Cloth || !m_ClothMesh) return;
    
    const auto& positions = m_Cloth->GetPositions();
    const auto& normals = m_Cloth->GetNormals();
    
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
    
    m_ClothMesh->UpdateVertices(vertices);
}

void ClothSimulationApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("XPBD Cloth Simulation");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Text("Particles: %d", m_Width * m_Height);
    ImGui::Text("Solver Iterations: %d", m_SolverIterations);
    
    if (m_Paused) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "PAUSED");
    }
    
    ImGui::Separator();
    
    ImGui::Text("Simulation:");
    ImGui::Checkbox("Paused (Space)", &m_Paused);
    ImGui::SliderFloat("Damping", &m_Damping, 0.9f, 1.0f);
    ImGui::SliderInt("Solver Iterations", &m_SolverIterations, 1, 20);
    
    if (m_Cloth) {
        m_Cloth->Damping = m_Damping;
        m_Cloth->SolverIterations = m_SolverIterations;
    }
    
    ImGui::Separator();
    
    ImGui::Text("Wind:");
    ImGui::Checkbox("Enable Wind", &m_WindEnabled);
    ImGui::SliderFloat("Wind Strength", &m_WindStrength, 0.0f, 2.0f);
    ImGui::SliderFloat3("Wind Direction", &m_WindDirection.x, -1.0f, 1.0f);
    
    ImGui::Separator();
    
    ImGui::Text("Collision Sphere:");
    if (ImGui::SliderFloat3("Position", &m_SpherePosition.x, -5.0f, 5.0f)) {
    }
    ImGui::SliderFloat("Radius", &m_SphereRadius, 0.1f, 2.0f);
    
    ImGui::Separator();
    
    ImGui::Text("Appearance:");
    ImGui::Checkbox("Wireframe (W)", &m_Wireframe);
    ImGui::Checkbox("Use Textures", &m_UseTextures);
    ImGui::ColorEdit3("Cloth Color", &m_ClothColor.x);
    
    ImGui::Separator();
    
    ImGui::Text("Cloth Size (requires reset):");
    ImGui::SliderInt("Width", &m_Width, 5, 64);
    ImGui::SliderInt("Height", &m_Height, 5, 64);
    ImGui::SliderFloat("Spacing", &m_Spacing, 0.02f, 0.2f);
    
    if (ImGui::Button("Reset Cloth (R)")) {
        CreateCloth();
    }
    
    ImGui::Separator();
    
    ImGui::Text("Controls:");
    ImGui::BulletText("Left Mouse: Orbit camera");
    ImGui::BulletText("Right Mouse: Pan camera");
    ImGui::BulletText("Scroll: Zoom");
    ImGui::BulletText("Space: Pause/Resume");
    ImGui::BulletText("R: Reset cloth");
    ImGui::BulletText("W: Toggle wireframe");
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new ClothSimulationApp();
}
