#include "SpriteAnimation.hpp"

namespace GameEngine {

    static SpriteFrame s_DefaultFrame;

    void SpriteAnimation::AddFrame(const glm::vec2& uvMin, const glm::vec2& uvMax, float duration) {
        m_Frames.push_back({uvMin, uvMax, duration});
    }

    void SpriteAnimation::CreateFromGrid(int cols, int rows, int startFrame, int frameCount, float frameDuration) {
        m_Frames.clear();
        float cellW = 1.0f / static_cast<float>(cols);
        float cellH = 1.0f / static_cast<float>(rows);
        for (int i = startFrame; i < startFrame + frameCount; i++) {
            int col = i % cols;
            int row = i / cols;
            glm::vec2 uvMin(col * cellW, row * cellH);
            glm::vec2 uvMax((col + 1) * cellW, (row + 1) * cellH);
            m_Frames.push_back({uvMin, uvMax, frameDuration});
        }
    }

    void SpriteAnimation::Update(float dt) {
        if (!m_Playing || m_Frames.empty()) return;
        m_Timer += dt;
        while (m_Timer >= m_Frames[m_CurrentFrame].duration) {
            m_Timer -= m_Frames[m_CurrentFrame].duration;
            m_CurrentFrame++;
            if (m_CurrentFrame >= static_cast<int>(m_Frames.size())) {
                if (m_Looping) { m_CurrentFrame = 0; }
                else { m_CurrentFrame = static_cast<int>(m_Frames.size()) - 1; m_Playing = false; break; }
            }
        }
    }

    const SpriteFrame& SpriteAnimation::GetCurrentFrame() const {
        if (m_Frames.empty()) return s_DefaultFrame;
        return m_Frames[m_CurrentFrame];
    }

}
