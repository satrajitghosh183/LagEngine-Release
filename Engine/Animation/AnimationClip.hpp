#pragma once

#include "../Core/Base.hpp"
#include "Bone.hpp"
#include <string>
#include <vector>

namespace GameEngine {

    /**
     * @brief Animation clip
     */
    class AnimationClip {
    public:
        AnimationClip(const std::string& name, float duration, float ticksPerSecond);
        
        /**
         * @brief Add animation channel
         */
        void AddChannel(const AnimationChannel& channel);
        
        /**
         * @brief Get name
         */
        const std::string& GetName() const { return m_Name; }
        
        /**
         * @brief Get duration
         */
        float GetDuration() const { return m_Duration; }
        
        /**
         * @brief Get ticks per second
         */
        float GetTicksPerSecond() const { return m_TicksPerSecond; }
        
        /**
         * @brief Get channels
         */
        const std::vector<AnimationChannel>& GetChannels() const { return m_Channels; }
        
    private:
        std::string m_Name;
        float m_Duration;
        float m_TicksPerSecond;
        std::vector<AnimationChannel> m_Channels;
    };
}