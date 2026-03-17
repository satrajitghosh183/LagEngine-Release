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

        // Serialize from public properties (source of truth for inspector/test usage)
        j["lightType"] = static_cast<int>(Type);
        j["color"] = { Color.r, Color.g, Color.b };
        j["intensity"] = Intensity;
        j["range"] = Range;
        j["constant"] = m_Light.Constant;
        j["linear"] = m_Light.Linear;
        j["quadratic"] = m_Light.Quadratic;
        j["innerConeAngle"] = InnerConeAngle;
        j["outerConeAngle"] = OuterConeAngle;
        j["castShadows"] = CastShadows;
        j["shadowMapResolution"] = m_Light.ShadowMapResolution;
        j["shadowBias"] = m_Light.ShadowBias;

        return j;
    }

    void LightComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];

        if (data.contains("lightType"))
            Type = static_cast<LightType>(data["lightType"].get<int>());

        if (data.contains("color")) {
            auto c = data["color"];
            Color = glm::vec3(c[0], c[1], c[2]);
        }

        if (data.contains("intensity"))
            Intensity = data["intensity"];

        if (data.contains("range"))
            Range = data["range"];

        if (data.contains("constant"))
            m_Light.Constant = data["constant"];

        if (data.contains("linear"))
            m_Light.Linear = data["linear"];

        if (data.contains("quadratic"))
            m_Light.Quadratic = data["quadratic"];

        if (data.contains("innerConeAngle"))
            InnerConeAngle = data["innerConeAngle"];

        if (data.contains("outerConeAngle"))
            OuterConeAngle = data["outerConeAngle"];

        if (data.contains("castShadows"))
            CastShadows = data["castShadows"];

        if (data.contains("shadowMapResolution"))
            m_Light.ShadowMapResolution = data["shadowMapResolution"];

        if (data.contains("shadowBias"))
            m_Light.ShadowBias = data["shadowBias"];

        // Sync public properties into the internal Light struct
        SyncToLight();
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