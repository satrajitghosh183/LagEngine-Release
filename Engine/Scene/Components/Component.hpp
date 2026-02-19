#pragma once

#include "../../Core/Base.hpp"
#include "../../Core/UUID.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GameEngine {

    class Entity;
    class Scene;

    /**
     * @brief Base component class
     * 
     * All components inherit from this
     * Provides serialization interface and common functionality
     */
    class Component {
    public:
        virtual ~Component() = default;
        
        /**
         * @brief Get component type name
         */
        virtual std::string GetTypeName() const = 0;
        
        /**
         * @brief Serialize component to JSON
         */
        virtual nlohmann::json Serialize() const = 0;
        
        /**
         * @brief Deserialize component from JSON
         */
        virtual void Deserialize(const nlohmann::json& data) = 0;
        
        /**
         * @brief Called when component is added to entity
         */
        virtual void OnCreate() {}
        
        /**
         * @brief Called every frame
         */
        virtual void OnUpdate(float deltaTime) {}
        
        /**
         * @brief Called every fixed timestep
         */
        virtual void OnFixedUpdate(float fixedDeltaTime) {}
        
        /**
         * @brief Called when component is removed
         */
        virtual void OnDestroy() {}
        
        /**
         * @brief Enable/disable component
         */
        bool Enabled = true;
        
        /**
         * @brief Owner entity UUID and scene (set by scene). Use GetOwnerEntity() to get Entity.
         */
        UUID OwnerUUID = 0;
        Scene* OwnerScene = nullptr;
        
        /**
         * @brief Get the owner entity. Returns invalid Entity if not set.
         */
        Entity GetOwnerEntity() const;
    };
}

// Component registration macro
#define COMPONENT_TYPE(type) \
    virtual std::string GetTypeName() const override { return #type; }
