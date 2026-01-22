#pragma once

#include "../Base.hpp"
#include "../../Audio/AudioClip.hpp"
#include <glm/glm.hpp>
#include <string>

namespace GameEngine {

    /**
     * @brief Audio server interface
     * 
     * Provides audio playback without exposing implementation details
     */
    class IAudioServer {
    public:
        virtual ~IAudioServer() = default;

        /**
         * @brief Initialize audio server
         */
        virtual void Initialize() = 0;

        /**
         * @brief Shutdown audio server
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Play sound
         */
        virtual void PlaySound(Ref<AudioClip> sound, const glm::vec3& position = glm::vec3(0.0f)) = 0;

        /**
         * @brief Set listener position and orientation
         */
        virtual void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) = 0;

        /**
         * @brief Set master volume (0.0 to 1.0)
         */
        virtual void SetVolume(float volume) = 0;

        /**
         * @brief Get master volume
         */
        virtual float GetVolume() const = 0;

        /**
         * @brief Stop all sounds
         */
        virtual void StopAll() = 0;
    };

}

