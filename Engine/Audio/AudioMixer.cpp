#include "AudioMixer.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace GameEngine {

    // =====================================================================
    // AudioBus
    // =====================================================================

    AudioBus::AudioBus(const std::string& name) : m_Name(name) {}

    float AudioBus::getEffectiveVolume() const {
        if (m_Muted) return 0.0f;
        float vol = m_Volume;
        if (m_Parent) {
            vol *= m_Parent->getEffectiveVolume();
        }
        return vol;
    }

    void AudioBus::addEffect(const AudioEffectParams& effect) {
        m_Effects.push_back(effect);
    }

    void AudioBus::removeEffect(int index) {
        if (index >= 0 && index < static_cast<int>(m_Effects.size())) {
            m_Effects.erase(m_Effects.begin() + index);
        }
    }

    void AudioBus::clearEffects() {
        m_Effects.clear();
    }

    void AudioBus::fadeTo(float targetVolume, float duration) {
        if (duration <= 0.0f) {
            m_Volume = targetVolume;
            m_Fading = false;
            return;
        }
        m_FadeFrom = m_Volume;
        m_FadeTo = targetVolume;
        m_FadeDuration = duration;
        m_FadeTimer = 0.0f;
        m_Fading = true;
    }

    void AudioBus::update(float dt) {
        if (!m_Fading) return;

        m_FadeTimer += dt;
        float t = glm::clamp(m_FadeTimer / m_FadeDuration, 0.0f, 1.0f);

        // Smooth step for natural-sounding fade
        t = t * t * (3.0f - 2.0f * t);
        m_Volume = glm::mix(m_FadeFrom, m_FadeTo, t);

        if (m_FadeTimer >= m_FadeDuration) {
            m_Volume = m_FadeTo;
            m_Fading = false;
        }
    }

    // =====================================================================
    // AudioMixer
    // =====================================================================

    AudioMixer::AudioMixer() : m_MasterBus("Master") {}

    void AudioMixer::init() {
        // Create default busses
        createBus("Music", "Master");
        createBus("SFX", "Master");
        createBus("Voice", "Master");
        createBus("UI", "Master");
        createBus("Ambient", "Master");
    }

    void AudioMixer::update(float dt) {
        m_MasterBus.update(dt);
        for (auto& [name, bus] : m_Busses) {
            bus->update(dt);
        }
    }

    AudioBus* AudioMixer::createBus(const std::string& name, const std::string& parentBus) {
        auto bus = CreateScope<AudioBus>(name);

        // Set parent
        if (parentBus == "Master") {
            bus->setParent(&m_MasterBus);
        } else {
            auto it = m_Busses.find(parentBus);
            if (it != m_Busses.end()) {
                bus->setParent(it->second.get());
            } else {
                bus->setParent(&m_MasterBus);
            }
        }

        AudioBus* ptr = bus.get();
        m_Busses[name] = std::move(bus);
        return ptr;
    }

    AudioBus* AudioMixer::getBus(const std::string& name) {
        if (name == "Master") return &m_MasterBus;
        auto it = m_Busses.find(name);
        return (it != m_Busses.end()) ? it->second.get() : nullptr;
    }

    void AudioMixer::setBusVolume(const std::string& busName, float volume) {
        AudioBus* bus = getBus(busName);
        if (bus) bus->setVolume(volume);
    }

    float AudioMixer::getBusVolume(const std::string& busName) const {
        if (busName == "Master") return m_MasterBus.getVolume();
        auto it = m_Busses.find(busName);
        return (it != m_Busses.end()) ? it->second->getVolume() : 0.0f;
    }

    AudioMixer::MixerSnapshot AudioMixer::takeSnapshot() const {
        MixerSnapshot snapshot;
        snapshot.BusVolumes["Master"] = m_MasterBus.getVolume();
        snapshot.BusMuted["Master"] = m_MasterBus.isMuted();

        for (const auto& [name, bus] : m_Busses) {
            snapshot.BusVolumes[name] = bus->getVolume();
            snapshot.BusMuted[name] = bus->isMuted();
        }
        return snapshot;
    }

    void AudioMixer::applySnapshot(const MixerSnapshot& snapshot, float transitionTime) {
        for (const auto& [name, volume] : snapshot.BusVolumes) {
            AudioBus* bus = getBus(name);
            if (bus) {
                if (transitionTime > 0.0f) {
                    bus->fadeTo(volume, transitionTime);
                } else {
                    bus->setVolume(volume);
                }
            }
        }

        if (transitionTime <= 0.0f) {
            for (const auto& [name, muted] : snapshot.BusMuted) {
                AudioBus* bus = getBus(name);
                if (bus) bus->setMuted(muted);
            }
        }
    }

}
