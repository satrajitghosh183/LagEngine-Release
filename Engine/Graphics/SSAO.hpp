#pragma once

#include "../Core/Base.hpp"
#include "FrameBuffer.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include "UniformBuffer.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace GameEngine {

    /**
     * @brief Screen-Space Ambient Occlusion (Vulkan)
     *
     * Implements SSAO using a render-pass approach:
     *   1. SSAO pass   — samples a hemisphere kernel in view-space,
     *                    writing a single-channel R8_UNORM AO value.
     *   2. Blur pass   — 4x4 box-blur on the AO buffer to remove noise.
     *
     * The two output framebuffers expose their color attachments as
     * VkImageView / VkDescriptorImageInfo so subsequent passes can sample
     * the final blurred AO image via a combined image–sampler descriptor.
     *
     * Usage:
     *   ssao.Init(width, height);
     *   // inside command buffer:
     *   ssao.Compute(cmd, posView, normalView, sampler, projectionUBO, frameIndex);
     *   ssao.Blur(cmd);
     *   // bind ssao.GetBlurredAODescriptorInfo() to your lighting pass
     */
    class SSAO {
    public:
        SSAO() = default;
        ~SSAO();

        void Init(uint32_t width, uint32_t height);
        void Resize(uint32_t width, uint32_t height);
        void Shutdown();

        /**
         * @brief Record SSAO compute pass into cmd.
         * @param posView      VkImageView of the G-Buffer world/view position attachment.
         * @param normalView   VkImageView of the G-Buffer normals attachment.
         * @param sampler      Sampler to use for G-Buffer reads (nearest, clamp-to-edge).
         * @param projBufInfo  Descriptor buffer info for the camera projection UBO.
         */
        void Compute(VkCommandBuffer cmd,
                     VkImageView posView,
                     VkImageView normalView,
                     VkSampler sampler,
                     VkDescriptorBufferInfo projBufInfo);

        /**
         * @brief Record the blur pass into cmd. Call after Compute().
         */
        void Blur(VkCommandBuffer cmd);

        /**
         * @brief Descriptor info for the blurred AO texture (binding to lighting pass).
         */
        VkDescriptorImageInfo GetBlurredAODescriptorInfo() const;

        /**
         * @brief Raw Vulkan handles for the blurred AO image.
         */
        VkImageView GetBlurredAOView()  const { return m_BlurAOView; }
        VkImage     GetBlurredAOImage() const { return m_BlurAOImage; }

        // Tuning parameters — change any time before Compute().
        float Radius    = 0.5f;
        float Bias      = 0.025f;
        int   KernelSize = 32;
        bool  Enabled   = true;

    private:
        // ---- helpers ----
        void CreateImages(uint32_t width, uint32_t height);
        void CreateRenderPasses();
        void CreateFramebuffers(uint32_t width, uint32_t height);
        void CreateSampler();
        void CreateNoiseTexture();
        void CreateDescriptorLayouts();
        void CreatePipelines();
        void GenerateKernel();
        void DestroyImages();

        // Creates a VkImage + VmaAllocation + VkImageView for a single-channel float attachment
        void CreateColorImage(uint32_t width, uint32_t height, VkFormat fmt,
                              VkImage& outImage, VmaAllocation& outAlloc,
                              VkImageView& outView);
        void TransitionLayout(VkCommandBuffer cmd, VkImage image,
                              VkImageLayout oldL, VkImageLayout newL);

        // ---- SSAO pass resources ----
        VkImage       m_SSAOImage     = VK_NULL_HANDLE;
        VmaAllocation m_SSAOAlloc     = VK_NULL_HANDLE;
        VkImageView   m_SSAOView      = VK_NULL_HANDLE;
        VkRenderPass  m_SSAORenderPass = VK_NULL_HANDLE;
        VkFramebuffer m_SSAOFB        = VK_NULL_HANDLE;

        // ---- Blur pass resources ----
        VkImage       m_BlurAOImage     = VK_NULL_HANDLE;
        VmaAllocation m_BlurAOAlloc     = VK_NULL_HANDLE;
        VkImageView   m_BlurAOView      = VK_NULL_HANDLE;
        VkRenderPass  m_BlurRenderPass  = VK_NULL_HANDLE;
        VkFramebuffer m_BlurFB          = VK_NULL_HANDLE;

        // ---- Noise texture ----
        VkImage       m_NoiseImage = VK_NULL_HANDLE;
        VmaAllocation m_NoiseAlloc = VK_NULL_HANDLE;
        VkImageView   m_NoiseView  = VK_NULL_HANDLE;

        // ---- Samplers ----
        VkSampler m_NoiseSampler  = VK_NULL_HANDLE;   // repeat, nearest
        VkSampler m_ResultSampler = VK_NULL_HANDLE;   // clamp, nearest

        // ---- Kernel UBO (array of vec4, w unused) ----
        VkBuffer      m_KernelBuffer = VK_NULL_HANDLE;
        VmaAllocation m_KernelAlloc  = VK_NULL_HANDLE;
        std::vector<glm::vec4> m_Kernel;   // up to 64 samples

        // ---- SSAO push-constants struct ----
        struct SSAOPushConstants {
            float Radius;
            float Bias;
            int   KernelSize;
            float NoiseScaleX;
            float NoiseScaleY;
        };

        // ---- Descriptor layouts / pools / sets ----
        std::unique_ptr<VulkanDescriptorSetLayout> m_SSAOLayout;
        std::unique_ptr<VulkanDescriptorSetLayout> m_BlurLayout;
        std::unique_ptr<VulkanDescriptorPool>      m_DescPool;
        VkDescriptorSet m_SSAOSet = VK_NULL_HANDLE;
        VkDescriptorSet m_BlurSet = VK_NULL_HANDLE;

        // ---- Pipelines ----
        VkPipelineLayout m_SSAOPipelineLayout = VK_NULL_HANDLE;
        VkPipelineLayout m_BlurPipelineLayout = VK_NULL_HANDLE;
        VulkanPipeline   m_SSAOPipeline;
        VulkanPipeline   m_BlurPipeline;

        uint32_t m_Width  = 0;
        uint32_t m_Height = 0;
    };

} // namespace GameEngine
