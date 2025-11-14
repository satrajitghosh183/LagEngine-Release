#include "LightComponent.hpp"

namespace GameEngine {

    LightComponent::LightComponent() {
    }

    LightComponent::LightComponent(LightType type) {
        m_Light.Type = type;
    }

    nlohmann::json LightComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        
        j["lightType"] = static_cast<int>(m_Light.Type);
        j["color"] = { m_Light.Color.r, m_Light.Color.g, m_Light.Color.b };
        j["intensity"] = m_Light.Intensity;
        j["range"] = m_Light.Range;
        j["constant"] = m_Light.Constant;
        j["linear"] = m_Light.Linear;
        j["quadratic"] = m_Light.Quadratic;
        j["innerConeAngle"] = m_Light.InnerConeAngle;
        j["outerConeAngle"] = m_Light.OuterConeAngle;
        j["castShadows"] = m_Light.CastShadows;
        j["shadowMapResolution"] = m_Light.ShadowMapResolution;
        j["shadowBias"] = m_Light.ShadowBias;
        
        return j;
    }

    void LightComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("lightType"))
            m_Light.Type = static_cast<LightType>(data["lightType"].get<int>());
        
        if (data.contains("color")) {
            auto c = data["color"];
            m_Light.Color = glm::vec3(c[0], c[1], c[2]);
        }
        
        if (data.contains("intensity"))
            m_Light.Intensity = data["intensity"];
        
        if (data.contains("range"))
            m_Light.Range = data["range"];
        
        if (data.contains("constant"))
            m_Light.Constant = data["constant"];
        
        if (data.contains("linear"))
            m_Light.Linear = data["linear"];
        
        if (data.contains("quadratic"))
            m_Light.Quadratic = data["quadratic"];
        
        if (data.contains("innerConeAngle"))
            m_Light.InnerConeAngle = data["innerConeAngle"];
        
        if (data.contains("outerConeAngle"))
            m_Light.OuterConeAngle = data["outerConeAngle"];
        
        if (data.contains("castShadows"))
            m_Light.CastShadows = data["castShadows"];
        
        if (data.contains("shadowMapResolution"))
            m_Light.ShadowMapResolution = data["shadowMapResolution"];
        
        if (data.contains("shadowBias"))
            m_Light.ShadowBias = data["shadowBias"];
    }

    void LightComponent::OnCreate() {
        // Register with lighting system
        // TODO: Add to scene's light list
    }

    void LightComponent::OnDestroy() {
        // Unregister from lighting system
        // TODO: Remove from scene's light list
    }
}