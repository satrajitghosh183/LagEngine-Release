#pragma once

#include "../Core/Runtime/IAudioServer.hpp"
#include "AudioEngine.hpp"
#include "AudioClip.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Audio server implementation
     * 
     * Wraps AudioEngine behind IAudioServer interface
     */
    class AudioServer : public IAudioServer {
    public:
        AudioServer();
        ~AudioServer() override = default;

        void Initialize() override;
        void Shutdown() override;

        void PlaySound(Ref<AudioClip> sound, const glm::vec3& position = glm::vec3(0.0f)) override;

        void SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) override;

        void SetVolume(float volume) override;
        float GetVolume() const override;

        void StopAll() override;

    private:
        glm::vec3 m_ListenerPosition;
        glm::vec3 m_ListenerForward;
        glm::vec3 m_ListenerUp;
    };

}

