#include "FluidSimApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>

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

FluidSimulationApp::FluidSimulationApp()
    : Application("SPH Water Simulation") {
}

void FluidSimulationApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_BasicShader = CreateBasicShader();
    
    CreateScene();
    
    m_FluidRenderer.Init();
    
    SpawnFluidParticles();
    
    GE_INFO("SPH Water Simulation initialized with {} particles!", m_SPHSolver->GetParticleCount());
}

void FluidSimulationApp::CreateScene() {
    m_Scene = CreateRef<Scene>("FluidScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    m_SPHSolver = CreateRef<Physics::SPHSolver>(20000);
    m_SPHSolver->BoundsMin = m_BoundsMin;
    m_SPHSolver->BoundsMax = m_BoundsMax;
    m_SPHSolver->SmoothingRadius = m_SmoothingRadius;
    m_SPHSolver->Viscosity = m_Viscosity;
    m_SPHSolver->RestDensity = m_RestDensity;
    m_SPHSolver->GasConstant = m_GasConstant;
    m_SPHSolver->BoundsDamping = m_BoundsDamping;
    
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    auto createWall = [&](const glm::vec3& pos, const glm::vec3& scale, const glm::vec3& color) {
        Entity wall = m_Scene->CreateEntity("Wall");
        auto& transform = wall.AddComponent<TransformComponent>();
        transform.Position = pos;
        transform.Scale = scale;
        
        auto wallMesh = MeshGenerator3D::CreateCube(1.0f);
        auto wallMaterial = CreateRef<Material>(m_BasicShader);
        wallMaterial->SetVec3("u_Color", color);
        wallMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        wall.AddComponent<MeshRendererComponent>(wallMesh, wallMaterial);
        
        return wall;
    };
    
    glm::vec3 domainCenter = (m_BoundsMin + m_BoundsMax) * 0.5f;
    glm::vec3 domainSize = m_BoundsMax - m_BoundsMin;
    
    float thickness = 0.02f;
    glm::vec3 wallColor(0.4f, 0.4f, 0.5f);
    
    createWall(glm::vec3(domainCenter.x, m_BoundsMin.y - thickness * 0.5f, domainCenter.z),
               glm::vec3(domainSize.x + thickness * 2, thickness, domainSize.z + thickness * 2), 
               glm::vec3(0.5f, 0.5f, 0.55f));
    
    createWall(glm::vec3(m_BoundsMin.x - thickness * 0.5f, domainCenter.y, domainCenter.z),
               glm::vec3(thickness, domainSize.y, domainSize.z), wallColor);
    createWall(glm::vec3(m_BoundsMax.x + thickness * 0.5f, domainCenter.y, domainCenter.z),
               glm::vec3(thickness, domainSize.y, domainSize.z), wallColor);
    createWall(glm::vec3(domainCenter.x, domainCenter.y, m_BoundsMin.z - thickness * 0.5f),
               glm::vec3(domainSize.x, domainSize.y, thickness), wallColor);
    createWall(glm::vec3(domainCenter.x, domainCenter.y, m_BoundsMax.z + thickness * 0.5f),
               glm::vec3(domainSize.x, domainSize.y, thickness), wallColor);
}

void FluidSimulationApp::SpawnFluidParticles() {
    m_SPHSolver->Clear();
    
    int nx = 36, ny = 25, nz = 18;
    float spacing = 0.028f;
    glm::vec3 start = m_BoundsMin + glm::vec3(0.1f, 0.1f, 0.08f);
    
    m_SPHSolver->InitBlock(nx, ny, nz, spacing, start);
    
    GE_INFO("Spawned {} fluid particles in {}x{}x{} grid", 
            m_SPHSolver->GetParticleCount(), nx, ny, nz);
}

void FluidSimulationApp::OnUpdate(float deltaTime) {
    glm::vec2 currentMousePos = Input::GetMousePosition();
    glm::vec2 mouseDelta = currentMousePos - m_LastMousePos;
    
    if (Input::IsMouseButtonPressed(MouseButton::Left)) {
        if (!m_LeftMouseDown) {
            m_LeftMouseDown = true;
        } else {
            m_CameraYaw -= mouseDelta.x * 0.005f;
            m_CameraPitch -= mouseDelta.y * 0.005f;
            m_CameraPitch = glm::clamp(m_CameraPitch, -1.55f, 1.2f);
        }
    } else {
        m_LeftMouseDown = false;
    }
    
    if (Input::IsMouseButtonPressed(MouseButton::Right)) {
        if (!m_RightMouseDown) {
            m_RightMouseDown = true;
        } else {
            glm::vec3 forward(std::cos(m_CameraYaw), 0, std::sin(m_CameraYaw));
            glm::vec3 right(-forward.z, 0, forward.x);
            m_CameraTarget = m_CameraTarget + right * (-mouseDelta.x * 0.002f * m_CameraDistance)
                           + forward * (mouseDelta.y * 0.002f * m_CameraDistance);
        }
    } else {
        m_RightMouseDown = false;
    }
    
    float scrollDelta = Input::GetMouseScroll().y;
    if (std::abs(scrollDelta) > 0.001f) {
        m_CameraDistance *= std::pow(0.9f, scrollDelta);
        m_CameraDistance = glm::clamp(m_CameraDistance, 0.5f, 6.0f);
    }
    
    m_LastMousePos = currentMousePos;
    
    float cy = std::cos(m_CameraYaw), sy = std::sin(m_CameraYaw);
    float cp = std::cos(m_CameraPitch), sp = std::sin(m_CameraPitch);
    glm::vec3 dir(cp * cy, sp, cp * sy);
    glm::vec3 eyePos = m_CameraTarget - dir * m_CameraDistance;
    
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    camTransform.Position = eyePos;
    camTransform.LookAt(m_CameraTarget);
    
    if (Input::IsKeyJustPressed(KeyCode::Space)) {
        m_Paused = !m_Paused;
    }
    
    if (Input::IsKeyJustPressed(KeyCode::R)) {
        SpawnFluidParticles();
    }
    
    if (!m_Paused) {
        m_SPHSolver->SmoothingRadius = m_SmoothingRadius;
        m_SPHSolver->Viscosity = m_Viscosity;
        m_SPHSolver->RestDensity = m_RestDensity;
        m_SPHSolver->GasConstant = m_GasConstant;
        m_SPHSolver->BoundsDamping = m_BoundsDamping;
        
        float subDt = 0.004f;
        for (int s = 0; s < m_Substeps; ++s) {
            m_SPHSolver->Update(subDt);
        }
    }
    
    m_Scene->Update(deltaTime);
}

void FluidSimulationApp::OnRender() {
    RenderCommand::SetClearColor({1.0f, 1.0f, 1.0f, 1.0f});
    RenderCommand::Clear();
    
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    Camera3D* cam = scene ? scene->GetMainCamera() : nullptr;
    
    if (cam && m_FluidRenderer.IsReady()) {
        std::vector<glm::vec3> positions = m_SPHSolver->GetPositions();
        m_FluidRenderer.Render(positions, cam->GetViewMatrix(), cam->GetProjectionMatrix(), m_ParticleRadius);
    }
    
    if (cam) {
        DebugDraw::DrawBox(
            (m_BoundsMin + m_BoundsMax) * 0.5f,
            m_BoundsMax - m_BoundsMin,
            glm::quat(1, 0, 0, 0),
            glm::vec3(0.5f, 0.5f, 0.6f)
        );
        
        DebugDraw::Update(Time::GetDeltaTime());
        DebugDraw::Render(*cam);
    }

    RenderUI();
}

void FluidSimulationApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("SPH Water Simulation");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Separator();
    
    ImGui::Text("Particles: %d", m_SPHSolver->GetParticleCount());
    
    if (m_Paused) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "PAUSED (Space to resume)");
    }
    
    ImGui::Checkbox("Paused", &m_Paused);
    ImGui::SliderInt("Substeps", &m_Substeps, 1, 5);
    
    ImGui::Separator();
    ImGui::Text("Simulation Parameters:");
    ImGui::SliderFloat("Smoothing Radius", &m_SmoothingRadius, 0.02f, 0.1f, "%.3f");
    ImGui::SliderFloat("Rest Density", &m_RestDensity, 500.0f, 2000.0f);
    ImGui::SliderFloat("Gas Constant", &m_GasConstant, 500.0f, 5000.0f);
    ImGui::SliderFloat("Viscosity", &m_Viscosity, 0.01f, 0.5f);
    ImGui::SliderFloat("Bounce Damping", &m_BoundsDamping, 0.0f, 1.0f);
    ImGui::SliderFloat("Particle Radius", &m_ParticleRadius, 0.005f, 0.05f, "%.3f");
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Left Mouse Drag: Orbit camera");
    ImGui::BulletText("Right Mouse Drag: Pan camera");
    ImGui::BulletText("Scroll: Zoom in/out");
    ImGui::BulletText("Space: Pause/Resume");
    ImGui::BulletText("R: Reset simulation");
    
    if (ImGui::Button("Reset Simulation")) {
        SpawnFluidParticles();
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void FluidSimulationApp::OnShutdown() {
    m_FluidRenderer.Shutdown();
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new FluidSimulationApp();
}
