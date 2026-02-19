#pragma once

#include "Component.hpp"
#include "../../Graphics/SpriteAnimation.hpp"

namespace GameEngine {

    class SpriteAnimatorComponent : public Component {
    public:
        COMPONENT_TYPE(SpriteAnimatorComponent)

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;

        SpriteAnimation& GetAnimation() { return m_Animation; }

        int FrameColumns = 4;
        int FrameRows = 4;
        int TotalFrames = 16;
        float FrameDuration = 0.1f;
        bool Looping = true;
        bool Playing = true;

    private:
        SpriteAnimation m_Animation;
    };

}
