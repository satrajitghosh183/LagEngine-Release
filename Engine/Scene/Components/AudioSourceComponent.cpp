#include "AudioSourceComponent.hpp"
#include "../Scene.hpp"
#include "TransformComponent.hpp"

namespace GameEngine {

    AudioSourceComponent::AudioSourceComponent() {
        m_Source = CreateRef<AudioSource>();
    }

    nlohmann::json AudioSourceComponent::Serialize() const {
        nlohmann::json data;
        data["clipPath"] = m_ClipPath;
        data["volume"] = Volume;
        data["pitch"] = Pitch;
        data["loop"] = Loop;
        data["playOnStart"] = PlayOnStart;
        data["spatial"] = Spatial;
        return data;
    }

    void AudioSourceComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("clipPath")) {
            m_ClipPath = data["clipPath"].get<std::string>();
        }
        if (data.contains("volume")) {
            Volume = data["volume"].get<float>();
        }
        if (data.contains("pitch")) {
            Pitch = data["pitch"].get<float>();
        }
        if (data.contains("loop")) {
            Loop = data["loop"].get<bool>();
        }
        if (data.contains("playOnStart")) {
            PlayOnStart = data["playOnStart"].get<bool>();
        }
        if (data.contains("spatial")) {
            Spatial = data["spatial"].get<bool>();
        }
    }

    void AudioSourceComponent::OnCreate() {
        if (!m_ClipPath.empty()) {
            SetClip(m_ClipPath);
        }
        
        if (m_Source) {
            m_Source->SetVolume(Volume);
            m_Source->SetPitch(Pitch);
            m_Source->SetLoop(Loop);
        }
        
        if (PlayOnStart && m_Source) {
            m_Source->Play();
        }
    }

    void AudioSourceComponent::OnUpdate(float deltaTime) {
        if (!m_Source || !Spatial) return;
        
        // Update position from transform
        Entity owner = GetOwnerEntity();
        if (owner && owner.HasComponent<TransformComponent>()) {
            auto& transform = owner.GetComponent<TransformComponent>();
            m_Source->SetPosition(transform.Position);
        }
    }

    void AudioSourceComponent::OnDestroy() {
        if (m_Source) {
            m_Source->Stop();
        }
    }

    void AudioSourceComponent::SetClip(const std::string& clipPath) {
        m_ClipPath = clipPath;
        
        // Load audio clip - for now just store the path
        // Full implementation would load via AssetDatabase
        // TODO: Integrate with AssetDatabase when available
    }

}
