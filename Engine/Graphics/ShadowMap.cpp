#include "ShadowMap.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

namespace GameEngine {

    // =========================================================================
    // ShadowMap
    // =========================================================================

    ShadowMap::ShadowMap(uint32_t resolution)
        : m_Resolution(resolution) {
        CreateDepthResources();
        CreateRenderPass();
        CreateFramebuffer();
        CreateSampler();
        GE_CORE_INFO("ShadowMap created: {}x{}", resolution, resolution);
    }

    ShadowMap::~ShadowMap() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        vkDeviceWaitIdle(d);

        if (m_Framebuffer) { vkDestroyFramebuffer(d, m_Framebuffer, nullptr); }
        if (m_RenderPass)  { vkDestroyRenderPass(d, m_RenderPass, nullptr);   }
        if (m_Sampler)     { vkDestroySampler(d, m_Sampler, nullptr);         }
        if (m_DepthView)   { vkDestroyImageView(d, m_DepthView, nullptr);     }
        if (m_DepthImage)  { vmaDestroyImage(a, m_DepthImage, m_DepthAlloc);  }
    }

    // -------------------------------------------------------------------------
    // Resource creation
    // -------------------------------------------------------------------------

    void ShadowMap::CreateDepthResources() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType   = VK_IMAGE_TYPE_2D;
        ici.format      = VK_FORMAT_D32_SFLOAT;
        ici.extent      = {m_Resolution, m_Resolution, 1};
        ici.mipLevels   = 1;
        ici.arrayLayers = 1;
        ici.samples     = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling      = VK_IMAGE_TILING_OPTIMAL;
        ici.usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;
        ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        if (vmaCreateImage(a, &ici, &vaci, &m_DepthImage, &m_DepthAlloc, nullptr) != VK_SUCCESS) {
            GE_CORE_ERROR("ShadowMap: Failed to create depth image");
            return;
        }

        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image    = m_DepthImage;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format   = VK_FORMAT_D32_SFLOAT;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(d, &ivci, nullptr, &m_DepthView);
    }

    void ShadowMap::CreateRenderPass() {
        VkDevice d = VulkanDevice::Get().GetDevice();

        VkAttachmentDescription depthAtt{};
        depthAtt.format         = VK_FORMAT_D32_SFLOAT;
        depthAtt.samples        = VK_SAMPLE_COUNT_1_BIT;
        depthAtt.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAtt.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depthAtt.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAtt.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAtt.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference depthRef{};
        depthRef.attachment = 0;
        depthRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription sub{};
        sub.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount    = 0;
        sub.pDepthStencilAttachment = &depthRef;

        // Ensure writes are visible to the fragment shader in the lighting pass
        std::array<VkSubpassDependency, 2> deps{};
        deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass    = 0;
        deps[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        deps[1].srcSubpass    = 0;
        deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask  = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 1;
        rpci.pAttachments    = &depthAtt;
        rpci.subpassCount    = 1;
        rpci.pSubpasses      = &sub;
        rpci.dependencyCount = static_cast<uint32_t>(deps.size());
        rpci.pDependencies   = deps.data();

        if (vkCreateRenderPass(d, &rpci, nullptr, &m_RenderPass) != VK_SUCCESS) {
            GE_CORE_ERROR("ShadowMap: Failed to create render pass");
        }
    }

    void ShadowMap::CreateFramebuffer() {
        VkDevice d = VulkanDevice::Get().GetDevice();

        VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fci.renderPass      = m_RenderPass;
        fci.attachmentCount = 1;
        fci.pAttachments    = &m_DepthView;
        fci.width           = m_Resolution;
        fci.height          = m_Resolution;
        fci.layers          = 1;

        if (vkCreateFramebuffer(d, &fci, nullptr, &m_Framebuffer) != VK_SUCCESS) {
            GE_CORE_ERROR("ShadowMap: Failed to create framebuffer");
        }
    }

    void ShadowMap::CreateSampler() {
        VkDevice d = VulkanDevice::Get().GetDevice();

        VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sci.magFilter        = VK_FILTER_NEAREST;
        sci.minFilter        = VK_FILTER_NEAREST;
        sci.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sci.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sci.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        sci.borderColor      = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE; // outside = lit (no shadow)
        sci.compareEnable    = VK_TRUE;                            // PCF-ready
        sci.compareOp        = VK_COMPARE_OP_LESS_OR_EQUAL;
        sci.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        vkCreateSampler(d, &sci, nullptr, &m_Sampler);
    }

    // -------------------------------------------------------------------------
    // Begin / End
    // -------------------------------------------------------------------------

    void ShadowMap::Begin(VkCommandBuffer cmd) {
        VkClearValue clear{};
        clear.depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass        = m_RenderPass;
        rpbi.framebuffer       = m_Framebuffer;
        rpbi.renderArea.offset = {0, 0};
        rpbi.renderArea.extent = {m_Resolution, m_Resolution};
        rpbi.clearValueCount   = 1;
        rpbi.pClearValues      = &clear;

        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        VkViewport viewport{};
        viewport.width    = static_cast<float>(m_Resolution);
        viewport.height   = static_cast<float>(m_Resolution);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.extent = {m_Resolution, m_Resolution};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void ShadowMap::End(VkCommandBuffer cmd) {
        vkCmdEndRenderPass(cmd);
        // Depth image transitions to VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        // automatically via the renderpass finalLayout.
    }

    // -------------------------------------------------------------------------
    // Descriptor info
    // -------------------------------------------------------------------------

    VkDescriptorImageInfo ShadowMap::GetDepthDescriptorInfo() const {
        VkDescriptorImageInfo info{};
        info.sampler     = m_Sampler;
        info.imageView   = m_DepthView;
        info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        return info;
    }

    // -------------------------------------------------------------------------
    // Light-space matrix helpers
    // -------------------------------------------------------------------------

    glm::mat4 ShadowMap::CalculateDirectionalLightMatrix(const glm::vec3& lightDirection,
                                                          const glm::vec3& sceneCenter,
                                                          float sceneRadius) const {
        glm::vec3 lightPos = sceneCenter - glm::normalize(lightDirection) * sceneRadius;

        glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, glm::vec3(0.0f, 1.0f, 0.0f));

        float orthoSize = sceneRadius * 1.5f;
        // Vulkan clip-space: Y is inverted, Z in [0,1].
        // glm::ortho already handles Y; no extra correction needed for NDC depth.
        glm::mat4 lightProj = glm::ortho(-orthoSize, orthoSize,
                                          -orthoSize, orthoSize,
                                          0.1f, sceneRadius * 3.0f);
        // Flip Y for Vulkan NDC
        lightProj[1][1] *= -1.0f;

        return lightProj * lightView;
    }

    glm::mat4 ShadowMap::CalculateSpotLightMatrix(const glm::vec3& lightPosition,
                                                   const glm::vec3& lightDirection,
                                                   float outerConeAngle,
                                                   float nearPlane,
                                                   float farPlane) const {
        glm::mat4 lightView = glm::lookAt(lightPosition,
                                           lightPosition + glm::normalize(lightDirection),
                                           glm::vec3(0.0f, 1.0f, 0.0f));

        float fov = outerConeAngle * 2.0f;
        glm::mat4 lightProj = glm::perspective(glm::radians(fov), 1.0f, nearPlane, farPlane);
        lightProj[1][1] *= -1.0f; // Vulkan Y flip

        return lightProj * lightView;
    }

    // =========================================================================
    // ShadowMapManager
    // =========================================================================

    ShadowMapManager::ShadowMapManager(int maxShadowCastingLights)
        : m_MaxLights(maxShadowCastingLights) {
        m_ShadowMaps.reserve(maxShadowCastingLights);
        m_LightSpaceMatrices.resize(maxShadowCastingLights, glm::mat4(1.0f));
        for (int i = 0; i < maxShadowCastingLights; i++) {
            m_ShadowMaps.push_back(CreateScope<ShadowMap>(1024));
        }
        GE_CORE_INFO("ShadowMapManager: {} shadow maps created", maxShadowCastingLights);
    }

    void ShadowMapManager::RenderDirectionalShadow(
        int lightIndex,
        const glm::vec3& direction,
        const glm::vec3& sceneCenter,
        float sceneRadius,
        VkCommandBuffer cmd,
        const std::function<void(VkCommandBuffer, const glm::mat4&)>& renderCallback) {

        if (lightIndex < 0 || lightIndex >= m_MaxLights) return;

        auto& sm = m_ShadowMaps[lightIndex];
        glm::mat4 lsm = sm->CalculateDirectionalLightMatrix(direction, sceneCenter, sceneRadius);
        sm->SetLightSpaceMatrix(lsm);
        m_LightSpaceMatrices[lightIndex] = lsm;

        sm->Begin(cmd);
        renderCallback(cmd, lsm);
        sm->End(cmd);
    }

    void ShadowMapManager::RenderSpotShadow(
        int lightIndex,
        const glm::vec3& position,
        const glm::vec3& direction,
        float outerConeAngle,
        float range,
        VkCommandBuffer cmd,
        const std::function<void(VkCommandBuffer, const glm::mat4&)>& renderCallback) {

        if (lightIndex < 0 || lightIndex >= m_MaxLights) return;

        auto& sm = m_ShadowMaps[lightIndex];
        glm::mat4 lsm = sm->CalculateSpotLightMatrix(position, direction, outerConeAngle, 0.1f, range);
        sm->SetLightSpaceMatrix(lsm);
        m_LightSpaceMatrices[lightIndex] = lsm;

        sm->Begin(cmd);
        renderCallback(cmd, lsm);
        sm->End(cmd);
    }

    std::vector<VkDescriptorImageInfo> ShadowMapManager::GetShadowMapDescriptors() const {
        std::vector<VkDescriptorImageInfo> infos;
        infos.reserve(m_ShadowMaps.size());
        for (const auto& sm : m_ShadowMaps) {
            infos.push_back(sm->GetDepthDescriptorInfo());
        }
        return infos;
    }

    ShadowMap* ShadowMapManager::GetShadowMap(int index) {
        if (index < 0 || index >= static_cast<int>(m_ShadowMaps.size())) return nullptr;
        return m_ShadowMaps[index].get();
    }

    void ShadowMapManager::SetResolution(uint32_t resolution) {
        for (auto& sm : m_ShadowMaps) {
            sm = CreateScope<ShadowMap>(resolution);
        }
    }

} // namespace GameEngine
