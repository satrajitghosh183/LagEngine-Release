#pragma once

#include "../Core/Base.hpp"
#include "AudioClip.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Audio source - plays audio clips
     * 
     * Can be 2D or 3D positional
     */
    class AudioSource {
    public:
        AudioSource();
        ~AudioSource();
        
        /**
         * @brief Set audio clip
         */
        void SetClip(const Ref<AudioClip>& clip);
        
        /**
         * @brief Play audio
         */
        void Play();
        
        /**
         * @brief Pause audio
         */
        void Pause();
        
        /**
         * @brief Stop audio
         */
        void Stop();
        
        /**
         * @brief Check if playing
         */
        bool IsPlaying() const;
        
        /**
         * @brief Set volume [0, 1]
         */
        void SetVolume(float volume);
        float GetVolume() const { return m_Volume; }
        
        /**
         * @brief Set pitch (1.0 = normal)
         */
        void SetPitch(float pitch);
        float GetPitch() const { return m_Pitch; }
        
        /**
         * @brief Set looping
         */
        void SetLoop(bool loop);
        bool IsLooping() const { return m_Loop; }
        
        /**
         * @brief Set 3D position
         */
        void SetPosition(const glm::vec3& position);
        glm::vec3 GetPosition() const { return m_Position; }
        
        /**
         * @brief Set velocity (for Doppler)
         */
        void SetVelocity(const glm::vec3& velocity);
        
        /**
         * @brief Set if spatial (3D) or not
         */
        void SetSpatial(bool spatial);
        bool IsSpatial() const { return m_Spatial; }
        
        /**
         * @brief Set attenuation parameters
         */
        void SetRolloffFactor(float rolloff);
        void SetReferenceDistance(float distance);
        void SetMaxDistance(float distance);
        
    private:
        uint32_t m_SourceID;
        Ref<AudioClip> m_Clip;
        
        float m_Volume;
        float m_Pitch;
        bool m_Loop;
        bool m_Spatial;
        glm::vec3 m_Position;
    };
}