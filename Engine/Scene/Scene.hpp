#pragma once

#include "../Core/Base.hpp"
#include "../Core/UUID.hpp"
#include "../Core/Logger.hpp"
#include "../Physics/PhysicsWorld.hpp"
#include <stdexcept>
#include "../Graphics/Camera3D.hpp"
#include "Entity.hpp"
#include "Components/Component.hpp"
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <memory>
#include <string>

namespace GameEngine {

    /**
     * @brief Scene class - Container for all entities and systems
     * 
     * Full-featured scene management:
     * - Entity creation/destruction with unique IDs
     * - Component registry (type-safe storage)
     * - Entity hierarchy management
     * - Physics world integration
     * - Active camera management
     * - Scene update/fixed update/render loops
     * - Entity queries (by tag, component, etc.)
     * - Scene serialization support
     * 
     * Usage:
     *   auto scene = CreateRef<Scene>("MainScene");
     *   Entity player = scene->CreateEntity("Player");
     *   player.AddComponent<TransformComponent>();
     *   player.AddComponent<MeshRendererComponent>(mesh, material);
     *   
     *   scene->Update(deltaTime);
     *   scene->Render();
     */
    class Scene {
    public:
        Scene(const std::string& name = "Untitled Scene");
        ~Scene();
        
        /**
         * @brief Scene lifecycle
         */
        void OnStart();
        void Update(float deltaTime);
        void FixedUpdate(float fixedDeltaTime);
        void Render();
        void OnStop();
        
        /**
         * @brief Entity management
         */
        Entity CreateEntity(const std::string& name = "Entity");
        Entity CreateEntityWithUUID(UUID uuid, const std::string& name = "Entity");
        void DestroyEntity(Entity entity);
        Entity GetEntityByUUID(UUID uuid);
        Entity GetEntityByName(const std::string& name);
        
        /**
         * @brief Entity queries
         */
        template<typename... Components>
        std::vector<Entity> GetEntitiesWith();
        
        std::vector<Entity> GetEntitiesByTag(const std::string& tag);
        std::vector<Entity> GetAllEntities();
        
        /**
         * @brief Get entity count
         */
        size_t GetEntityCount() const { return m_Entities.size(); }

        /**
         * @brief Check if an entity exists by UUID (O(1))
         */
        bool HasEntity(UUID uuid) const { return m_Entities.find(uuid) != m_Entities.end(); }
        
        /**
         * @brief Scene properties
         */
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }
        
        UUID GetUUID() const { return m_SceneID; }
        
        /**
         * @brief Physics world
         */
        Ref<Physics::PhysicsWorld> GetPhysicsWorld() const { return m_PhysicsWorld; }
        
        /**
         * @brief Camera management
         */
        Entity GetMainCameraEntity();
        Camera3D* GetMainCamera();
        void SetMainCamera(Entity entity);
        
        /**
         * @brief Scene state
         */
        bool IsRunning() const { return m_IsRunning; }
        bool IsPaused() const { return m_IsPaused; }
        void SetPaused(bool paused) { m_IsPaused = paused; }
        
        /**
         * @brief Merge another scene into this scene
         * @param other Scene to merge from
         * @param prefix Optional prefix for entity names to avoid collisions
         */
        void Merge(const Ref<Scene>& other, const std::string& prefix = "");
        
    private:
        // Entity data structure
        struct EntityData {
            UUID ID;
            std::string Name;
            std::string Tag;
            bool Active = true;
            
            // Hierarchy
            UUID Parent = 0;
            std::vector<UUID> Children;
            
            // Components (stored by type)
            std::unordered_map<std::type_index, Scope<Component>> Components;
        };
        
        // Internal helpers
        EntityData& GetEntityData(UUID uuid);
        void UpdateEntityHierarchy(Entity entity);
        void DestroyEntityInternal(UUID uuid);
        void UpdateEntityNameIndex(UUID uuid, const std::string& newName);
        
        // Component management
        template<typename T>
        T* GetComponent(UUID uuid);
        
        template<typename T>
        bool HasComponent(UUID uuid);
        
        template<typename T, typename... Args>
        T& AddComponent(UUID uuid, Args&&... args);
        
        template<typename T>
        void RemoveComponent(UUID uuid);
        
        std::vector<Component*> GetAllComponents(UUID uuid);
        
    private:
        UUID m_SceneID;
        std::string m_Name;
        bool m_IsRunning = false;
        bool m_IsPaused = false;
        
        // Entity storage
        std::unordered_map<UUID, EntityData> m_Entities;
        std::unordered_map<std::string, UUID> m_NameIndex;

        // Physics
        Ref<Physics::PhysicsWorld> m_PhysicsWorld;
        
        // Active camera
        UUID m_MainCameraEntity = 0;
        
        friend class Entity;
        friend class SceneSerializer;
    };
    
    // Template implementations
    template<typename T>
    T* Scene::GetComponent(UUID uuid) {
        auto& entityData = GetEntityData(uuid);
        auto typeIndex = std::type_index(typeid(T));
        
        auto it = entityData.Components.find(typeIndex);
        if (it != entityData.Components.end()) {
            return static_cast<T*>(it->second.get());
        }
        
        return nullptr;
    }
    
    template<typename T>
    bool Scene::HasComponent(UUID uuid) {
        auto& entityData = GetEntityData(uuid);
        auto typeIndex = std::type_index(typeid(T));
        return entityData.Components.find(typeIndex) != entityData.Components.end();
    }
    
    template<typename T, typename... Args>
    T& Scene::AddComponent(UUID uuid, Args&&... args) {
        auto& entityData = GetEntityData(uuid);
        auto typeIndex = std::type_index(typeid(T));

        if (entityData.Components.find(typeIndex) != entityData.Components.end()) {
            GE_CORE_ERROR("Entity already has component!");
            throw std::runtime_error("Entity already has component!");
        }

        auto component = CreateScope<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        
        component->OwnerUUID = uuid;
        component->OwnerScene = this;
        
        entityData.Components[typeIndex] = std::move(component);
        
        // Call OnCreate
        componentPtr->OnCreate();
        
        return *componentPtr;
    }
    
    template<typename T>
    void Scene::RemoveComponent(UUID uuid) {
        auto& entityData = GetEntityData(uuid);
        auto typeIndex = std::type_index(typeid(T));
        
        auto it = entityData.Components.find(typeIndex);
        if (it != entityData.Components.end()) {
            it->second->OnDestroy();
            entityData.Components.erase(it);
        }
    }
    
    template<typename... Components>
    std::vector<Entity> Scene::GetEntitiesWith() {
        std::vector<Entity> result;
        
        for (auto& [uuid, entityData] : m_Entities) {
            bool hasAll = (HasComponent<Components>(uuid) && ...);
            
            if (hasAll) {
                result.emplace_back(uuid, this);
            }
        }
        
        return result;
    }
}

// Entity template implementations - must be after Scene definition
namespace GameEngine {

    template<typename T, typename... Args>
    T& Entity::AddComponent(Args&&... args) {
        if (!IsValid()) {
            GE_CORE_ERROR("AddComponent on invalid entity");
            throw std::runtime_error("Invalid entity!");
        }
        return m_Scene->AddComponent<T>(m_UUID, std::forward<Args>(args)...);
    }

    template<typename T>
    T& Entity::GetComponent() {
        if (!IsValid()) {
            GE_CORE_ERROR("GetComponent on invalid entity");
            throw std::runtime_error("Invalid entity!");
        }
        T* comp = m_Scene->GetComponent<T>(m_UUID);
        if (!comp) {
            GE_CORE_ERROR("Entity does not have component");
            throw std::runtime_error("Entity does not have component!");
        }
        return *comp;
    }

    template<typename T>
    const T& Entity::GetComponent() const {
        if (!IsValid()) {
            GE_CORE_ERROR("GetComponent on invalid entity");
            throw std::runtime_error("Invalid entity!");
        }
        T* comp = const_cast<Scene*>(m_Scene)->GetComponent<T>(m_UUID);
        if (!comp) {
            GE_CORE_ERROR("Entity does not have component");
            throw std::runtime_error("Entity does not have component!");
        }
        return *comp;
    }

    template<typename T>
    bool Entity::HasComponent() const {
        if (!IsValid()) return false;
        return const_cast<Scene*>(m_Scene)->HasComponent<T>(m_UUID);
    }

    template<typename T>
    void Entity::RemoveComponent() {
        if (!IsValid()) {
            GE_CORE_ERROR("RemoveComponent on invalid entity");
            throw std::runtime_error("Invalid entity!");
        }
        m_Scene->RemoveComponent<T>(m_UUID);
    }

}