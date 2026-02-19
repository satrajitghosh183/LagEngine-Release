#include "SpriteAnimatorComponent.hpp"

namespace GameEngine {

    void SpriteAnimatorComponent::OnCreate() {
        m_Animation.CreateFromGrid(FrameColumns, FrameRows, 0, TotalFrames, FrameDuration);
        m_Animation.SetLooping(Looping);
        if (Playing) m_Animation.Play();
    }

    void SpriteAnimatorComponent::OnUpdate(float deltaTime) {
        m_Animation.Update(deltaTime);
    }

    nlohmann::json SpriteAnimatorComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = "SpriteAnimatorComponent";
        j["frameColumns"] = FrameColumns;
        j["frameRows"] = FrameRows;
        j["totalFrames"] = TotalFrames;
        j["frameDuration"] = FrameDuration;
        j["looping"] = Looping;
        j["playing"] = Playing;
        return j;
    }

    void SpriteAnimatorComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("frameColumns")) FrameColumns = data["frameColumns"];
        if (data.contains("frameRows")) FrameRows = data["frameRows"];
        if (data.contains("totalFrames")) TotalFrames = data["totalFrames"];
        if (data.contains("frameDuration")) FrameDuration = data["frameDuration"];
        if (data.contains("looping")) Looping = data["looping"];
        if (data.contains("playing")) Playing = data["playing"];
    }

}
