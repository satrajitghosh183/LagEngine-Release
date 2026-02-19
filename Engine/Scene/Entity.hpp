#pragma once

#ifndef GAMEENGINE_ENTITY_HPP
#define GAMEENGINE_ENTITY_HPP

#include "../Core/Base.hpp"
#include "../Core/UUID.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <typeindex>

namespace GameEngine {

    // Forward declaration
    class Scene;
    class Component;

    /**
     * @brief Entity class - Game object with components
     * 
     * Full-featured Entity-Component System:
     * - Unique ID per entity
     * - Component management (add/remove/get)
     * - Parent-child hierarchy
     * - Enable/disable state
     * - Name and tag system
     * - Component iteration
     * 
     * Usage:
     *   Entity entity = scene->CreateEntity("Player");
     *   entity.AddComponent<TransformComponent>();
     *   entity.AddComponent<MeshRendererComponent>(mesh, material);
     *   
     *   auto& transform = entity.GetComponent<TransformComponent>();
     *   transform.Position = glm::vec3(0, 1, 0);
     */
    class Entity {
    public:
        Entity() = default;
        Entity(UUID id, Scene* scene);
        Entity(const Entity& other) = default;
        
        /**
         * @brief Add component to entity
         */
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);
        
        /**
         * @brief Get component from entity
         */
        template<typename T>
        T& GetComponent();
        
        template<typename T>
        const T& GetComponent() const;
        
        /**
         * @brief Check if entity has component
         */
        template<typename T>
        bool HasComponent() const;
        
        /**
         * @brief Remove component from entity
         */
        template<typename T>
        void RemoveComponent();
        
        /**
         * @brief Get all components
         */
        std::vector<Component*> GetAllComponents();
        
        /**
         * @brief Entity hierarchy
         */
        void SetParent(Entity parent);
        Entity GetParent() const;
        void AddChild(Entity child);
        void RemoveChild(Entity child);
        std::vector<Entity> GetChildren() const;
        bool HasParent() const;
        
        /**
         * @brief Entity properties
         */
        UUID GetUUID() const { return m_UUID; }
        const std::string& GetName() const;
        void SetName(const std::string& name);
        
        const std::string& GetTag() const;
        void SetTag(const std::string& tag);
        
        bool IsActive() const;
        void SetActive(bool active);
        
        /**
         * @brief Destroy entity
         */
        void Destroy();
        
        /**
         * @brief Check if entity is valid (scene exists, UUID non-zero, and entity still in scene)
         */
        bool IsValid() const;
        
        /**
         * @brief Get the scene this entity belongs to
         */
        Scene* GetScene() const { return m_Scene; }
        
        operator bool() const { return IsValid(); }
        operator UUID() const { return m_UUID; }
        
        bool operator==(const Entity& other) const {
            return m_UUID == other.m_UUID && m_Scene == other.m_Scene;
        }
        
        bool operator!=(const Entity& other) const {
            return !(*this == other);
        }
        
    private:
        UUID m_UUID;
        Scene* m_Scene = nullptr;
        
        friend class Scene;
    };
}

#endif // GAMEENGINE_ENTITY_HPP