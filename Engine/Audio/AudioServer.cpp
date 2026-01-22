#include "AudioServer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    AudioServer::AudioServer() {
    }

    void AudioServer::Initialize() {
        AudioEngine::Init();
        GE_CORE_INFO("AudioServer initialized");
    }

    void AudioServer::Shutdown() {
        AudioEngine::Shutdown();
        GE_CORE_INFO("AudioServer shutdown");
    }

    void AudioServer::PlaySound(Ref<AudioClip> sound, const glm::vec3& position) {
        if (!sound) {
            return;
        }

        // Use AudioEngine to play 3D sound
        // Note: AudioEngine::Play3D expects a filepath, so we'd need to extend it
        // For now, this is a placeholder
        if (position == glm::vec3(0.0f)) {
            // 2D sound
            // AudioEngine::Play2D(sound->GetPath(), 1.0f, false);
        } else {
            // 3D sound
            // AudioEngine::Play3D(sound->GetPath(), position, 1.0f, false);
        }
    }

    void AudioServer::SetListener(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
        m_ListenerPosition = position;
        m_ListenerForward = forward;
        m_ListenerUp = up;

        AudioEngine::SetListenerPosition(position);
        AudioEngine::SetListenerOrientation(forward, up);
    }

    void AudioServer::SetVolume(float volume) {
        AudioEngine::SetMasterVolume(volume);
    }

    float AudioServer::GetVolume() const {
        return AudioEngine::GetMasterVolume();
    }

    void AudioServer::StopAll() {
        // TODO: Implement stop all when AudioEngine supports it
    }

}

