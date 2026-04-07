#pragma once

#include "../Core/Base.hpp"
#include "AudioEngine.hpp"
#include "AudioClip.hpp"
#include <string>

namespace GameEngine {
namespace AudioHelpers {

    /**
     * @brief Sound handle (similar to Wicked Engine's Sound)
     */
    struct Sound {
        Ref<AudioClip> Clip;
        std::string Path;
        
        Sound() = default;
        Sound(const Ref<AudioClip>& clip) : Clip(clip) {}
        bool IsValid() const { return Clip != nullptr; }
    };

    /**
     * @brief Sound instance handle (similar to Wicked Engine's SoundInstance)
     */
    struct SoundInstance {
        Ref<AudioClip> Clip;
        glm::vec3 Position = glm::vec3(0.0f);
        float Volume = 1.0f;
        bool Loop = false;
        bool IsPlaying = false;
        
        SoundInstance() = default;
        SoundInstance(const Ref<AudioClip>& clip) : Clip(clip) {}
        bool IsValid() const { return Clip != nullptr; }
    };

    /**
     * @brief Create a sound from file (Wicked Engine style)
     * @param filepath Path to audio file
     * @param out Output sound handle
     * @return true if successful
     */
    bool CreateSound(const std::string& filepath, Sound& out);

    /**
     * @brief Create a sound instance from a sound
     * @param sound Source sound
     * @param out Output sound instance
     * @return true if successful
     */
    bool CreateSoundInstance(const Sound& sound, SoundInstance& out);

    /**
     * @brief Play a sound instance
     */
    void Play(SoundInstance& instance);

    /**
     * @brief Stop a sound instance
     */
    void Stop(SoundInstance& instance);

    /**
     * @brief Set volume for a sound instance or master volume
     * @param volume Volume [0, 1]
     * @param instance Sound instance (nullptr for master volume)
     */
    void SetVolume(float volume, SoundInstance* instance = nullptr);

}

}
