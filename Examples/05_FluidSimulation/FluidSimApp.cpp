#include "FluidSimApp.hpp"
#include <GameEngine/Graphics/MeshGenerator3D.hpp>
#include <GameEngine/Graphics/RenderCommand.hpp>
#include <GameEngine/Utilities/Debug/DebugDraw.hpp>
#include <GameEngine/Platform/Input.hpp>
#include <GameEngine/Core/MathUtils.hpp>

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
    
    // Create container walls (for visualization)
    auto createWall = [&](const glm::vec3& pos, const glm::vec3& scale) {
        Entity wall = m_Scene->CreateEntity("Wall");
        auto& transform = wall.GetComponent<TransformComponent>();
        transform.Position = pos;
        transform.Scale = scale;
        
        auto& mesh = wall.AddComponent<MeshRendererComponent>();
        mesh.Mesh = MeshGenerator3D::CreateCube();
        mesh.Material = CreateRef<Material>();
        mesh.Material->Albedo = glm::vec3(0.3f, 0.3f, 0.4f);
        mesh.Material->Transparency = 0.3f;
        
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
    m_FluidMaterial = CreateRef<Material>();
    m_FluidMaterial->Albedo = glm::vec3(0.2f, 0.5f, 0.9f);
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
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    float camX = m_CameraDistance * cos(Math::ToRadians(m_CameraYaw)) * cos(Math::ToRadians(m_CameraPitch));
    float camY = m_CameraDistance * sin(Math::ToRadians(m_CameraPitch));
    float camZ = m_CameraDistance * sin(Math::ToRadians(m_CameraYaw)) * cos(Math::ToRadians(m_CameraPitch));
    cam.SetPosition(glm::vec3(camX, camY + 5, camZ));
    cam.LookAt(glm::vec3(0, 5, 0));
    
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
    
    m_Scene->OnUpdate(deltaTime);
}

void FluidSimulationApp::OnRender() {
    RenderCommand::SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    RenderCommand::Clear();
    
    auto& cam = m_CameraEntity.GetComponent<CameraComponent>().GetCamera();
    m_Scene->OnRender(cam);
    
    // Render fluid particles
    RenderFluidParticles(cam);
    
    // Debug draw
    DebugDraw::DrawGrid(10.0f, 10);
    DebugDraw::DrawBox(
        (m_SPHSolver->BoundsMin + m_SPHSolver->BoundsMax) * 0.5f,
        m_SPHSolver->BoundsMax - m_SPHSolver->BoundsMin,
        glm::quat(1, 0, 0, 0),
        glm::vec3(1, 1, 0)
    );
    
    DebugDraw::Update(Time::GetDeltaTime());
    DebugDraw::Render(cam);
    
    RenderUI();
}

void FluidSimulationApp::RenderFluidParticles(const Camera3D& camera) {
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
        // This is simplified - in production you'd use instanced rendering
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), particle.Position);
        transform = glm::scale(transform, glm::vec3(0.1f));
        
        // TODO: Render with proper instancing
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