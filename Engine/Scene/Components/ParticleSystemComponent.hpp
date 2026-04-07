#pragma once

#include "Component.hpp"
#include "../../ParticleSystem/ParticleEmitter.hpp"
#include "../../Graphics/Texture2D.hpp"

namespace GameEngine {

    /**
     * @brief Particle system component
     */
    class ParticleSystemComponent : public Component {
    public:
        ParticleSystemComponent();
        
        COMPONENT_TYPE(ParticleSystemComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;
        
        /**
         * @brief Get emitter
         */
        Ref<ParticleEmitter> GetEmitter() const { return m_Emitter; }
        
        /**
         * @brief Set particle texture
         */
        void SetTexture(const Ref<Texture2D>& texture) { m_Texture = texture; }
        Ref<Texture2D> GetTexture() const { return m_Texture; }
        
    private:
        Ref<ParticleEmitter> m_Emitter;
        Ref<Texture2D> m_Texture;
    };
}