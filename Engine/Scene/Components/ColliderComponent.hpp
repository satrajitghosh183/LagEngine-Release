#pragma once

#include "Component.hpp"
#include "../../Physics/Shapes/CollisionShape.hpp"
#include "../../Physics/Shapes/SphereShape.hpp"
#include "../../Physics/Shapes/BoxShape.hpp"
#include "../../Physics/Shapes/CapsuleShape.hpp"
#include "../../Physics/Shapes/PlaneShape.hpp"

namespace GameEngine {

    /**
     * @brief Collider component - attaches collision shape to entity
     * 
     * Links entity to physics collision detection system
     */
    class ColliderComponent : public Component {
    public:
        ColliderComponent() = default;
        ColliderComponent(const Ref<Physics::CollisionShape>& shape);
        
        COMPONENT_TYPE(ColliderComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnDestroy() override;
        
        /**
         * @brief Get/Set collision shape
         */
        Ref<Physics::CollisionShape> GetShape() const { return m_Shape; }
        void SetShape(const Ref<Physics::CollisionShape>& shape);
        
        /**
         * @brief Collider properties
         */
        bool IsTrigger = false;  // Doesn't resolve collisions, only detects
        float Friction = 0.5f;
        float Restitution = 0.3f;  // Bounciness (0 = no bounce, 1 = perfect bounce)
        
        /**
         * @brief Collision callbacks
         */
        std::function<void(Entity)> OnCollisionEnter;
        std::function<void(Entity)> OnCollisionStay;
        std::function<void(Entity)> OnCollisionExit;
        
    private:
        Ref<Physics::CollisionShape> m_Shape;
    };
}