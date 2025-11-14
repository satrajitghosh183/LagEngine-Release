#pragma once

#include "Component.hpp"
#include "../../Audio/AudioSource.hpp"
#include "../../Audio/AudioClip.hpp"

namespace GameEngine {

    /**
     * @brief Audio source component
     * 
     * Attaches audio playback to entity
     */
    class AudioSourceComponent : public Component {
    public:
        AudioSourceComponent();
        
        COMPONENT_TYPE(AudioSourceComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;
        void OnDestroy() override;
        
        /**
         * @brief Get audio source
         */
        Ref<AudioSource> GetSource() const { return m_Source; }
        
        /**
         * @brief Set audio clip path
         */
        void SetClip(const std::string& clipPath);
        
        /**
         * @brief Playback control
         */
        void Play() { if (m_Source) m_Source->Play(); }
        void Pause() { if (m_Source) m_Source->Pause(); }
        void Stop() { if (m_Source) m_Source->Stop(); }
        
        /**
         * @brief Properties
         */
        float Volume = 1.0f;
        float Pitch = 1.0f;
        bool Loop = false;
        bool PlayOnStart = false;
        bool Spatial = true;
        
    private:
        Ref<AudioSource> m_Source;
        std::string m_ClipPath;
    };
}