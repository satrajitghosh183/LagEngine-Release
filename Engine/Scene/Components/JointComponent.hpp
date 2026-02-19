#pragma once

#include "Component.hpp"
#include "../../Physics/Constraints/Constraint.hpp"
#include "../../Physics/Constraints/DistanceJoint.hpp"
#include "../../Physics/Constraints/HingeJoint.hpp"
#include "../../Physics/Constraints/FixedJoint.hpp"
#include "../../Physics/Constraints/SliderJoint.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    // Forward declaration
    class Scene;

    /**
     * @brief Component for physics joints between entities
     * 
     * Supports various joint types: distance, hinge, fixed, slider.
     */
    class JointComponent : public Component {
    public:
        /**
         * @brief Joint type enumeration
         */
        enum class JointType {
            Distance = 0,
            Hinge,
            Fixed,
            Slider
        };
        
        JointComponent() = default;
        ~JointComponent() = default;
        
        COMPONENT_TYPE(JointComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Get the underlying constraint
         */
        Ref<Physics::Constraint> GetConstraint() const { return m_Constraint; }
        
        /**
         * @brief Joint configuration
         */
        JointType Type = JointType::Distance;
        
        // Connected entity (UUID as string for serialization)
        uint64_t ConnectedEntityID = 0;
        
        // Anchor points (local space)
        glm::vec3 AnchorA = glm::vec3(0.0f);
        glm::vec3 AnchorB = glm::vec3(0.0f);
        
        // For hinge/slider - axis direction (local space)
        glm::vec3 Axis = glm::vec3(0.0f, 1.0f, 0.0f);
        
        // Distance joint specific
        float Distance = -1.0f;  // -1 = auto-calculate
        bool IsSpring = false;
        float SpringStiffness = 100.0f;
        float SpringDamping = 10.0f;
        
        // Hinge joint specific
        bool UseAngularLimits = false;
        float LowerAngularLimit = -glm::pi<float>();
        float UpperAngularLimit = glm::pi<float>();
        
        // Slider joint specific
        bool UseLinearLimits = false;
        float LowerLinearLimit = -1.0f;
        float UpperLinearLimit = 1.0f;
        
        // Motor (for hinge and slider)
        bool UseMotor = false;
        float MotorSpeed = 0.0f;
        float MaxMotorForce = 100.0f;
        
        // General
        bool Enabled = true;
        bool BreakOnForce = false;
        float BreakForce = 1000.0f;
        
        /**
         * @brief Lifecycle callbacks
         */
        void OnCreate();
        void OnDestroy();
        
        /**
         * @brief Rebuild the joint (call after changing properties)
         */
        void RebuildJoint();
        
    private:
        Ref<Physics::Constraint> m_Constraint;
        Scene* m_Scene = nullptr;
    };

}
