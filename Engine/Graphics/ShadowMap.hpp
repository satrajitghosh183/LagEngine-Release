#pragma once

#include "../Core/Base.hpp"
#include "Shader.hpp"
#include "FrameBuffer.hpp"
#include "Light.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Shadow map for a single light source
     * 
     * Supports directional, point, and spot light shadows.
     * Uses depth-only framebuffer for shadow mapping.
     */
    class ShadowMap {
    public:
        /**
         * @brief Create shadow map
         * @param resolution Texture resolution (width and height)
         */
        ShadowMap(uint32_t resolution = 1024);
        ~ShadowMap();
        
        /**
         * @brief Begin shadow map rendering
         * Call this before rendering shadow casters
         */
        void Begin();
        
        /**
         * @brief End shadow map rendering
         */
        void End();
        
        /**
         * @brief Bind shadow map texture for sampling
         * @param slot Texture slot
         */
        void BindTexture(uint32_t slot = 0) const;
        
        /**
         * @brief Get light space matrix for directional light
         */
        glm::mat4 CalculateDirectionalLightMatrix(
            const glm::vec3& lightDirection,
            const glm::vec3& sceneCenter,
            float sceneRadius
        ) const;

        /**
         * @brief Get light space matrix for spot light
         */
        glm::mat4 CalculateSpotLightMatrix(
            const glm::vec3& lightPosition,
            const glm::vec3& lightDirection,
            float outerConeAngle,
            float nearPlane = 0.1f,
            float farPlane = 100.0f
        ) const;
        
        /**
         * @brief Get resolution
         */
        uint32_t GetResolution() const { return m_Resolution; }
        
        /**
         * @brief Get depth texture ID
         */
        uint32_t GetDepthTextureID() const { return m_DepthTexture; }
        
        /**
         * @brief Get shadow depth shader
         */
        static Ref<Shader> GetDepthShader();
        
        /**
         * @brief Get the current light space matrix (set after Begin)
         */
        const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
        void SetLightSpaceMatrix(const glm::mat4& matrix) { m_LightSpaceMatrix = matrix; }
        
    private:
        uint32_t m_Resolution;
        uint32_t m_FBO = 0;
        uint32_t m_DepthTexture = 0;
        
        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
        
        int m_PreviousViewport[4] = {0, 0, 0, 0};
        
        // Shared depth-only shader
        static Ref<Shader> s_DepthShader;
    };

    /**
     * @brief Manages all shadow maps in the scene
     */
    class ShadowMapManager {
    public:
        ShadowMapManager(int maxShadowCastingLights = 4);
        ~ShadowMapManager() = default;
        
        /**
         * @brief Prepare shadow maps for rendering
         */
        void BeginShadowPass();
        
        /**
         * @brief Finish shadow pass
         */
        void EndShadowPass();
        
        /**
         * @brief Render shadow map for a directional light
         */
        void RenderDirectionalShadow(
            int lightIndex,
            const glm::vec3& direction,
            const glm::vec3& sceneCenter,
            float sceneRadius,
            const std::function<void(const glm::mat4& lightSpaceMatrix)>& renderCallback
        );
        
        /**
         * @brief Render shadow map for a spot light
         */
        void RenderSpotShadow(
            int lightIndex,
            const glm::vec3& position,
            const glm::vec3& direction,
            float outerConeAngle,
            float range,
            const std::function<void(const glm::mat4& lightSpaceMatrix)>& renderCallback
        );
        
        /**
         * @brief Bind shadow maps for sampling
         * @param startSlot First texture slot to use
         */
        void BindShadowMaps(uint32_t startSlot = 4) const;
        
        /**
         * @brief Get light space matrices for shader
         */
        const std::vector<glm::mat4>& GetLightSpaceMatrices() const { return m_LightSpaceMatrices; }
        
        /**
         * @brief Get shadow map at index
         */
        ShadowMap* GetShadowMap(int index);
        
        /**
         * @brief Set shadow map resolution
         */
        void SetResolution(uint32_t resolution);
        
    private:
        std::vector<Scope<ShadowMap>> m_ShadowMaps;
        std::vector<glm::mat4> m_LightSpaceMatrices;
        int m_MaxLights;
        int m_CurrentViewport[4];
    };

}
