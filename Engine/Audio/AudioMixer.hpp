#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

namespace GameEngine {

    enum class AudioEffectType {
        Reverb,
        Echo,
        LowPass,
        HighPass,
        Chorus,
        Distortion
    };

    struct AudioEffectParams {
        AudioEffectType Type = AudioEffectType::Reverb;

        // Reverb
        float ReverbDecayTime = 1.5f;
        float ReverbDensity = 0.8f;
        float ReverbDiffusion = 1.0f;

        // Echo
        float EchoDelay = 0.1f;
        float EchoFeedback = 0.5f;
        float EchoDamping = 0.5f;

        // Low/High pass
        float FilterCutoff = 5000.0f;
        float FilterResonance = 1.0f;

        // Chorus
        float ChorusRate = 1.1f;
        float ChorusDepth = 0.1f;

        // Distortion
        float DistortionGain = 0.5f;
        float DistortionEdge = 0.2f;

        float WetMix = 0.3f;
    };

    class AudioBus {
    public:
        AudioBus(const std::string& name = "Master");
        ~AudioBus() = default;

        void setName(const std::string& name) { m_Name = name; }
        const std::string& getName() const { return m_Name; }

        void setVolume(float volume) { m_Volume = glm::clamp(volume, 0.0f, 2.0f); }
        float getVolume() const { return m_Volume; }

        void setMuted(bool muted) { m_Muted = muted; }
        bool isMuted() const { return m_Muted; }

        void setSolo(bool solo) { m_Solo = solo; }
        bool isSolo() const { return m_Solo; }

        // Effects chain
        void addEffect(const AudioEffectParams& effect);
        void removeEffect(int index);
        void clearEffects();
        const std::vector<AudioEffectParams>& getEffects() const { return m_Effects; }

        // Parent bus (for hierarchical mixing)
        void setParent(AudioBus* parent) { m_Parent = parent; }
        AudioBus* getParent() const { return m_Parent; }

        // Effective volume (considering parent hierarchy and mute)
        float getEffectiveVolume() const;

        // Fade
        void fadeTo(float targetVolume, float duration);
        void update(float dt);

    private:
        std::string m_Name;
        float m_Volume = 1.0f;
        bool m_Muted = false;
        bool m_Solo = false;
        AudioBus* m_Parent = nullptr;

        std::vector<AudioEffectParams> m_Effects;

        // Fade state
        bool m_Fading = false;
        float m_FadeFrom = 1.0f;
        float m_FadeTo = 1.0f;
        float m_FadeDuration = 0.0f;
        float m_FadeTimer = 0.0f;
    };

    class AudioMixer {
    public:
        AudioMixer();
        ~AudioMixer() = default;

        // Initialize with default busses
        void init();

        // Update fades and effects
        void update(float dt);

        // Bus management
        AudioBus* createBus(const std::string& name, const std::string& parentBus = "Master");
        AudioBus* getBus(const std::string& name);
        AudioBus* getMasterBus() { return &m_MasterBus; }
        const std::unordered_map<std::string, Scope<AudioBus>>& getBusses() const { return m_Busses; }

        // Convenience: set volume on a named bus
        void setBusVolume(const std::string& busName, float volume);
        float getBusVolume(const std::string& busName) const;

        // Snapshot system (save/restore entire mixer state)
        struct MixerSnapshot {
            std::unordered_map<std::string, float> BusVolumes;
            std::unordered_map<std::string, bool> BusMuted;
        };

        MixerSnapshot takeSnapshot() const;
        void applySnapshot(const MixerSnapshot& snapshot, float transitionTime = 0.0f);

        // Master volume shortcut
        void setMasterVolume(float volume) { m_MasterBus.setVolume(volume); }
        float getMasterVolume() const { return m_MasterBus.getVolume(); }

    private:
        AudioBus m_MasterBus;
        std::unordered_map<std::string, Scope<AudioBus>> m_Busses;
    };

}
