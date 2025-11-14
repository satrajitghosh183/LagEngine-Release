#pragma once

#include "Component.hpp"
#include "../../Graphics/Light.hpp"

namespace GameEngine {

    /**
     * @brief Light component
     * 
     * Adds lighting to entities
     */
    class LightComponent : public Component {
    public:
        LightComponent();
        LightComponent(LightType type);
        
        COMPONENT_TYPE(LightComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnDestroy() override;
        
        /**
         * @brief Get light data
         */
        Light& GetLight() { return m_Light; }
        const Light& GetLight() const { return m_Light; }
        
        /**
         * @brief Helper setters
         */
        void SetType(LightType type) { m_Light.Type = type; }
        void SetColor(const glm::vec3& color) { m_Light.Color = color; }
        void SetIntensity(float intensity) { m_Light.Intensity = intensity; }
        void SetRange(float range) { m_Light.Range = range; }
        
    private:
        Light m_Light;
    };
}