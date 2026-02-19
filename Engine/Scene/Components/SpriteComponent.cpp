#include "SpriteComponent.hpp"

namespace GameEngine {

    nlohmann::json SpriteComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = "SpriteComponent";
        j["color"] = {Color.r, Color.g, Color.b, Color.a};
        j["texturePath"] = TexturePath;
        j["uvMin"] = {UVMin.x, UVMin.y};
        j["uvMax"] = {UVMax.x, UVMax.y};
        j["sortOrder"] = SortOrder;
        return j;
    }

    void SpriteComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("color")) {
            auto c = data["color"];
            Color = glm::vec4(c[0], c[1], c[2], c[3]);
        }
        if (data.contains("texturePath")) TexturePath = data["texturePath"];
        if (data.contains("uvMin")) { auto u = data["uvMin"]; UVMin = {u[0], u[1]}; }
        if (data.contains("uvMax")) { auto u = data["uvMax"]; UVMax = {u[0], u[1]}; }
        if (data.contains("sortOrder")) SortOrder = data["sortOrder"];
    }

}
