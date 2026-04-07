#pragma once

#include "Component.hpp"
#include "../../Physics/Physics2D/RigidBody2D.hpp"

namespace GameEngine {

    class RigidBody2DComponent : public Component {
    public:
        COMPONENT_TYPE(RigidBody2DComponent)

        // Serialized properties
        BodyType2D Type = BodyType2D::Dynamic;
        float Mass = 1.0f;
        float GravityScale = 1.0f;
        float LinearDamping = 0.01f;
        float AngularDamping = 0.05f;
        bool FixedRotation = false;
        bool ContinuousCollision = false;

        // Runtime
        Ref<RigidBody2D> Body;

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["type"] = static_cast<int>(Type);
            j["mass"] = Mass;
            j["gravityScale"] = GravityScale;
            j["linearDamping"] = LinearDamping;
            j["angularDamping"] = AngularDamping;
            j["fixedRotation"] = FixedRotation;
            j["continuousCollision"] = ContinuousCollision;
            return j;
        }

        void Deserialize(const nlohmann::json& data) override {
            if (data.contains("type")) Type = static_cast<BodyType2D>(data["type"].get<int>());
            if (data.contains("mass")) Mass = data["mass"].get<float>();
            if (data.contains("gravityScale")) GravityScale = data["gravityScale"].get<float>();
            if (data.contains("linearDamping")) LinearDamping = data["linearDamping"].get<float>();
            if (data.contains("angularDamping")) AngularDamping = data["angularDamping"].get<float>();
            if (data.contains("fixedRotation")) FixedRotation = data["fixedRotation"].get<bool>();
            if (data.contains("continuousCollision")) ContinuousCollision = data["continuousCollision"].get<bool>();
        }
    };

} // namespace GameEngine
