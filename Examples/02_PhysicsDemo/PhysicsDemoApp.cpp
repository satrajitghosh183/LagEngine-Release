#include "PhysicsDemoApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../../Engine/Scene/Components/ColliderComponent.hpp"
#include "../../Engine/Scene/Components/CameraComponent.hpp"
#include "../../Engine/Graphics/Material.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include "../../Engine/Platform/Input.hpp"
#include "../../Engine/Core/Time.hpp"
#include "../../Engine/Core/RandomUtils.hpp"
#include "../../Engine/Physics/Shapes/BoxShape.hpp"
#include "../../Engine/Physics/Shapes/SphereShape.hpp"
#include "../../Engine/Physics/Shapes/PlaneShape.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <filesystem>

PhysicsDemoApp::PhysicsDemoApp() : Application("Physics Demo - Bullet Style") {
}

void PhysicsDemoApp::OnInit() {
    UIRenderer::Init();
    DebugDraw::Init();
    
    m_Scene = CreateRef<Scene>("PhysicsDemoScene");
    GetSceneManager().SetActiveScene(m_Scene);
    
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
    
    m_CubeMesh = MeshGenerator3D::CreateCube(1.0f);
    m_SphereMesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
    m_CylinderMesh = MeshGenerator3D::CreateCylinder(0.5f, 1.0f, 16);
    m_PlaneMesh = MeshGenerator3D::CreatePlane(40.0f, 40.0f, 1, 1);
    
    SetupScene();
    
    GE_INFO("Physics Demo initialized!");
}

void PhysicsDemoApp::OnUpdate(float deltaTime) {
    glm::vec2 currentMousePos = Input::GetMousePosition();
    glm::vec2 mouseDelta = currentMousePos - m_LastMousePos;
    
    if (Input::IsMouseButtonPressed(MouseButton::Middle) ||
        (Input::IsMouseButtonPressed(MouseButton::Left) && !ImGui::GetIO().WantCaptureMouse)) {
        if (!m_LeftMouseDown) {
            m_LeftMouseDown = true;
        } else {
            m_CameraYaw -= mouseDelta.x * 0.005f;
            m_CameraPitch -= mouseDelta.y * 0.005f;
            m_CameraPitch = glm::clamp(m_CameraPitch, 0.1f, 1.5f);
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
            m_CameraTarget = m_CameraTarget + right * (-mouseDelta.x * 0.02f)
                           + glm::vec3(0, 1, 0) * (mouseDelta.y * 0.02f);
        }
    } else {
        m_RightMouseDown = false;
    }
    
    float scrollDelta = Input::GetMouseScroll().y;
    if (std::abs(scrollDelta) > 0.001f) {
        m_CameraDistance *= std::pow(0.9f, scrollDelta);
        m_CameraDistance = glm::clamp(m_CameraDistance, 5.0f, 100.0f);
    }
    
    m_LastMousePos = currentMousePos;
    
    UpdateCamera(deltaTime);
    
    if (Input::IsKeyJustPressed(KeyCode::Space) && !ImGui::GetIO().WantCaptureKeyboard) {
        glm::vec3 spawnPos(Random::Range(-3.0f, 3.0f), 10.0f + Random::Range(0.0f, 5.0f), Random::Range(-3.0f, 3.0f));
        glm::vec3 color(Random::Range(0.3f, 1.0f), Random::Range(0.3f, 1.0f), Random::Range(0.3f, 1.0f));
        
        if (m_SpawnShape == 0) {
            SpawnBox(spawnPos, glm::vec3(1.0f), color, m_SpawnMass);
        } else if (m_SpawnShape == 1) {
            SpawnSphere(spawnPos, 0.5f, color, m_SpawnMass);
        } else {
            SpawnCylinder(spawnPos, 0.4f, 1.0f, color, m_SpawnMass);
        }
    }
    
    if (Input::IsKeyJustPressed(KeyCode::R)) {
        SetupScene();
    }
    
    if (Input::IsKeyJustPressed(KeyCode::P)) {
        m_Paused = !m_Paused;
    }
    
    if (Input::IsKeyJustPressed(KeyCode::D1)) {
        m_DemoMode = 0;
        SetupScene();
    }
    if (Input::IsKeyJustPressed(KeyCode::D2)) {
        m_DemoMode = 1;
        SetupScene();
    }
    if (Input::IsKeyJustPressed(KeyCode::D3)) {
        m_DemoMode = 2;
        SetupScene();
    }
}

void PhysicsDemoApp::OnRender() {
    RenderCommand::SetClearColor({0.15f, 0.15f, 0.2f, 1.0f});
    RenderCommand::Clear();
    
    DebugDraw::DrawGrid(20.0f, 20);
    
    DebugDraw::Update(Time::GetDeltaTime());
    auto scene = Application::Get().GetSceneManager().GetActiveScene();
    if (scene) {
        Camera3D* cam = scene->GetMainCamera();
        if (cam) DebugDraw::Render(*cam);
    }
    
    RenderUI();
}

void PhysicsDemoApp::OnShutdown() {
    DebugDraw::Shutdown();
    UIRenderer::Shutdown();
}

void PhysicsDemoApp::SetupScene() {
    auto entities = m_Scene->GetAllEntities();
    for (auto& entity : entities) {
        if (entity != m_CameraEntity) {
            m_Scene->DestroyEntity(entity);
        }
    }
    m_DynamicObjects.clear();
    
    SetupCamera();
    CreateGround();
    
    if (m_DemoMode == 0) {
        CreateStack(glm::vec3(0, 0.5f, 0), 5);
    } else if (m_DemoMode == 1) {
        CreatePyramid(glm::vec3(0, 0.5f, 0), 6);
    } else if (m_DemoMode == 2) {
        CreateDominos(glm::vec3(-8, 0.5f, 0), 20);
    }
}

void PhysicsDemoApp::CreateGround() {
    Entity ground = m_Scene->CreateEntity("Ground");
    
    auto& transform = ground.AddComponent<TransformComponent>();
    transform.Position = glm::vec3(0, 0, 0);
    transform.Scale = glm::vec3(1, 1, 1);
    
    auto material = CreateRef<Material>(m_BasicShader);
    material->SetVec3("u_Color", glm::vec3(0.4f, 0.5f, 0.4f));
    material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    ground.AddComponent<MeshRendererComponent>(m_PlaneMesh, material);
    
    auto& rb = ground.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Static;
    rb.UseGravity = false;
    
    auto& collider = ground.AddComponent<ColliderComponent>();
    auto planeShape = CreateRef<Physics::PlaneShape>(glm::vec3(0, 1, 0), 0.0f);
    collider.SetShape(planeShape);
}

Entity PhysicsDemoApp::SpawnBox(const glm::vec3& pos, const glm::vec3& size, const glm::vec3& color, float mass) {
    Entity obj = m_Scene->CreateEntity("Box");
    
    auto& transform = obj.AddComponent<TransformComponent>();
    transform.Position = pos;
    transform.Scale = size;
    transform.Rotation = glm::quat(glm::vec3(
        Random::Range(0.0f, 0.5f),
        Random::Range(0.0f, 0.5f),
        Random::Range(0.0f, 0.5f)
    ));
    
    auto material = CreateRef<Material>(m_BasicShader);
    material->SetVec3("u_Color", color);
    material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    obj.AddComponent<MeshRendererComponent>(m_CubeMesh, material);
    
    auto& rb = obj.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Dynamic;
    rb.Mass = mass;
    rb.UseGravity = true;
    
    auto& collider = obj.AddComponent<ColliderComponent>();
    auto boxShape = CreateRef<Physics::BoxShape>(size * 0.5f);
    collider.SetShape(boxShape);
    
    m_DynamicObjects.push_back(obj);
    return obj;
}

Entity PhysicsDemoApp::SpawnSphere(const glm::vec3& pos, float radius, const glm::vec3& color, float mass) {
    Entity obj = m_Scene->CreateEntity("Sphere");
    
    auto& transform = obj.AddComponent<TransformComponent>();
    transform.Position = pos;
    transform.Scale = glm::vec3(radius * 2.0f);
    
    auto material = CreateRef<Material>(m_BasicShader);
    material->SetVec3("u_Color", color);
    material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    obj.AddComponent<MeshRendererComponent>(m_SphereMesh, material);
    
    auto& rb = obj.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Dynamic;
    rb.Mass = mass;
    rb.UseGravity = true;
    
    auto& collider = obj.AddComponent<ColliderComponent>();
    auto sphereShape = CreateRef<Physics::SphereShape>(radius);
    collider.SetShape(sphereShape);
    
    m_DynamicObjects.push_back(obj);
    return obj;
}

Entity PhysicsDemoApp::SpawnCylinder(const glm::vec3& pos, float radius, float height, const glm::vec3& color, float mass) {
    Entity obj = m_Scene->CreateEntity("Cylinder");
    
    auto& transform = obj.AddComponent<TransformComponent>();
    transform.Position = pos;
    transform.Scale = glm::vec3(radius * 2.0f, height, radius * 2.0f);
    
    auto material = CreateRef<Material>(m_BasicShader);
    material->SetVec3("u_Color", color);
    material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
    material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    
    obj.AddComponent<MeshRendererComponent>(m_CylinderMesh, material);
    
    auto& rb = obj.AddComponent<RigidBodyComponent>();
    rb.Type = RigidBodyComponent::BodyType::Dynamic;
    rb.Mass = mass;
    rb.UseGravity = true;
    
    auto& collider = obj.AddComponent<ColliderComponent>();
    auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(radius, height * 0.5f, radius));
    collider.SetShape(boxShape);
    
    m_DynamicObjects.push_back(obj);
    return obj;
}

void PhysicsDemoApp::CreateStack(const glm::vec3& basePos, int layers) {
    float boxSize = 1.0f;
    for (int y = 0; y < layers; y++) {
        for (int x = 0; x < layers - y; x++) {
            for (int z = 0; z < layers - y; z++) {
                glm::vec3 pos = basePos + glm::vec3(
                    (x - (layers - y - 1) * 0.5f) * boxSize * 1.05f,
                    y * boxSize * 1.02f + boxSize * 0.5f,
                    (z - (layers - y - 1) * 0.5f) * boxSize * 1.05f
                );
                
                float t = (float)y / layers;
                glm::vec3 color = glm::mix(
                    glm::vec3(0.8f, 0.3f, 0.2f),
                    glm::vec3(0.2f, 0.3f, 0.8f),
                    t
                );
                
                SpawnBox(pos, glm::vec3(boxSize), color, 1.0f);
            }
        }
    }
}

void PhysicsDemoApp::CreatePyramid(const glm::vec3& basePos, int baseSize) {
    float boxSize = 1.0f;
    for (int y = 0; y < baseSize; y++) {
        int layerSize = baseSize - y;
        float offset = (baseSize - 1 - y) * 0.5f;
        
        for (int x = 0; x < layerSize; x++) {
            for (int z = 0; z < layerSize; z++) {
                glm::vec3 pos = basePos + glm::vec3(
                    (x - offset) * boxSize * 1.02f,
                    y * boxSize * 1.02f + boxSize * 0.5f,
                    (z - offset) * boxSize * 1.02f
                );
                
                float t = (float)y / baseSize;
                glm::vec3 color = glm::mix(
                    glm::vec3(0.9f, 0.7f, 0.2f),
                    glm::vec3(0.9f, 0.2f, 0.1f),
                    t
                );
                
                SpawnBox(pos, glm::vec3(boxSize), color, 1.0f);
            }
        }
    }
}

void PhysicsDemoApp::CreateDominos(const glm::vec3& startPos, int count) {
    float spacing = 1.2f;
    float dominoWidth = 0.2f;
    float dominoHeight = 2.0f;
    float dominoDepth = 1.0f;
    
    for (int i = 0; i < count; i++) {
        glm::vec3 pos = startPos + glm::vec3(i * spacing, dominoHeight * 0.5f, 0);
        
        float t = (float)i / count;
        glm::vec3 color = glm::mix(
            glm::vec3(0.2f, 0.8f, 0.3f),
            glm::vec3(0.3f, 0.2f, 0.8f),
            t
        );
        
        Entity domino = m_Scene->CreateEntity("Domino");
        
        auto& transform = domino.AddComponent<TransformComponent>();
        transform.Position = pos;
        transform.Scale = glm::vec3(dominoWidth, dominoHeight, dominoDepth);
        
        auto material = CreateRef<Material>(m_BasicShader);
        material->SetVec3("u_Color", color);
        material->SetVec3("u_LightDir", glm::normalize(glm::vec3(1, -1, 1)));
        material->SetVec3("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        
        domino.AddComponent<MeshRendererComponent>(m_CubeMesh, material);
        
        auto& rb = domino.AddComponent<RigidBodyComponent>();
        rb.Type = RigidBodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f;
        rb.UseGravity = true;
        
        auto& collider = domino.AddComponent<ColliderComponent>();
        auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(dominoWidth * 0.5f, dominoHeight * 0.5f, dominoDepth * 0.5f));
        collider.SetShape(boxShape);
        
        m_DynamicObjects.push_back(domino);
    }
    
    Entity ball = SpawnSphere(startPos + glm::vec3(-3.0f, 1.5f, 0), 0.5f, glm::vec3(1, 0, 0), 5.0f);
    if (ball.IsValid() && ball.HasComponent<RigidBodyComponent>()) {
        auto& rb = ball.GetComponent<RigidBodyComponent>();
        rb.LinearVelocity = glm::vec3(8.0f, 0, 0);
    }
}

void PhysicsDemoApp::SetupCamera() {
    if (!m_CameraEntity.IsValid()) {
        m_CameraEntity = m_Scene->CreateEntity("MainCamera");
        m_CameraEntity.AddComponent<CameraComponent>();
        m_Scene->SetMainCamera(m_CameraEntity);
    }
    
    UpdateCamera(0.0f);
}

void PhysicsDemoApp::UpdateCamera(float deltaTime) {
    float cy = std::cos(m_CameraYaw), sy = std::sin(m_CameraYaw);
    float cp = std::cos(m_CameraPitch), sp = std::sin(m_CameraPitch);
    glm::vec3 dir(cp * cy, sp, cp * sy);
    glm::vec3 eyePos = m_CameraTarget + dir * m_CameraDistance;
    
    auto& transform = m_CameraEntity.GetComponent<TransformComponent>();
    transform.Position = eyePos;
    transform.LookAt(m_CameraTarget);
}

void PhysicsDemoApp::RenderUI() {
    UIRenderer::BeginFrame();
    
    ImGui::Begin("Physics Demo");
    ImGui::Text("FPS: %.1f", Time::GetFPS());
    ImGui::Text("Objects: %d", (int)m_DynamicObjects.size());
    
    ImGui::Separator();
    
    const char* demos[] = { "Stack (1)", "Pyramid (2)", "Dominos (3)" };
    if (ImGui::Combo("Demo Mode", &m_DemoMode, demos, 3)) {
        SetupScene();
    }
    
    ImGui::Separator();
    ImGui::Text("Spawn Settings:");
    const char* shapes[] = { "Box", "Sphere", "Cylinder" };
    ImGui::Combo("Shape", &m_SpawnShape, shapes, 3);
    ImGui::SliderFloat("Mass", &m_SpawnMass, 0.1f, 10.0f);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("Space: Spawn object");
    ImGui::BulletText("R: Reset scene");
    ImGui::BulletText("P: Pause/Resume");
    ImGui::BulletText("1/2/3: Switch demo");
    ImGui::BulletText("Left/Middle Mouse: Orbit");
    ImGui::BulletText("Right Mouse: Pan");
    ImGui::BulletText("Scroll: Zoom");
    
    if (ImGui::Button("Reset Scene (R)")) {
        SetupScene();
    }
    
    ImGui::End();
    
    UIRenderer::EndFrame();
}

void PhysicsDemoApp::CreateFallingObjects() {
    for (int i = 0; i < 5; ++i) {
        glm::vec3 pos(Random::Range(-3.0f, 3.0f), 10.0f + i * 2.0f, Random::Range(-3.0f, 3.0f));
        glm::vec3 color(Random::Range(0.3f, 1.0f), Random::Range(0.3f, 1.0f), Random::Range(0.3f, 1.0f));
        
        if (i % 2 == 0) {
            SpawnBox(pos, glm::vec3(1.0f), color, 1.0f);
        } else {
            SpawnSphere(pos, 0.5f, color, 1.0f);
        }
    }
}

GameEngine::Application* GameEngine::CreateApplication() {
    return new PhysicsDemoApp();
}
