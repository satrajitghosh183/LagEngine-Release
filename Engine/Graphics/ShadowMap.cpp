#include "ShadowMap.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    Ref<Shader> ShadowMap::s_DepthShader = nullptr;

    ShadowMap::ShadowMap(uint32_t resolution)
        : m_Resolution(resolution) {
        
        // Create framebuffer
        glGenFramebuffers(1, &m_FBO);
        
        // Create depth texture
        glGenTextures(1, &m_DepthTexture);
        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 
                     m_Resolution, m_Resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        
        // Texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        
        // Border color for areas outside shadow map
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        
        // Attach to framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);
        
        // No color buffer
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        
        // Check completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            GE_CORE_ERROR("ShadowMap: Framebuffer incomplete!");
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        GE_CORE_DEBUG("ShadowMap created: {}x{}", m_Resolution, m_Resolution);
    }

    ShadowMap::~ShadowMap() {
        if (m_DepthTexture) {
            glDeleteTextures(1, &m_DepthTexture);
        }
        if (m_FBO) {
            glDeleteFramebuffers(1, &m_FBO);
        }
    }

    void ShadowMap::Begin() {
        glGetIntegerv(GL_VIEWPORT, m_PreviousViewport);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
        glViewport(0, 0, m_Resolution, m_Resolution);
        glClear(GL_DEPTH_BUFFER_BIT);
        
        // Enable depth testing and writing
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        
        // Avoid shadow acne with front-face culling
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
    }

    void ShadowMap::End() {
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(m_PreviousViewport[0], m_PreviousViewport[1],
                   m_PreviousViewport[2], m_PreviousViewport[3]);
    }

    void ShadowMap::BindTexture(uint32_t slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
    }

    glm::mat4 ShadowMap::CalculateDirectionalLightMatrix(
        const glm::vec3& lightDirection,
        const glm::vec3& sceneCenter,
        float sceneRadius
    ) const {
        // Create orthographic projection for directional light
        glm::vec3 lightPos = sceneCenter - glm::normalize(lightDirection) * sceneRadius;

        glm::mat4 lightView = glm::lookAt(
            lightPos,
            sceneCenter,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Orthographic projection to cover scene
        float orthoSize = sceneRadius * 1.5f;
        glm::mat4 lightProjection = glm::ortho(
            -orthoSize, orthoSize,
            -orthoSize, orthoSize,
            0.1f, sceneRadius * 3.0f
        );

        return lightProjection * lightView;
    }

    glm::mat4 ShadowMap::CalculateSpotLightMatrix(
        const glm::vec3& lightPosition,
        const glm::vec3& lightDirection,
        float outerConeAngle,
        float nearPlane,
        float farPlane
    ) const {
        // Create perspective projection for spot light
        glm::mat4 lightView = glm::lookAt(
            lightPosition,
            lightPosition + glm::normalize(lightDirection),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Perspective projection matching spot light cone
        float fov = outerConeAngle * 2.0f;  // Full cone angle
        glm::mat4 lightProjection = glm::perspective(
            glm::radians(fov),
            1.0f,  // Square aspect ratio
            nearPlane,
            farPlane
        );

        return lightProjection * lightView;
    }

    Ref<Shader> ShadowMap::GetDepthShader() {
        if (!s_DepthShader) {
            const char* vertexSrc = R"(
                #version 420 core
                layout(location = 0) in vec3 a_Position;
                
                uniform mat4 u_LightSpaceMatrix;
                uniform mat4 u_Model;
                
                void main() {
                    gl_Position = u_LightSpaceMatrix * u_Model * vec4(a_Position, 1.0);
                }
            )";
            
            const char* fragmentSrc = R"(
                #version 420 core
                void main() {
                    // Depth is written automatically
                }
            )";
            
            s_DepthShader = CreateRef<Shader>("ShadowDepth", vertexSrc, fragmentSrc);
        }
        
        return s_DepthShader;
    }

    // ShadowMapManager implementation

    ShadowMapManager::ShadowMapManager(int maxShadowCastingLights)
        : m_MaxLights(maxShadowCastingLights) {
        
        m_ShadowMaps.reserve(maxShadowCastingLights);
        m_LightSpaceMatrices.resize(maxShadowCastingLights, glm::mat4(1.0f));
        
        // Create shadow maps
        for (int i = 0; i < maxShadowCastingLights; i++) {
            m_ShadowMaps.push_back(CreateScope<ShadowMap>(1024));
        }
        
        GE_CORE_INFO("ShadowMapManager created with {} shadow maps", maxShadowCastingLights);
    }

    void ShadowMapManager::BeginShadowPass() {
        // Save current viewport
        glGetIntegerv(GL_VIEWPORT, m_CurrentViewport);
    }

    void ShadowMapManager::EndShadowPass() {
        // Restore viewport
        glViewport(m_CurrentViewport[0], m_CurrentViewport[1], 
                   m_CurrentViewport[2], m_CurrentViewport[3]);
    }

    void ShadowMapManager::RenderDirectionalShadow(
        int lightIndex,
        const glm::vec3& direction,
        const glm::vec3& sceneCenter,
        float sceneRadius,
        const std::function<void(const glm::mat4& lightSpaceMatrix)>& renderCallback
    ) {
        if (lightIndex < 0 || lightIndex >= m_MaxLights) return;
        
        auto& shadowMap = m_ShadowMaps[lightIndex];
        
        // Calculate light space matrix for directional light
        glm::mat4 lightSpaceMatrix = shadowMap->CalculateDirectionalLightMatrix(
            direction, sceneCenter, sceneRadius
        );
        
        shadowMap->SetLightSpaceMatrix(lightSpaceMatrix);
        m_LightSpaceMatrices[lightIndex] = lightSpaceMatrix;
        
        // Render to shadow map
        shadowMap->Begin();
        
        auto depthShader = ShadowMap::GetDepthShader();
        depthShader->Bind();
        depthShader->SetUniformMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        
        // Call render callback to draw scene
        renderCallback(lightSpaceMatrix);
        
        depthShader->Unbind();
        shadowMap->End();
    }

    void ShadowMapManager::RenderSpotShadow(
        int lightIndex,
        const glm::vec3& position,
        const glm::vec3& direction,
        float outerConeAngle,
        float range,
        const std::function<void(const glm::mat4& lightSpaceMatrix)>& renderCallback
    ) {
        if (lightIndex < 0 || lightIndex >= m_MaxLights) return;
        
        auto& shadowMap = m_ShadowMaps[lightIndex];
        
        // Calculate light space matrix for spot light
        glm::mat4 lightSpaceMatrix = shadowMap->CalculateSpotLightMatrix(
            position, direction, outerConeAngle, 0.1f, range
        );
        
        shadowMap->SetLightSpaceMatrix(lightSpaceMatrix);
        m_LightSpaceMatrices[lightIndex] = lightSpaceMatrix;
        
        // Render to shadow map
        shadowMap->Begin();
        
        auto depthShader = ShadowMap::GetDepthShader();
        depthShader->Bind();
        depthShader->SetUniformMat4("u_LightSpaceMatrix", lightSpaceMatrix);
        
        // Call render callback to draw scene
        renderCallback(lightSpaceMatrix);
        
        depthShader->Unbind();
        shadowMap->End();
    }

    void ShadowMapManager::BindShadowMaps(uint32_t startSlot) const {
        for (size_t i = 0; i < m_ShadowMaps.size(); i++) {
            m_ShadowMaps[i]->BindTexture(startSlot + static_cast<uint32_t>(i));
        }
    }

    ShadowMap* ShadowMapManager::GetShadowMap(int index) {
        if (index < 0 || index >= static_cast<int>(m_ShadowMaps.size())) {
            return nullptr;
        }
        return m_ShadowMaps[index].get();
    }

    void ShadowMapManager::SetResolution(uint32_t resolution) {
        for (auto& shadowMap : m_ShadowMaps) {
            shadowMap = CreateScope<ShadowMap>(resolution);
        }
    }

}
