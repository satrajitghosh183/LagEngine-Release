#include "AnimationClip.hpp"

namespace GameEngine {

    AnimationClip::AnimationClip(const std::string& name, float duration, float ticksPerSecond)
        : m_Name(name), m_Duration(duration), m_TicksPerSecond(ticksPerSecond) {
        
        // Ensure we have a valid ticks per second
        if (m_TicksPerSecond <= 0.0f) {
            m_TicksPerSecond = 25.0f; // Default to 25 fps
        }
    }

    void AnimationClip::AddChannel(const AnimationChannel& channel) {
        m_Channels.push_back(channel);
    }

}
