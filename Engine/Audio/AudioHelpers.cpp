#include "AudioHelpers.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {
namespace AudioHelpers {

    bool CreateSound(const std::string& filepath, Sound& out) {
        auto clip = AudioEngine::LoadClip(filepath);
        if (!clip) {
            GE_CORE_ERROR("AudioHelpers::CreateSound: Failed to load sound: {}", filepath);
            return false;
        }
        
        out.Clip = clip;
        out.Path = filepath;
        return true;
    }

    bool CreateSoundInstance(const Sound& sound, SoundInstance& out) {
        if (!sound.IsValid()) {
            GE_CORE_ERROR("AudioHelpers::CreateSoundInstance: Invalid sound");
            return false;
        }
        
        out.Clip = sound.Clip;
        return true;
    }

    void Play(SoundInstance& instance) {
        if (!instance.IsValid()) {
            GE_CORE_WARN("AudioHelpers::Play: Invalid sound instance");
            return;
        }
        
        // Play 2D sound (non-spatial)
        AudioEngine::Play2D(instance.Clip->GetPath(), instance.Volume, instance.Loop);
        instance.IsPlaying = true;
    }

    void Stop(SoundInstance& instance) {
        if (!instance.IsValid()) {
            return;
        }
        AudioEngine::StopClip(instance.Clip->GetPath());
        instance.IsPlaying = false;
    }

    void SetVolume(float volume, SoundInstance* instance) {
        if (instance) {
            instance->Volume = glm::clamp(volume, 0.0f, 1.0f);
        } else {
            // Set master volume
            AudioEngine::SetMasterVolume(glm::clamp(volume, 0.0f, 1.0f));
        }
    }

}

}
