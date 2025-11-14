#include "Scene.hpp"
#include "Components/TransformComponent.hpp"
#include "Components/MeshRendererComponent.hpp"
#include "Components/RigidBodyComponent.hpp"
#include "Components/CameraComponent.hpp"
#include "../Core/Logger.hpp"
#include "../Graphics/Renderer3D.hpp"

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
        OnStop();
        
        // Destroy all entities
        for (auto& [uuid, entityData] : m_Entities) {
            for (auto& [type, component] : entityData.Components) {
                component->OnDestroy();
                delete component->Owner;
            }
        }
        
        m_Entities.clear();
        
        GE_CORE_INFO("Scene '{0}' destroyed", m_Name);
    }

    void Scene::OnStart() {
        m_IsRunning = true;
        GE_CORE_INFO("Scene '{0}' started", m_Name);
    }

    void Scene::Update(float deltaTime) {
        if (!m_IsRunning || m_IsPaused) return;
        
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
        if (!m_IsRunning || m_IsPaused) return;
        
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
        if (!m_IsRunning) return;
        
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
    }

    void Scene::OnStop() {
        if (!m_IsRunning) return;
        
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
        
        m_Entities[uuid] = std::move(entityData);
        
        Entity entity(uuid, this);
        
        // All entities have a transform component by default
        entity.AddComponent<TransformComponent>();
        
        GE_CORE_TRACE("Entity '{0}' created (UUID: {1})", name, uuid.ToString());
        
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
            // Call OnDestroy for all components
            for (auto& [type, component] : it->second.Components) {
                component->OnDestroy();
                delete component->Owner;
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
        for (auto& [uuid, entityData] : m_Entities) {
            if (entityData.Name == name) {
                return Entity(uuid, this);
            }
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

    Scene::EntityData& Scene::GetEntityData(UUID uuid) {
        auto it = m_Entities.find(uuid);
        GE_CORE_ASSERT(it != m_Entities.end(), "Entity not found!");
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