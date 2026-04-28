#pragma once

#include "../Core/Base.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <memory>

namespace GameEngine {

    /**
     * @brief Shadow map for a single light source (Vulkan)
     *
     * Owns a depth-only VkRenderPass + VkFramebuffer backed by a
     * VK_FORMAT_D32_SFLOAT VkImage. The depth image is left in
     * VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL after End() so it
     * can immediately be sampled in the lighting pass.
     *
     * Usage:
     *   shadowMap.Begin(cmd);
     *   // record depth-only draw calls ...
     *   shadowMap.End(cmd);
     *   // bind shadowMap.GetDepthDescriptorInfo() to lighting descriptor set
     */
    class ShadowMap {
    public:
        explicit ShadowMap(uint32_t resolution = 1024);
        ~ShadowMap();

        ShadowMap(const ShadowMap&)            = delete;
        ShadowMap& operator=(const ShadowMap&) = delete;

        /**
         * @brief Begin depth-only render pass.
         *        Transitions depth image to DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
         *        sets the viewport/scissor to m_Resolution x m_Resolution,
         *        and clears the depth buffer.
         */
        void Begin(VkCommandBuffer cmd);

        /**
         * @brief End render pass and transition depth image to READ_ONLY layout
         *        so it can be sampled in the lighting pass.
         */
        void End(VkCommandBuffer cmd);

        /**
         * @brief Descriptor image info for binding depth texture to lighting pass.
         *        Layout is VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL.
         */
        VkDescriptorImageInfo GetDepthDescriptorInfo() const;

        VkImageView GetDepthView()  const { return m_DepthView;  }
        VkImage     GetDepthImage() const { return m_DepthImage; }
        uint32_t    GetResolution() const { return m_Resolution; }

        const glm::mat4& GetLightSpaceMatrix() const { return m_LightSpaceMatrix; }
        void SetLightSpaceMatrix(const glm::mat4& m) { m_LightSpaceMatrix = m; }

        /** @brief Compute an orthographic light-space matrix for a directional light. */
        glm::mat4 CalculateDirectionalLightMatrix(const glm::vec3& lightDirection,
                                                   const glm::vec3& sceneCenter,
                                                   float sceneRadius) const;

        /** @brief Compute a perspective light-space matrix for a spot light. */
        glm::mat4 CalculateSpotLightMatrix(const glm::vec3& lightPosition,
                                            const glm::vec3& lightDirection,
                                            float outerConeAngle,
                                            float nearPlane = 0.1f,
                                            float farPlane  = 100.0f) const;

        VkRenderPass GetRenderPass() const { return m_RenderPass; }

    private:
        void CreateDepthResources();
        void CreateRenderPass();
        void CreateFramebuffer();
        void CreateSampler();

        uint32_t m_Resolution;

        VkImage       m_DepthImage = VK_NULL_HANDLE;
        VmaAllocation m_DepthAlloc = VK_NULL_HANDLE;
        VkImageView   m_DepthView  = VK_NULL_HANDLE;
        VkSampler     m_Sampler    = VK_NULL_HANDLE;

        VkRenderPass  m_RenderPass  = VK_NULL_HANDLE;
        VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

        glm::mat4 m_LightSpaceMatrix = glm::mat4(1.0f);
    };

    // -------------------------------------------------------------------------

    /**
     * @brief Manages all shadow maps for a scene (Vulkan)
     *
     * Holds one ShadowMap per shadow-casting light and provides
     * helpers to build the light-space matrices and bind the depth images
     * to descriptor sets for the lighting pass.
     */
    class ShadowMapManager {
    public:
        explicit ShadowMapManager(int maxShadowCastingLights = 4);
        ~ShadowMapManager() = default;

        /**
         * @brief Render shadow map for a directional light.
         * @param renderCallback Called with the light-space matrix; caller
         *        should record depth-only draw calls inside the callback.
         */
        void RenderDirectionalShadow(
            int lightIndex,
            const glm::vec3& direction,
            const glm::vec3& sceneCenter,
            float sceneRadius,
            VkCommandBuffer cmd,
            const std::function<void(VkCommandBuffer, const glm::mat4&)>& renderCallback);

        /**
         * @brief Render shadow map for a spot light.
         */
        void RenderSpotShadow(
            int lightIndex,
            const glm::vec3& position,
            const glm::vec3& direction,
            float outerConeAngle,
            float range,
            VkCommandBuffer cmd,
            const std::function<void(VkCommandBuffer, const glm::mat4&)>& renderCallback);

        /**
         * @brief Returns VkDescriptorImageInfo array for all shadow maps.
         *        Use this to fill an array descriptor in the lighting pass.
         */
        std::vector<VkDescriptorImageInfo> GetShadowMapDescriptors() const;

        const std::vector<glm::mat4>& GetLightSpaceMatrices() const { return m_LightSpaceMatrices; }

        ShadowMap* GetShadowMap(int index);
        void SetResolution(uint32_t resolution);

    private:
        std::vector<Scope<ShadowMap>> m_ShadowMaps;
        std::vector<glm::mat4>        m_LightSpaceMatrices;
        int                           m_MaxLights;
    };

} // namespace GameEngine
