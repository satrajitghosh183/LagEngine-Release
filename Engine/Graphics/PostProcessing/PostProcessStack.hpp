#pragma once

#include "../../Core/Base.hpp"
#include "../Vulkan/VulkanDevice.hpp"
#include "../FrameBuffer.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Abstract base for a single post-processing effect.
     *
     * Vulkan pipeline note
     * --------------------
     * Each concrete effect owns its own VkPipeline (fullscreen-triangle) and
     * descriptor set.  The apply() call records into an already-active render
     * pass targeting outputFramebuffer.  The input is supplied as a
     * VkDescriptorImageInfo so the effect can bind it to set 0 / binding 0.
     */
    class PostProcessEffect {
    public:
        virtual ~PostProcessEffect() = default;
        virtual std::string getName() const = 0;

        /**
         * @brief Allocate Vulkan pipelines and descriptor sets.
         * @param width  Initial framebuffer width
         * @param height Initial framebuffer height
         * @param renderPass RenderPass the effect will draw into (for pipeline creation)
         */
        virtual void init(uint32_t width, uint32_t height, VkRenderPass renderPass) = 0;

        /**
         * @brief Recreate size-dependent resources.
         */
        virtual void resize(uint32_t width, uint32_t height) = 0;

        /**
         * @brief Record draw commands.
         * @param cmd             Active command buffer (render pass already begun)
         * @param inputImageInfo  Descriptor info for the input texture (SHADER_READ_ONLY layout)
         * @param width           Viewport width
         * @param height          Viewport height
         */
        virtual void apply(VkCommandBuffer cmd,
                           const VkDescriptorImageInfo& inputImageInfo,
                           uint32_t width, uint32_t height) = 0;

        virtual void renderUI() {} // ImGui settings

        bool Enabled  = true;
        int  Priority = 0; // Lower = earlier in chain
    };

    /**
     * @brief Ordered chain of PostProcessEffect objects using Vulkan ping-pong framebuffers.
     *
     * Usage:
     *   stack.init(width, height, swapchainFormat);
     *   stack.addEffect<BloomEffect>();
     *   stack.addEffect<ToneMappingEffect>();
     *
     *   // Per frame:
     *   VkImageView finalView = stack.apply(cmd, sceneImageInfo, width, height);
     *   // Blit/present finalView
     */
    class PostProcessStack {
    public:
        PostProcessStack() = default;
        ~PostProcessStack();

        PostProcessStack(const PostProcessStack&) = delete;
        PostProcessStack& operator=(const PostProcessStack&) = delete;

        /**
         * @brief Allocate ping-pong framebuffers and initialise all effects.
         * @param format Vulkan format for the ping-pong color attachments (e.g. VK_FORMAT_R16G16B16A16_SFLOAT)
         */
        void init(uint32_t width, uint32_t height,
                  VkFormat format = VK_FORMAT_R16G16B16A16_SFLOAT);

        void resize(uint32_t width, uint32_t height);

        /** Add an effect to the stack and initialise it. */
        template<typename T, typename... Args>
        T* addEffect(Args&&... args) {
            auto effect = CreateScope<T>(std::forward<Args>(args)...);
            if (m_Initialized) {
                effect->init(m_Width, m_Height, m_RenderPass);
            }
            T* ptr = effect.get();
            m_Effects.push_back(std::move(effect));
            sortEffects();
            return ptr;
        }

        void removeEffect(PostProcessEffect* effect);

        /**
         * @brief Apply all enabled effects.
         *
         * Records vkCmdBeginRenderPass / apply() / vkCmdEndRenderPass for each
         * effect into cmd, ping-ponging between two internal framebuffers.
         *
         * @param cmd             Active primary command buffer (outside any render pass)
         * @param sceneImageInfo  Descriptor info for the HDR scene texture
         * @return ImageView of the final processed image (ready for presentation blit)
         */
        VkImageView apply(VkCommandBuffer cmd,
                          const VkDescriptorImageInfo& sceneImageInfo,
                          uint32_t width, uint32_t height);

        const std::vector<Scope<PostProcessEffect>>& getEffects() const { return m_Effects; }

        VkRenderPass getRenderPass() const { return m_RenderPass; }

    private:
        void createPingPongResources();
        void destroyPingPongResources();
        void sortEffects();

        uint32_t m_Width  = 0;
        uint32_t m_Height = 0;
        VkFormat m_Format = VK_FORMAT_R16G16B16A16_SFLOAT;
        bool     m_Initialized = false;

        // Shared render pass used by all fullscreen effects
        VkRenderPass m_RenderPass = VK_NULL_HANDLE;

        // Ping-pong color attachments
        struct PingPong {
            VkImage       Image      = VK_NULL_HANDLE;
            VmaAllocation Allocation = VK_NULL_HANDLE;
            VkImageView   View       = VK_NULL_HANDLE;
            VkFramebuffer Framebuffer = VK_NULL_HANDLE;
            VkSampler     Sampler    = VK_NULL_HANDLE;
        };
        PingPong m_PingPong[2];

        std::vector<Scope<PostProcessEffect>> m_Effects;
    };

    // =========================================================================
    // Built-in effects
    // =========================================================================

    /**
     * @brief Luminance threshold + Gaussian blur + additive combine (Bloom).
     *
     * Implemented as three fullscreen compute/graphics passes:
     *   Pass 1 — bright-pass filter (writes to m_BrightFB)
     *   Pass 2 — separable Gaussian blur (ping-pong between m_BlurFB[0] and [1])
     *   Pass 3 — additive combine (writes to outputFBO)
     */
    class BloomEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Bloom"; }
        void init(uint32_t width, uint32_t height, VkRenderPass renderPass) override;
        void resize(uint32_t width, uint32_t height) override;
        void apply(VkCommandBuffer cmd,
                   const VkDescriptorImageInfo& inputImageInfo,
                   uint32_t width, uint32_t height) override;
        void renderUI() override;

        float Threshold  = 1.0f;
        float Intensity  = 1.0f;
        int   BlurPasses = 5;
    };

    /** @brief ACES / Reinhard / Filmic tone mapping + gamma correction. */
    class ToneMappingEffect : public PostProcessEffect {
    public:
        enum class Algorithm { Reinhard, ACES, Filmic, Uncharted2 };

        std::string getName() const override { return "Tone Mapping"; }
        void init(uint32_t width, uint32_t height, VkRenderPass renderPass) override;
        void resize(uint32_t width, uint32_t height) override;
        void apply(VkCommandBuffer cmd,
                   const VkDescriptorImageInfo& inputImageInfo,
                   uint32_t width, uint32_t height) override;
        void renderUI() override;

        Algorithm CurrentAlgorithm = Algorithm::ACES;
        float     Exposure         = 1.0f;
        float     Gamma            = 2.2f;
    };

    /** @brief Fast approximate anti-aliasing (FXAA). */
    class FXAAEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "FXAA"; }
        void init(uint32_t width, uint32_t height, VkRenderPass renderPass) override;
        void resize(uint32_t width, uint32_t height) override;
        void apply(VkCommandBuffer cmd,
                   const VkDescriptorImageInfo& inputImageInfo,
                   uint32_t width, uint32_t height) override;

        float SubpixelQuality    = 0.75f;
        float EdgeThreshold      = 0.166f;
        float EdgeThresholdMin   = 0.0833f;
    };

    /** @brief Screen-space vignette darkening. */
    class VignetteEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Vignette"; }
        void init(uint32_t width, uint32_t height, VkRenderPass renderPass) override;
        void resize(uint32_t width, uint32_t height) override;
        void apply(VkCommandBuffer cmd,
                   const VkDescriptorImageInfo& inputImageInfo,
                   uint32_t width, uint32_t height) override;

        float     Intensity   = 0.5f;
        float     Smoothness  = 2.0f;
        glm::vec3 Color       = glm::vec3(0.0f);
    };

    /** @brief Saturation, contrast, brightness, color filter, and temperature. */
    class ColorGradingEffect : public PostProcessEffect {
    public:
        std::string getName() const override { return "Color Grading"; }
        void init(uint32_t width, uint32_t height, VkRenderPass renderPass) override;
        void resize(uint32_t width, uint32_t height) override;
        void apply(VkCommandBuffer cmd,
                   const VkDescriptorImageInfo& inputImageInfo,
                   uint32_t width, uint32_t height) override;
        void renderUI() override;

        float     Saturation  = 1.0f;
        float     Contrast    = 1.0f;
        float     Brightness  = 0.0f;
        glm::vec3 ColorFilter = glm::vec3(1.0f);
        float     Temperature = 0.0f; // -1 = cool, +1 = warm
    };

} // namespace GameEngine
