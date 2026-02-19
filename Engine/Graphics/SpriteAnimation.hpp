#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    struct SpriteFrame {
        glm::vec2 uvMin{0.0f};
        glm::vec2 uvMax{1.0f};
        float duration = 0.1f;
    };

    class SpriteAnimation {
    public:
        void AddFrame(const glm::vec2& uvMin, const glm::vec2& uvMax, float duration);
        void CreateFromGrid(int cols, int rows, int startFrame, int frameCount, float frameDuration);
        void Update(float dt);
        void Play() { m_Playing = true; }
        void Stop() { m_Playing = false; m_CurrentFrame = 0; m_Timer = 0; }
        void SetLooping(bool loop) { m_Looping = loop; }
        bool IsPlaying() const { return m_Playing; }
        const SpriteFrame& GetCurrentFrame() const;
        int GetFrameCount() const { return static_cast<int>(m_Frames.size()); }

    private:
        std::vector<SpriteFrame> m_Frames;
        int m_CurrentFrame = 0;
        float m_Timer = 0.0f;
        bool m_Playing = false;
        bool m_Looping = true;
    };

}
