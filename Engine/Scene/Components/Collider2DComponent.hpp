#pragma once

#include "Component.hpp"
#include "../../Physics/Physics2D/Collider2D.hpp"

namespace GameEngine {

    class Collider2DComponent : public Component {
    public:
        COMPONENT_TYPE(Collider2DComponent)

        ShapeType2D ShapeType = ShapeType2D::Box;
        float Radius = 0.5f;          // For circle
        glm::vec2 Size = {1.0f, 1.0f}; // For box
        glm::vec2 Offset = {0, 0};
        bool IsTrigger = false;
        float Friction = 0.3f;
        float Restitution = 0.3f;
        uint16_t CollisionLayer = 1;
        uint16_t CollisionMask = 0xFFFF;

        // Runtime
        Ref<Collider2D> Collider;

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["shapeType"] = static_cast<int>(ShapeType);
            j["radius"] = Radius;
            j["size"] = {Size.x, Size.y};
            j["offset"] = {Offset.x, Offset.y};
            j["isTrigger"] = IsTrigger;
            j["friction"] = Friction;
            j["restitution"] = Restitution;
            j["collisionLayer"] = CollisionLayer;
            j["collisionMask"] = CollisionMask;
            return j;
        }

        void Deserialize(const nlohmann::json& data) override {
            if (data.contains("shapeType")) ShapeType = static_cast<ShapeType2D>(data["shapeType"].get<int>());
            if (data.contains("radius")) Radius = data["radius"].get<float>();
            if (data.contains("size")) Size = {data["size"][0].get<float>(), data["size"][1].get<float>()};
            if (data.contains("offset")) Offset = {data["offset"][0].get<float>(), data["offset"][1].get<float>()};
            if (data.contains("isTrigger")) IsTrigger = data["isTrigger"].get<bool>();
            if (data.contains("friction")) Friction = data["friction"].get<float>();
            if (data.contains("restitution")) Restitution = data["restitution"].get<float>();
            if (data.contains("collisionLayer")) CollisionLayer = data["collisionLayer"].get<uint16_t>();
            if (data.contains("collisionMask")) CollisionMask = data["collisionMask"].get<uint16_t>();
        }
    };

} // namespace GameEngine
