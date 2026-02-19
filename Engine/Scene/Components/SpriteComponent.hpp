#pragma once

#include "Component.hpp"
#include <glm/glm.hpp>
#include <string>

namespace GameEngine {

    class SpriteComponent : public Component {
    public:
        COMPONENT_TYPE(SpriteComponent)

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;

        glm::vec4 Color = glm::vec4(1.0f);
        std::string TexturePath;
        glm::vec2 UVMin = {0.0f, 0.0f};
        glm::vec2 UVMax = {1.0f, 1.0f};
        int SortOrder = 0;
    };

}
