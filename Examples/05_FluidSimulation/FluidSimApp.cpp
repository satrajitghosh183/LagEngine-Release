#include "FluidSimApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Core/RandomUtils.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
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

FluidSimulationApp::FluidSimulationApp()
    : Application("SPH Fluid Simulation") {
}

void FluidSimulationApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    CreateScene();
    SpawnFluidParticles();
    
    GE_INFO("Fluid Simulation initialized!");
}

void FluidSimulationApp::CreateScene() {
    m_Scene = CreateRef<Scene>("FluidScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
    auto basicShader = CreateBasicShader();
    
    // Create SPH solver
    m_SPHSolver = CreateRef<Physics::SPHSolver>(5000);
    m_SPHSolver->BoundsMin = glm::vec3(-5, 0, -5);
    m_SPHSolver->BoundsMax = glm::vec3(5, 10, 5);
    m_SPHSolver->SmoothingRadius = 0.2f;
    m_SPHSolver->Viscosity = 0.018f;
    m_SPHSolver->RestDensity = 1000.0f;
    
    // Create camera
    m_CameraEntity = m_Scene->CreateEntity("Camera");
    m_CameraEntity.AddComponent<CameraComponent>();
    m_Scene->SetMainCamera(m_CameraEntity);
    
    // Create container walls (for visualization)
    auto createWall = [&](const glm::vec3& pos, const glm::vec3& scale) {
        Entity wall = m_Scene->CreateEntity("Wall");
        auto& transform = wall.AddComponent<TransformComponent>();
        transform.Position = pos;
        transform.Scale = scale;
        
        auto wallMesh = MeshGenerator3D::CreateCube(1.0f);
        auto wallMaterial = CreateRef<Material>(basicShader);
        wallMaterial->SetVec3("u_Color", glm::vec3(0.3f, 0.3f, 0.4f));
        wallMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        wall.AddComponent<MeshRendererComponent>(wallMesh, wallMaterial);
        
        return wall;
    };
    
    // Ground
    createWall(glm::vec3(0, -0.25f, 0), glm::vec3(10, 0.5f, 10));
    
    // Walls
    createWall(glm::vec3(-5, 5, 0), glm::vec3(0.2f, 10, 10));
    createWall(glm::vec3(5, 5, 0), glm::vec3(0.2f, 10, 10));
    createWall(glm::vec3(0, 5, -5), glm::vec3(10, 10, 0.2f));
    createWall(glm::vec3(0, 5, 5), glm::vec3(10, 10, 0.2f));
    
    // Create particle mesh (instanced sphere)
    m_ParticleMesh = MeshGenerator3D::CreateSphere(0.1f, 8, 8);
    m_FluidMaterial = CreateRef<Material>(basicShader);
    m_FluidMaterial->SetVec3("u_Color", glm::vec3(0.2f, 0.5f, 0.9f));
    m_FluidMaterial->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
}

void FluidSimulationApp::SpawnFluidParticles() {
    // Spawn fluid in a cube
    float spacing = m_SPHSolver->SmoothingRadius * 0.9f;
    int particlesPerAxis = 15;
    
    for (int z = 0; z < particlesPerAxis; z++) {
        for (int y = 0; y < particlesPerAxis; y++) {
            for (int x = 0; x < particlesPerAxis; x++) {
                glm::vec3 position = glm::vec3(
                    -2.0f + x * spacing,
                    2.0f + y * spacing,
                    -2.0f + z * spacing
                );
                
                m_SPHSolver->AddParticle(position);
                m_ParticleCount++;
            }
        }
    }
    
    GE_INFO("Spawned {0} fluid particles", m_ParticleCount);
}

void FluidSimulationApp::OnUpdate(float deltaTime) {
    // Camera rotation
    if (Input::IsMouseButtonPressed(MouseButton::Middle)) {
        glm::vec2 delta = Input::GetMouseDelta();
        m_CameraYaw += delta.x * 0.5f;
        m_CameraPitch -= delta.y * 0.5f;
        m_CameraPitch = glm::clamp(m_CameraPitch, -89.0f, 89.0f);
    }
    
    // Update camera
    auto& camTransform = m_CameraEntity.GetComponent<TransformComponent>();
    float camX = m_CameraDistance * cos(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    float camY = m_CameraDistance * sin(glm::radians(m_CameraPitch));
    float camZ = m_CameraDistance * sin(glm::radians(m_CameraYaw)) * cos(glm::radians(m_CameraPitch));
    camTransform.Position = glm::vec3(camX, camY + 5, camZ);
    camTransform.LookAt(glm::vec3(0, 5, 0));
    
    // Update fluid simulation
    if (!m_Paused) {
        m_SPHSolver->Update(deltaTime);
    }
    
    // Spawn more particles
    if (Input::IsKeyPressed(KeyCode::Space)) {
        glm::vec3 spawnPos(Random::Range(-2.0f, 2.0f), 8.0f, Random::Range(-2.0f, 2.0f));
        m_SPHSolver->AddParticle(spawnPos, glm::vec3(0, -1, 0));
        m_ParticleCount++;
    }
    
    // Reset
    if (Input::IsKeyJustPressed(KeyCode::R)) {
        m_SPHSolver->Clear();
        m_ParticleCount = 0;
        SpawnFluidParticles();
    }
    
    m_Scene->Update(deltaTime);
}

void FluidSimulationApp::OnRender() {
    RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    RenderCommand::Clear();
    
    // Render fluid particles (simplified - just use debug draw)
    
    // Debug draw
    DebugDraw::DrawGrid(10.0f, 10);
    DebugDraw::DrawBox(
        (m_SPHSolver->BoundsMin + m_SPHSolver->BoundsMax) * 0.5f,
        m_SPHSolver->BoundsMax - m_SPHSolver->BoundsMin,
        glm::quat(1, 0, 0, 0),
        glm::vec3(1, 1, 0)
    );
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }

    RenderUI();
}

void FluidSimulationApp::RenderFluidParticles(const Camera3D& /*camera*/) {
    // Simple particle rendering (could be optimized with instancing)
    const auto& particles = m_SPHSolver->GetParticles();
    
    for (const auto& particle : particles) {
        // Color by density
        float normalizedDensity = particle.Density / m_SPHSolver->RestDensity;
        glm::vec3 color = glm::mix(
            glm::vec3(0.2f, 0.5f, 0.9f), // Low density
            glm::vec3(0.0f, 0.2f, 0.6f), // High density
            glm::clamp(normalizedDensity - 0.5f, 0.0f, 1.0f)
        );
        
        // Render sphere at particle position
        DebugDraw::DrawSphere(particle.Position, 0.1f, color);
    }
}

void FluidSimulationApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("SPH Fluid Simulation");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Separator();
    
    ImGui::Text("Particles: %d", m_ParticleCount);
    ImGui::Checkbox("Paused", &m_Paused);
    
    ImGui::Separator();
    ImGui::Text("Simulation Parameters:");
    ImGui::SliderFloat("Smoothing Radius", &m_SPHSolver->SmoothingRadius, 0.05f, 0.5f);
    ImGui::SliderFloat("Viscosity", &m_SPHSolver->Viscosity, 0.001f, 0.1f);
    ImGui::SliderFloat("Rest Density", &m_SPHSolver->RestDensity, 500.0f, 2000.0f);
    ImGui::SliderFloat("Gas Constant", &m_SPHSolver->GasConstant, 1000.0f, 5000.0f);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Space: Spawn particles");
    ImGui::BulletText("R: Reset simulation");
    ImGui::BulletText("Middle Mouse: Rotate camera");
    
    if (ImGui::Button("Reset Simulation")) {
        m_SPHSolver->Clear();
        m_ParticleCount = 0;
        SpawnFluidParticles();
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void FluidSimulationApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new FluidSimulationApp();
}
