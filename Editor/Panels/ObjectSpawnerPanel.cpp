#include "ObjectSpawnerPanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/MeshGenerator3D.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Scene/Components/MeshRendererComponent.hpp"
#include "../../Engine/Scene/Components/LightComponent.hpp"
#include "../../Engine/Scene/Components/RigidBodyComponent.hpp"
#include "../../Engine/Scene/Components/ColliderComponent.hpp"
#include "../../Engine/Scene/Components/AudioSourceComponent.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>

namespace GameEngine {

    ObjectSpawnerPanel::ObjectSpawnerPanel() {
        InitializeSpawnables();
    }

    void ObjectSpawnerPanel::InitializeSpawnables() {
        // Primitives
        m_Spawnables.push_back({
            "Cube", "A basic cube mesh", "cube_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateCube(s); }
        });
        m_Spawnables.push_back({
            "Sphere", "A UV sphere mesh", "sphere_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateSphere(s); }
        });
        m_Spawnables.push_back({
            "Plane", "A flat plane mesh", "plane_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreatePlane(s); }
        });
        m_Spawnables.push_back({
            "Cylinder", "A cylinder mesh", "cylinder_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateCylinder(s); }
        });
        m_Spawnables.push_back({
            "Capsule", "A capsule mesh", "capsule_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateCapsule(s); }
        });
        m_Spawnables.push_back({
            "Cone", "A cone mesh", "cone_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateCone(s); }
        });
        m_Spawnables.push_back({
            "Torus", "A torus (donut) mesh", "torus_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateTorus(s); }
        });
        m_Spawnables.push_back({
            "Quad", "A simple quad", "quad_icon",
            SpawnCategory::Primitives,
            [this](Scene* s) { return CreateQuad(s); }
        });

        // Lights
        m_Spawnables.push_back({
            "Directional Light", "Sun-like directional light", "directional_icon",
            SpawnCategory::Lights,
            [this](Scene* s) { return CreateDirectionalLight(s); }
        });
        m_Spawnables.push_back({
            "Point Light", "Omni-directional point light", "point_icon",
            SpawnCategory::Lights,
            [this](Scene* s) { return CreatePointLight(s); }
        });
        m_Spawnables.push_back({
            "Spot Light", "Focused spotlight", "spot_icon",
            SpawnCategory::Lights,
            [this](Scene* s) { return CreateSpotLight(s); }
        });

        // Physics
        m_Spawnables.push_back({
            "Rigid Body Cube", "Dynamic physics cube", "physics_icon",
            SpawnCategory::Physics,
            [this](Scene* s) { return CreateRigidBody(s); }
        });
        m_Spawnables.push_back({
            "Static Collider", "Static collision box", "collider_icon",
            SpawnCategory::Physics,
            [this](Scene* s) { return CreateStaticCollider(s); }
        });
        m_Spawnables.push_back({
            "Trigger Zone", "Trigger volume", "trigger_icon",
            SpawnCategory::Physics,
            [this](Scene* s) { return CreateTrigger(s); }
        });

        // Effects
        m_Spawnables.push_back({
            "Particle System", "GPU particle emitter", "particles_icon",
            SpawnCategory::Effects,
            [this](Scene* s) { return CreateParticleSystem(s); }
        });
        m_Spawnables.push_back({
            "Trail Renderer", "Motion trail effect", "trail_icon",
            SpawnCategory::Effects,
            [this](Scene* s) { return CreateTrailRenderer(s); }
        });

        // Audio
        m_Spawnables.push_back({
            "Audio Source", "3D audio emitter", "audio_icon",
            SpawnCategory::Audio,
            [this](Scene* s) { return CreateAudioSource(s); }
        });
        m_Spawnables.push_back({
            "Audio Listener", "Audio listener component", "listener_icon",
            SpawnCategory::Audio,
            [this](Scene* s) { return CreateAudioListener(s); }
        });
    }

    void ObjectSpawnerPanel::OnImGuiRender() {
        if (!m_IsOpen) return;
        
        ImGui::Begin("Object Spawner");

        // Search bar
        ImGui::PushItemWidth(-1);
        ImGui::InputTextWithHint("##Search", "Search objects...", m_SearchBuffer, sizeof(m_SearchBuffer));
        ImGui::PopItemWidth();

        ImGui::Separator();

        // Category tabs
        if (ImGui::BeginTabBar("SpawnerTabs")) {
            if (ImGui::BeginTabItem("Primitives")) {
                m_SelectedCategory = SpawnCategory::Primitives;
                RenderCategoryTab(SpawnCategory::Primitives);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Lights")) {
                m_SelectedCategory = SpawnCategory::Lights;
                RenderCategoryTab(SpawnCategory::Lights);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Physics")) {
                m_SelectedCategory = SpawnCategory::Physics;
                RenderCategoryTab(SpawnCategory::Physics);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Effects")) {
                m_SelectedCategory = SpawnCategory::Effects;
                RenderCategoryTab(SpawnCategory::Effects);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Audio")) {
                m_SelectedCategory = SpawnCategory::Audio;
                RenderCategoryTab(SpawnCategory::Audio);
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    void ObjectSpawnerPanel::RenderCategoryTab(SpawnCategory category) {
        auto objects = GetObjectsInCategory(category);
        std::string searchStr = m_SearchBuffer;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);

        // Filter by search
        if (!searchStr.empty()) {
            objects.erase(
                std::remove_if(objects.begin(), objects.end(), [&searchStr](SpawnableObject* obj) {
                    std::string name = obj->Name;
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    return name.find(searchStr) == std::string::npos;
                }),
                objects.end()
            );
        }

        if (objects.empty()) {
            ImGui::TextDisabled("No objects found");
            return;
        }

        // Grid or list view
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columns = static_cast<int>(panelWidth / (m_IconSize + 20));
        if (columns < 1) columns = 1;

        ImGui::BeginChild("ObjectGrid", ImVec2(0, 0), false);

        int count = 0;
        for (auto* obj : objects) {
            RenderSpawnableItem(*obj);
            count++;
            
            if (count % columns != 0) {
                ImGui::SameLine();
            }
        }

        ImGui::EndChild();
    }

    void ObjectSpawnerPanel::RenderSpawnableItem(const SpawnableObject& item) {
        ImGui::PushID(item.Name.c_str());

        ImGui::BeginGroup();

        // Icon placeholder (colored box)
        ImVec4 color;
        switch (item.Category) {
            case SpawnCategory::Primitives: color = ImVec4(0.3f, 0.5f, 0.8f, 1.0f); break;
            case SpawnCategory::Lights: color = ImVec4(1.0f, 0.9f, 0.4f, 1.0f); break;
            case SpawnCategory::Physics: color = ImVec4(0.2f, 0.8f, 0.4f, 1.0f); break;
            case SpawnCategory::Effects: color = ImVec4(0.9f, 0.4f, 0.9f, 1.0f); break;
            case SpawnCategory::Audio: color = ImVec4(0.4f, 0.8f, 0.9f, 1.0f); break;
            default: color = ImVec4(0.5f, 0.5f, 0.5f, 1.0f); break;
        }

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Background
        drawList->AddRectFilled(
            pos,
            ImVec2(pos.x + m_IconSize, pos.y + m_IconSize),
            ImGui::ColorConvertFloat4ToU32(color),
            8.0f
        );

        // Icon text (first letter)
        ImVec2 textSize = ImGui::CalcTextSize(item.Name.substr(0, 1).c_str());
        drawList->AddText(
            ImVec2(pos.x + m_IconSize / 2 - textSize.x / 2,
                   pos.y + m_IconSize / 2 - textSize.y / 2),
            IM_COL32(255, 255, 255, 255),
            item.Name.substr(0, 1).c_str()
        );

        // Invisible button for interaction
        if (ImGui::InvisibleButton("##spawn", ImVec2(static_cast<float>(m_IconSize), static_cast<float>(m_IconSize)))) {
            SpawnObject(item);
        }

        // Hover effect
        if (ImGui::IsItemHovered()) {
            drawList->AddRect(
                pos,
                ImVec2(pos.x + m_IconSize, pos.y + m_IconSize),
                IM_COL32(255, 255, 255, 200),
                8.0f, 0, 2.0f
            );

            ImGui::SetTooltip("%s\n%s", item.Name.c_str(), item.Description.c_str());
        }

        // Drag source for spawning into viewport
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("SPAWN_OBJECT", item.Name.c_str(), item.Name.length() + 1);
            ImGui::Text("Spawn: %s", item.Name.c_str());
            ImGui::EndDragDropSource();
        }

        // Label
        ImGui::SetCursorPosX(ImGui::GetCursorPosX());
        ImGui::TextWrapped("%s", item.Name.c_str());

        ImGui::EndGroup();

        ImGui::PopID();
    }

    void ObjectSpawnerPanel::SpawnObject(const SpawnableObject& item) {
        if (!m_Scene) {
            GE_CORE_WARN("ObjectSpawner: No scene set");
            return;
        }

        Entity entity = item.SpawnFunc(m_Scene);
        
        if (entity) {
            GE_CORE_INFO("Spawned: {}", item.Name);
            
            if (m_OnEntitySpawnedCallback) {
                m_OnEntitySpawnedCallback(entity);
            }
        }
    }

    void ObjectSpawnerPanel::RegisterSpawnable(const SpawnableObject& spawnable) {
        m_Spawnables.push_back(spawnable);
    }

    std::string ObjectSpawnerPanel::GetCategoryName(SpawnCategory category) {
        switch (category) {
            case SpawnCategory::Primitives: return "Primitives";
            case SpawnCategory::Lights: return "Lights";
            case SpawnCategory::Physics: return "Physics";
            case SpawnCategory::Effects: return "Effects";
            case SpawnCategory::Audio: return "Audio";
            case SpawnCategory::UI: return "UI";
            case SpawnCategory::Custom: return "Custom";
            default: return "Unknown";
        }
    }

    std::vector<SpawnableObject*> ObjectSpawnerPanel::GetObjectsInCategory(SpawnCategory category) {
        std::vector<SpawnableObject*> result;
        for (auto& obj : m_Spawnables) {
            if (obj.Category == category) {
                result.push_back(&obj);
            }
        }
        return result;
    }

    // Primitive creation implementations
    Entity ObjectSpawnerPanel::CreateCube(Scene* scene) {
        Entity entity = scene->CreateEntity("Cube");
        auto mesh = MeshGenerator3D::CreateCube(1.0f);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateSphere(Scene* scene) {
        Entity entity = scene->CreateEntity("Sphere");
        auto mesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreatePlane(Scene* scene) {
        Entity entity = scene->CreateEntity("Plane");
        auto mesh = MeshGenerator3D::CreatePlane(10.0f, 10.0f);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateCylinder(Scene* scene) {
        Entity entity = scene->CreateEntity("Cylinder");
        auto mesh = MeshGenerator3D::CreateCylinder(0.5f, 1.0f, 32);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateCapsule(Scene* scene) {
        Entity entity = scene->CreateEntity("Capsule");
        auto mesh = MeshGenerator3D::CreateCapsule(0.5f, 1.0f, 16, 8);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateCone(Scene* scene) {
        Entity entity = scene->CreateEntity("Cone");
        auto mesh = MeshGenerator3D::CreateCone(0.5f, 1.0f, 32);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateTorus(Scene* scene) {
        Entity entity = scene->CreateEntity("Torus");
        auto mesh = MeshGenerator3D::CreateTorus(1.0f, 0.3f, 32, 16);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateQuad(Scene* scene) {
        Entity entity = scene->CreateEntity("Quad");
        auto mesh = MeshGenerator3D::CreateQuad(1.0f, 1.0f);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        return entity;
    }

    // Light creation implementations
    Entity ObjectSpawnerPanel::CreateDirectionalLight(Scene* scene) {
        Entity entity = scene->CreateEntity("Directional Light");
        auto& light = entity.AddComponent<LightComponent>();
        light.Type = LightType::Directional;
        light.Color = glm::vec3(1.0f);
        light.Intensity = 1.0f;
        
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Rotation = glm::vec3(-45.0f, 0.0f, 0.0f);
        
        return entity;
    }

    Entity ObjectSpawnerPanel::CreatePointLight(Scene* scene) {
        Entity entity = scene->CreateEntity("Point Light");
        auto& light = entity.AddComponent<LightComponent>();
        light.Type = LightType::Point;
        light.Color = glm::vec3(1.0f);
        light.Intensity = 1.0f;
        light.Range = 10.0f;
        
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Position = glm::vec3(0.0f, 2.0f, 0.0f);
        
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateSpotLight(Scene* scene) {
        Entity entity = scene->CreateEntity("Spot Light");
        auto& light = entity.AddComponent<LightComponent>();
        light.Type = LightType::Spot;
        light.Color = glm::vec3(1.0f);
        light.Intensity = 1.0f;
        light.Range = 15.0f;
        light.InnerConeAngle = glm::radians(12.5f);
        light.OuterConeAngle = glm::radians(17.5f);
        
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Position = glm::vec3(0.0f, 3.0f, 0.0f);
        transform.Rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
        
        return entity;
    }

    // Physics creation implementations
    Entity ObjectSpawnerPanel::CreateRigidBody(Scene* scene) {
        Entity entity = scene->CreateEntity("RigidBody");
        auto mesh = MeshGenerator3D::CreateCube(1.0f);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        
        auto& rb = entity.AddComponent<RigidBodyComponent>();
        rb.Type = RigidBodyComponent::BodyType::Dynamic;
        rb.Mass = 1.0f;
        
        auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(0.5f));
        entity.AddComponent<ColliderComponent>(boxShape);
        
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateStaticCollider(Scene* scene) {
        Entity entity = scene->CreateEntity("Static Collider");
        auto mesh = MeshGenerator3D::CreateCube(1.0f);
        auto& renderer = entity.AddComponent<MeshRendererComponent>();
        renderer.SetMesh(mesh);
        
        auto& rb = entity.AddComponent<RigidBodyComponent>();
        rb.Type = RigidBodyComponent::BodyType::Static;
        
        auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(0.5f));
        entity.AddComponent<ColliderComponent>(boxShape);
        
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateTrigger(Scene* scene) {
        Entity entity = scene->CreateEntity("Trigger");
        
        auto& rb = entity.AddComponent<RigidBodyComponent>();
        rb.Type = RigidBodyComponent::BodyType::Static;
        
        auto boxShape = CreateRef<Physics::BoxShape>(glm::vec3(1.0f));
        auto& collider = entity.AddComponent<ColliderComponent>(boxShape);
        collider.IsTrigger = true;
        
        return entity;
    }

    // Effect creation implementations
    Entity ObjectSpawnerPanel::CreateParticleSystem(Scene* scene) {
        Entity entity = scene->CreateEntity("Particle System");
        // TODO: Add ParticleSystemComponent when available
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateTrailRenderer(Scene* scene) {
        Entity entity = scene->CreateEntity("Trail Renderer");
        // TODO: Add TrailRendererComponent when available
        return entity;
    }

    // Audio creation implementations
    Entity ObjectSpawnerPanel::CreateAudioSource(Scene* scene) {
        Entity entity = scene->CreateEntity("Audio Source");
        entity.AddComponent<AudioSourceComponent>();
        return entity;
    }

    Entity ObjectSpawnerPanel::CreateAudioListener(Scene* scene) {
        Entity entity = scene->CreateEntity("Audio Listener");
        // TODO: Add AudioListenerComponent when available
        return entity;
    }

}
