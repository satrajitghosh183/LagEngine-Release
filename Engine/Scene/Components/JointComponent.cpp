#include "JointComponent.hpp"
#include "RigidBodyComponent.hpp"
#include "TransformComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {

    nlohmann::json JointComponent::Serialize() const {
        nlohmann::json data;
        data["componentType"] = GetTypeName();
        data["jointType"] = static_cast<int>(Type);
        data["connectedEntityID"] = ConnectedEntityID;
        data["anchorA"] = { AnchorA.x, AnchorA.y, AnchorA.z };
        data["anchorB"] = { AnchorB.x, AnchorB.y, AnchorB.z };
        data["axis"] = { Axis.x, Axis.y, Axis.z };
        data["distance"] = Distance;
        data["isSpring"] = IsSpring;
        data["springStiffness"] = SpringStiffness;
        data["springDamping"] = SpringDamping;
        data["useAngularLimits"] = UseAngularLimits;
        data["lowerAngularLimit"] = LowerAngularLimit;
        data["upperAngularLimit"] = UpperAngularLimit;
        data["useLinearLimits"] = UseLinearLimits;
        data["lowerLinearLimit"] = LowerLinearLimit;
        data["upperLinearLimit"] = UpperLinearLimit;
        data["useMotor"] = UseMotor;
        data["motorSpeed"] = MotorSpeed;
        data["maxMotorForce"] = MaxMotorForce;
        data["enabled"] = Enabled;
        data["breakOnForce"] = BreakOnForce;
        data["breakForce"] = BreakForce;
        return data;
    }
    
    void JointComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("jointType")) Type = static_cast<JointType>(data["jointType"].get<int>());
        else if (data.contains("type")) Type = static_cast<JointType>(data["type"].get<int>());  // Backward compat
        if (data.contains("connectedEntityID")) ConnectedEntityID = data["connectedEntityID"];
        if (data.contains("anchorA")) {
            AnchorA.x = data["anchorA"][0];
            AnchorA.y = data["anchorA"][1];
            AnchorA.z = data["anchorA"][2];
        }
        if (data.contains("anchorB")) {
            AnchorB.x = data["anchorB"][0];
            AnchorB.y = data["anchorB"][1];
            AnchorB.z = data["anchorB"][2];
        }
        if (data.contains("axis")) {
            Axis.x = data["axis"][0];
            Axis.y = data["axis"][1];
            Axis.z = data["axis"][2];
        }
        if (data.contains("distance")) Distance = data["distance"];
        if (data.contains("isSpring")) IsSpring = data["isSpring"];
        if (data.contains("springStiffness")) SpringStiffness = data["springStiffness"];
        if (data.contains("springDamping")) SpringDamping = data["springDamping"];
        if (data.contains("useAngularLimits")) UseAngularLimits = data["useAngularLimits"];
        if (data.contains("lowerAngularLimit")) LowerAngularLimit = data["lowerAngularLimit"];
        if (data.contains("upperAngularLimit")) UpperAngularLimit = data["upperAngularLimit"];
        if (data.contains("useLinearLimits")) UseLinearLimits = data["useLinearLimits"];
        if (data.contains("lowerLinearLimit")) LowerLinearLimit = data["lowerLinearLimit"];
        if (data.contains("upperLinearLimit")) UpperLinearLimit = data["upperLinearLimit"];
        if (data.contains("useMotor")) UseMotor = data["useMotor"];
        if (data.contains("motorSpeed")) MotorSpeed = data["motorSpeed"];
        if (data.contains("maxMotorForce")) MaxMotorForce = data["maxMotorForce"];
        if (data.contains("enabled")) Enabled = data["enabled"];
        if (data.contains("breakOnForce")) BreakOnForce = data["breakOnForce"];
        if (data.contains("breakForce")) BreakForce = data["breakForce"];
    }

    void JointComponent::OnCreate() {
        if (!GetOwnerEntity()) return;
        
        // Get scene from owner entity
        m_Scene = GetOwnerEntity().GetScene();
        
        RebuildJoint();
        
        GE_CORE_DEBUG("JointComponent::OnCreate");
    }

    void JointComponent::OnDestroy() {
        if (m_Constraint && m_Scene) {
            auto physicsWorld = m_Scene->GetPhysicsWorld();
            if (physicsWorld) {
                physicsWorld->RemoveConstraint(m_Constraint);
            }
        }
        m_Constraint.reset();
        
        GE_CORE_DEBUG("JointComponent::OnDestroy");
    }

    void JointComponent::RebuildJoint() {
        if (!m_Scene || !GetOwnerEntity()) return;
        
        // Remove existing constraint
        if (m_Constraint) {
            auto physicsWorld = m_Scene->GetPhysicsWorld();
            if (physicsWorld) {
                physicsWorld->RemoveConstraint(m_Constraint);
            }
            m_Constraint.reset();
        }
        
        // Check this entity has RigidBodyComponent
        if (!GetOwnerEntity().HasComponent<RigidBodyComponent>()) {
            GE_CORE_WARN("JointComponent: Entity has no RigidBodyComponent");
            return;
        }
        
        // Get connected entity
        Entity connectedEntity = m_Scene->GetEntityByUUID(ConnectedEntityID);
        if (!connectedEntity.IsValid() || !connectedEntity.HasComponent<RigidBodyComponent>()) {
            GE_CORE_WARN("JointComponent: Connected entity not found or has no RigidBodyComponent");
            return;
        }
        
        auto& rbA = GetOwnerEntity().GetComponent<RigidBodyComponent>();
        auto& rbB = connectedEntity.GetComponent<RigidBodyComponent>();
        
        Physics::RigidBody* bodyA = rbA.GetRigidBody().get();
        Physics::RigidBody* bodyB = rbB.GetRigidBody().get();
        
        if (!bodyA || !bodyB) {
            GE_CORE_WARN("JointComponent: RigidBody not initialized");
            return;
        }
        
        // Create joint based on type
        switch (Type) {
            case JointType::Distance: {
                auto joint = CreateRef<Physics::DistanceJoint>(
                    bodyA, bodyB, AnchorA, AnchorB, Distance
                );
                joint->IsSpring = IsSpring;
                joint->SpringStiffness = SpringStiffness;
                joint->SpringDamping = SpringDamping;
                m_Constraint = joint;
                break;
            }
            
            case JointType::Hinge: {
                auto joint = CreateRef<Physics::HingeJoint>(
                    bodyA, bodyB, AnchorA, AnchorB, Axis, Axis
                );
                joint->UseLimits = UseAngularLimits;
                joint->LowerLimit = LowerAngularLimit;
                joint->UpperLimit = UpperAngularLimit;
                joint->UseMotor = UseMotor;
                joint->MotorSpeed = MotorSpeed;
                joint->MaxMotorTorque = MaxMotorForce;
                m_Constraint = joint;
                break;
            }
            
            case JointType::Fixed: {
                auto joint = CreateRef<Physics::FixedJoint>(
                    bodyA, bodyB, AnchorA, AnchorB
                );
                m_Constraint = joint;
                break;
            }
            
            case JointType::Slider: {
                auto joint = CreateRef<Physics::SliderJoint>(
                    bodyA, bodyB, Axis
                );
                joint->UseLimits = UseLinearLimits;
                joint->LowerLimit = LowerLinearLimit;
                joint->UpperLimit = UpperLinearLimit;
                joint->UseMotor = UseMotor;
                joint->MotorSpeed = MotorSpeed;
                joint->MaxMotorForce = MaxMotorForce;
                m_Constraint = joint;
                break;
            }
        }
        
        if (m_Constraint) {
            m_Constraint->Enabled = Enabled;
            
            // Add to physics world
            auto physicsWorld = m_Scene->GetPhysicsWorld();
            if (physicsWorld) {
                physicsWorld->AddConstraint(m_Constraint);
            }
            
            GE_CORE_DEBUG("JointComponent: Created joint of type {}", static_cast<int>(Type));
        }
    }

}
