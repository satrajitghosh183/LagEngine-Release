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
        
        // Direct property access for inspector (references to internal Light)
        LightType Type = LightType::Point;
        glm::vec3 Color = glm::vec3(1.0f);
        float Intensity = 1.0f;
        float Range = 10.0f;
        float InnerConeAngle = glm::radians(12.5f);
        float OuterConeAngle = glm::radians(17.5f);
        bool CastShadows = false;
        
        /**
         * @brief Sync properties to internal Light structure
         */
        void SyncToLight() {
            m_Light.Type = Type;
            m_Light.Color = Color;
            m_Light.Intensity = Intensity;
            m_Light.Range = Range;
            m_Light.InnerConeAngle = glm::degrees(InnerConeAngle);
            m_Light.OuterConeAngle = glm::degrees(OuterConeAngle);
            m_Light.CastShadows = CastShadows;
        }
        
        /**
         * @brief Sync from internal Light structure to properties
         */
        void SyncFromLight() {
            Type = m_Light.Type;
            Color = m_Light.Color;
            Intensity = m_Light.Intensity;
            Range = m_Light.Range;
            InnerConeAngle = glm::radians(m_Light.InnerConeAngle);
            OuterConeAngle = glm::radians(m_Light.OuterConeAngle);
            CastShadows = m_Light.CastShadows;
        }
        
    private:
        Light m_Light;
    };
}