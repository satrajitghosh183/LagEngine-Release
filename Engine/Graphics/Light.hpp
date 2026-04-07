#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Light types
     */
    enum class LightType {
        Directional,  // Sun-like light (parallel rays)
        Point,        // Omnidirectional light source
        Spot          // Cone-shaped light
    };

    /**
     * @brief Light properties
     */
    struct Light {
        LightType Type = LightType::Point;
        
        glm::vec3 Color = glm::vec3(1.0f);
        float Intensity = 1.0f;
        
        // Point & Spot light attenuation
        float Range = 10.0f;
        float Constant = 1.0f;
        float Linear = 0.09f;
        float Quadratic = 0.032f;
        
        // Spot light specific
        float InnerConeAngle = 12.5f;  // Degrees
        float OuterConeAngle = 17.5f;  // Degrees
        
        // Shadow mapping
        bool CastShadows = false;
        uint32_t ShadowMapResolution = 1024;
        float ShadowBias = 0.005f;
        
        /**
         * @brief Calculate attenuation at given distance
         */
        float CalculateAttenuation(float distance) const {
            return 1.0f / (Constant + Linear * distance + Quadratic * distance * distance);
        }
    };
}