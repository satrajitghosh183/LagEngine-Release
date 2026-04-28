#include "Scene.hpp"
#include "Components/TransformComponent.hpp"
#include <stdexcept>
#include "Components/MeshRendererComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "Components/ColliderComponent.hpp"
#include "Components/LightComponent.hpp"
#include "Components/ClothComponent.hpp"
#include "Components/JointComponent.hpp"
#include "Components/FluidEmitterComponent.hpp"
#include "Components/GPUParticleComponent.hpp"
#include "Components/ScriptComponent.hpp"
#include "../Core/Logger.hpp"
#include "../Graphics/Renderer3D.hpp"
#include "../Physics/Fluids/FluidRenderer.hpp"

namespace GameEngine {

    Scene::Scene(const std::string& name)
        : m_SceneID()
        , m_Name(name) {
        
        // Create physics world
        m_PhysicsWorld = CreateRef<Physics::PhysicsWorld>();
        m_PhysicsWorld->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
        
        GE_CORE_INFO("Scene '{0}' created (UUID: {1})", m_Name, m_SceneID.ToString());
    }

    Scene::~Scene() {
        // Only call OnStop if still running (prevents double-stop)
        if (m_IsRunning) {
            OnStop();
        }
        
        // Clear entities (OnDestroy already called in OnStop)
        m_Entities.clear();
        
        GE_CORE_INFO("Scene '{0}' destroyed", m_Name);
    }

    void Scene::OnStart() {
        m_IsRunning = true;
        
        // Call OnCreate on all components to initialize them for runtime
        for (auto& [uuid, entityData] : m_Entities) {
            if (!entityData.Active) continue;
            
            for (auto& [type, component] : entityData.Components) {
                if (component->Enabled) {
                    component->OnCreate();
                }
            }
        }
        
        GE_CORE_INFO("Scene '{0}' started", m_Name);
    }

    void Scene::Update(float deltaTime) {
        if (m_IsPaused) return;
        
        // Update all components
        for (auto& [uuid, entityData] : m_Entities) {
            if (!entityData.Active) continue;
            
            for (auto& [type, component] : entityData.Components) {
                if (component->Enabled) {
                    component->OnUpdate(deltaTime);
                }
            }
        }
    }

    void Scene::FixedUpdate(float fixedDeltaTime) {
        if (m_IsPaused) return;
        
        // Sync rigid bodies from transforms (before physics)
        for (auto& entity : GetEntitiesWith<TransformComponent, RigidBodyComponent>()) {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.Enabled) {
                rb.SyncFromTransform();
            }
        }
        
        // Update physics
        m_PhysicsWorld->Update(fixedDeltaTime);
        
        // Sync transforms from rigid bodies (after physics)
        for (auto& entity : GetEntitiesWith<TransformComponent, RigidBodyComponent>()) {
            auto& rb = entity.GetComponent<RigidBodyComponent>();
            if (rb.Enabled && rb.Type == RigidBodyComponent::BodyType::Dynamic) {
                rb.SyncToTransform();
            }
        }
        
        // Fixed update for components
        for (auto& [uuid, entityData] : m_Entities) {
            if (!entityData.Active) continue;
            
            for (auto& [type, component] : entityData.Components) {
                if (component->Enabled) {
                    component->OnFixedUpdate(fixedDeltaTime);
                }
            }
        }
    }

    void Scene::Render() {
        // Get main camera
        Camera3D* camera = GetMainCamera();
        if (!camera) {
            GE_CORE_WARN("No main camera found in scene '{0}'", m_Name);
            return;
        }
        
        // Begin scene
        Renderer3D::BeginScene(*camera);
        
        // Render all mesh renderers
        for (auto& entity : GetEntitiesWith<TransformComponent, MeshRendererComponent>()) {
            if (!entity.IsActive()) continue;
            
            auto& transform = entity.GetComponent<TransformComponent>();
            auto& meshRenderer = entity.GetComponent<MeshRendererComponent>();
            
            if (!meshRenderer.Enabled) continue;
            
            auto mesh = meshRenderer.GetMesh();
            auto material = meshRenderer.GetMaterial();
            
            if (mesh && material) {
                glm::mat4 worldTransform = transform.GetWorldTransform();
                Renderer3D::Submit(mesh, material, worldTransform);
            }
        }
        
        // End scene
        Renderer3D::EndScene();

        // -----------------------------------------------------------------------
        // SPH fluid rendering is handled by the engine's Vulkan render path,
        // which has access to the active render pass and command buffer.
        // The Scene only drives simulation; rendering happens in Renderer3D
        // via the entity-with-FluidEmitterComponent query there.
        // -----------------------------------------------------------------------

        // -----------------------------------------------------------------------
        // GPU particle rendering (compute-shader particles)
        // -----------------------------------------------------------------------
        {
            glm::mat4 viewProj = camera->GetViewProjectionMatrix();
            for (auto& entity : GetEntitiesWith<GPUParticleComponent>()) {
                if (!entity.IsActive()) continue;
                auto& gpc = entity.GetComponent<GPUParticleComponent>();
                if (gpc.Enabled && gpc.GetParticleSystem())
                    gpc.RenderParticles(viewProj);
            }
        }
    }

    void Scene::OnStop() {
        if (!m_IsRunning) return;
        
        // Call OnDestroy on all components to clean up runtime state
        for (auto& [uuid, entityData] : m_Entities) {
            for (auto& [type, component] : entityData.Components) {
                component->OnDestroy();
            }
        }
        
        m_IsRunning = false;
        GE_CORE_INFO("Scene '{0}' stopped", m_Name);
    }

    Entity Scene::CreateEntity(const std::string& name) {
        return CreateEntityWithUUID(UUID(), name);
    }

    Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name) {
        EntityData entityData;
        entityData.ID = uuid;
        entityData.Name = name;
        entityData.Active = true;

        std::string key = name;
        size_t suffix = 0;
        while (m_NameIndex.find(key) != m_NameIndex.end()) {
            key = name + "_" + std::to_string(++suffix);
        }
        entityData.Name = key;
        m_NameIndex[key] = uuid;
        
        m_Entities[uuid] = std::move(entityData);
        
        Entity entity(uuid, this);
        
        entity.AddComponent<TransformComponent>();
        
        GE_CORE_TRACE("Entity '{0}' created (UUID: {1})", key, uuid.ToString());
        
        return entity;
    }

    void Scene::DestroyEntity(Entity entity) {
        if (!entity.IsValid()) return;
        
        UUID uuid = entity.GetUUID();
        
        // Destroy children first
        auto& entityData = GetEntityData(uuid);
        std::vector<UUID> childrenCopy = entityData.Children;
        
        for (UUID childID : childrenCopy) {
            Entity child(childID, this);
            DestroyEntity(child);
        }
        
        // Remove from parent
        if (entityData.Parent != UUID(0)) {
            auto& parentData = GetEntityData(entityData.Parent);
            auto it = std::find(parentData.Children.begin(), parentData.Children.end(), uuid);
            if (it != parentData.Children.end()) {
                parentData.Children.erase(it);
            }
        }
        
        // Destroy internal
        DestroyEntityInternal(uuid);
    }

    void Scene::DestroyEntityInternal(UUID uuid) {
        auto it = m_Entities.find(uuid);
        if (it != m_Entities.end()) {
            m_NameIndex.erase(it->second.Name);
            for (auto& [type, component] : it->second.Components) {
                component->OnDestroy();
            }
            GE_CORE_TRACE("Entity '{0}' destroyed", it->second.Name);
            m_Entities.erase(it);
        }
    }

    Entity Scene::GetEntityByUUID(UUID uuid) {
        if (m_Entities.find(uuid) != m_Entities.end()) {
            return Entity(uuid, this);
        }
        return Entity();
    }

    Entity Scene::GetEntityByName(const std::string& name) {
        auto it = m_NameIndex.find(name);
        if (it != m_NameIndex.end()) {
            return Entity(it->second, this);
        }
        return Entity();
    }

    std::vector<Entity> Scene::GetEntitiesByTag(const std::string& tag) {
        std::vector<Entity> result;
        
        for (auto& [uuid, entityData] : m_Entities) {
            if (entityData.Tag == tag) {
                result.emplace_back(uuid, this);
            }
        }
        
        return result;
    }

    std::vector<Entity> Scene::GetAllEntities() {
        std::vector<Entity> result;
        
        for (auto& [uuid, entityData] : m_Entities) {
            result.emplace_back(uuid, this);
        }
        
        return result;
    }

    Entity Scene::GetMainCameraEntity() {
        if (m_MainCameraEntity != UUID(0)) {
            return GetEntityByUUID(m_MainCameraEntity);
        }
        
        // Find first camera
        for (auto& entity : GetEntitiesWith<CameraComponent>()) {
            auto& cameraComp = entity.GetComponent<CameraComponent>();
            if (cameraComp.IsMainCamera) {
                m_MainCameraEntity = entity.GetUUID();
                return entity;
            }
        }
        
        return Entity();
    }

    Camera3D* Scene::GetMainCamera() {
        Entity cameraEntity = GetMainCameraEntity();
        
        if (cameraEntity.IsValid() && cameraEntity.HasComponent<CameraComponent>()) {
            auto& cameraComp = cameraEntity.GetComponent<CameraComponent>();
            return cameraComp.GetCamera().get();
        }
        
        return nullptr;
    }

    void Scene::Merge(const Ref<Scene>& other, const std::string& prefix) {
        if (!other) {
            GE_CORE_WARN("Scene::Merge: Attempted to merge null scene");
            return;
        }

        // UUID mapping for hierarchy preservation
        std::unordered_map<UUID, UUID> uuidMap;

        // First pass: Create all entities with new UUIDs
        for (const auto& [oldUUID, oldEntityData] : other->m_Entities) {
            // Generate new UUID (always generate new to avoid collisions)
            UUID newUUID = UUID();
            
            // Check for collision and regenerate if needed
            while (m_Entities.find(newUUID) != m_Entities.end()) {
                newUUID = UUID();
            }

            uuidMap[oldUUID] = newUUID;

            // Create new entity
            std::string entityName = prefix.empty() ? oldEntityData.Name : prefix + "_" + oldEntityData.Name;
            Entity newEntity = CreateEntityWithUUID(newUUID, entityName);
            
            // Set properties
            if (!oldEntityData.Tag.empty()) {
                newEntity.SetTag(oldEntityData.Tag);
            }
            newEntity.SetActive(oldEntityData.Active);
        }

        // Second pass: Copy components and hierarchy
        for (const auto& [oldUUID, oldEntityData] : other->m_Entities) {
            UUID newUUID = uuidMap[oldUUID];
            Entity newEntity = GetEntityByUUID(newUUID);
            
            if (!newEntity.IsValid()) {
                GE_CORE_WARN("Scene::Merge: Failed to find entity with UUID {}", newUUID.ToString());
                continue;
            }

            // Copy components using serialization/deserialization
            for (const auto& [typeIndex, oldComponent] : oldEntityData.Components) {
                nlohmann::json componentData = oldComponent->Serialize();
                
                if (!componentData.contains("type")) {
                    GE_CORE_WARN("Scene::Merge: Component missing type field");
                    continue;
                }
                
                std::string type = componentData["type"];
                
                // Clone component based on type
                if (type == "TransformComponent") {
                    // Transform is already added, just deserialize
                    if (newEntity.HasComponent<TransformComponent>()) {
                        newEntity.GetComponent<TransformComponent>().Deserialize(componentData);
                    }
                }
                else if (type == "MeshRendererComponent") {
                    auto& comp = newEntity.AddComponent<MeshRendererComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "RigidBodyComponent") {
                    auto& comp = newEntity.AddComponent<RigidBodyComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "CameraComponent") {
                    auto& comp = newEntity.AddComponent<CameraComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "ColliderComponent") {
                    auto& comp = newEntity.AddComponent<ColliderComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "LightComponent") {
                    auto& comp = newEntity.AddComponent<LightComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "ClothComponent") {
                    auto& comp = newEntity.AddComponent<ClothComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "JointComponent") {
                    auto& comp = newEntity.AddComponent<JointComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "FluidEmitterComponent") {
                    auto& comp = newEntity.AddComponent<FluidEmitterComponent>();
                    comp.Deserialize(componentData);
                }
                else if (type == "ScriptComponent") {
                    auto& comp = newEntity.AddComponent<ScriptComponent>();
                    comp.Deserialize(componentData);
                }
                else {
                    GE_CORE_WARN("Scene::Merge: Unknown component type: {}", type);
                }
            }

            // Copy hierarchy (parent will be set in third pass)
            auto& newEntityData = GetEntityData(newUUID);
            if (oldEntityData.Parent != UUID(0)) {
                // Map parent UUID
                if (uuidMap.find(oldEntityData.Parent) != uuidMap.end()) {
                    newEntityData.Parent = uuidMap[oldEntityData.Parent];
                }
            }
            
            // Map children UUIDs
            for (const auto& oldChildUUID : oldEntityData.Children) {
                if (uuidMap.find(oldChildUUID) != uuidMap.end()) {
                    newEntityData.Children.push_back(uuidMap[oldChildUUID]);
                }
            }
        }

        // Third pass: Update entity hierarchy relationships
        for (const auto& [uuid, entityData] : m_Entities) {
            Entity entity = GetEntityByUUID(uuid);
            if (!entity.IsValid()) continue;
            
            if (entityData.Parent != UUID(0)) {
                Entity parentEntity = GetEntityByUUID(entityData.Parent);
                if (parentEntity.IsValid()) {
                    entity.SetParent(parentEntity);
                }
            }
        }

        GE_CORE_INFO("Scene '{0}' merged into '{1}' ({2} entities)", other->GetName(), m_Name, uuidMap.size());
    }

    void Scene::SetMainCamera(Entity entity) {
        if (!entity.IsValid() || !entity.HasComponent<CameraComponent>()) {
            GE_CORE_WARN("Attempted to set main camera to invalid entity or entity without CameraComponent");
            return;
        }
        
        // Unset previous main camera
        if (m_MainCameraEntity != UUID(0)) {
            Entity prevCamera = GetEntityByUUID(m_MainCameraEntity);
            if (prevCamera.IsValid() && prevCamera.HasComponent<CameraComponent>()) {
                prevCamera.GetComponent<CameraComponent>().IsMainCamera = false;
            }
        }
        
        // Set new main camera
        m_MainCameraEntity = entity.GetUUID();
        entity.GetComponent<CameraComponent>().IsMainCamera = true;
    }

    void Scene::UpdateEntityNameIndex(UUID uuid, const std::string& newName) {
        auto it = m_Entities.find(uuid);
        if (it == m_Entities.end()) return;
        std::string& currentName = it->second.Name;
        if (currentName == newName) return;
        m_NameIndex.erase(currentName);
        currentName = newName;
        size_t suffix = 0;
        std::string key = newName;
        while (m_NameIndex.find(key) != m_NameIndex.end() && m_NameIndex[key] != uuid) {
            key = newName + "_" + std::to_string(++suffix);
        }
        it->second.Name = key;
        m_NameIndex[key] = uuid;
    }

    Scene::EntityData& Scene::GetEntityData(UUID uuid) {
        auto it = m_Entities.find(uuid);
        if (it == m_Entities.end()) {
            GE_CORE_ERROR("Entity not found: {0}", uuid.ToString());
            throw std::runtime_error("Entity not found: " + uuid.ToString());
        }
        return it->second;
    }

    std::vector<Component*> Scene::GetAllComponents(UUID uuid) {
        std::vector<Component*> components;
        
        auto& entityData = GetEntityData(uuid);
        for (auto& [type, component] : entityData.Components) {
            components.push_back(component.get());
        }
        
        return components;
    }
}