#include "PostProcessStack.hpp"
#include "../../Core/Logger.hpp"
#include <algorithm>
#include <array>
#include <cstring>

// =============================================================================
// PostProcessStack — Vulkan implementation
//
// Design notes
// ------------
// * All effects render a fullscreen triangle (3 vertices, no vertex buffer).
//   The SPIR-V vertex shader generates clip-space positions from gl_VertexIndex.
// * Push constants pass per-effect parameters (exposure, gamma, etc.).
// * A single shared VkRenderPass is used by all effects — single subpass,
//   one color attachment, no depth.
// * Two ping-pong framebuffers alternate as input/output.  The "input" is
//   bound as a combined image sampler (descriptor set 0, binding 0).
// * Actual SPIR-V is loaded from Assets/Shaders/ at runtime via
//   Shader::LoadSpirv.  If the .spv files are absent the effect is a no-op.
// =============================================================================

namespace GameEngine {

    // =========================================================================
    // Helpers
    // =========================================================================

    static VkRenderPass CreateFullscreenRenderPass(VkDevice device, VkFormat colorFormat) {
        VkAttachmentDescription colorAttach{};
        colorAttach.format         = colorFormat;
        colorAttach.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttach.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttach.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttach.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttach.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttach.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttach.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorRef;

        // Subpass dependencies to ensure image layout transitions are correct
        std::array<VkSubpassDependency, 2> deps{};
        deps[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass      = 0;
        deps[0].srcStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[0].srcAccessMask   = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        deps[1].srcSubpass      = 0;
        deps[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask   = VK_ACCESS_SHADER_READ_BIT;
        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo rpCI{};
        rpCI.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpCI.attachmentCount = 1;
        rpCI.pAttachments    = &colorAttach;
        rpCI.subpassCount    = 1;
        rpCI.pSubpasses      = &subpass;
        rpCI.dependencyCount = static_cast<uint32_t>(deps.size());
        rpCI.pDependencies   = deps.data();

        VkRenderPass rp = VK_NULL_HANDLE;
        vkCreateRenderPass(device, &rpCI, nullptr, &rp);
        return rp;
    }

    static void CreateColorAttachment(VkDevice device, VmaAllocator allocator,
                                       VkFormat format, uint32_t width, uint32_t height,
                                       VkImage& outImage, VmaAllocation& outAlloc,
                                       VkImageView& outView) {
        VkImageCreateInfo imgCI{};
        imgCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imgCI.imageType     = VK_IMAGE_TYPE_2D;
        imgCI.format        = format;
        imgCI.extent        = {width, height, 1};
        imgCI.mipLevels     = 1;
        imgCI.arrayLayers   = 1;
        imgCI.samples       = VK_SAMPLE_COUNT_1_BIT;
        imgCI.tiling        = VK_IMAGE_TILING_OPTIMAL;
        imgCI.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imgCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocCI{};
        allocCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        vmaCreateImage(allocator, &imgCI, &allocCI, &outImage, &outAlloc, nullptr);

        VkImageViewCreateInfo viewCI{};
        viewCI.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image                           = outImage;
        viewCI.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        viewCI.format                          = format;
        viewCI.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.levelCount     = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = 1;
        vkCreateImageView(device, &viewCI, nullptr, &outView);
    }

    static VkSampler CreateLinearSampler(VkDevice device) {
        VkSamplerCreateInfo si{};
        si.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.magFilter    = VK_FILTER_LINEAR;
        si.minFilter    = VK_FILTER_LINEAR;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;

        VkSampler s = VK_NULL_HANDLE;
        vkCreateSampler(device, &si, nullptr, &s);
        return s;
    }

    // =========================================================================
    // PostProcessStack
    // =========================================================================

    PostProcessStack::~PostProcessStack() {
        destroyPingPongResources();

        if (m_RenderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(VulkanDevice::Get().GetDevice(), m_RenderPass, nullptr);
            m_RenderPass = VK_NULL_HANDLE;
        }
    }

    void PostProcessStack::init(uint32_t width, uint32_t height, VkFormat format) {
        m_Width  = width;
        m_Height = height;
        m_Format = format;

        VkDevice device = VulkanDevice::Get().GetDevice();
        m_RenderPass = CreateFullscreenRenderPass(device, format);

        createPingPongResources();

        for (auto& e : m_Effects) {
            e->init(width, height, m_RenderPass);
        }

        m_Initialized = true;
    }

    void PostProcessStack::resize(uint32_t width, uint32_t height) {
        m_Width  = width;
        m_Height = height;
        destroyPingPongResources();
        createPingPongResources();
        for (auto& e : m_Effects) {
            e->resize(width, height);
        }
    }

    void PostProcessStack::createPingPongResources() {
        VkDevice      device    = VulkanDevice::Get().GetDevice();
        VmaAllocator  allocator = VulkanDevice::Get().GetAllocator();

        for (int i = 0; i < 2; i++) {
            CreateColorAttachment(device, allocator, m_Format, m_Width, m_Height,
                                   m_PingPong[i].Image, m_PingPong[i].Allocation,
                                   m_PingPong[i].View);

            m_PingPong[i].Sampler = CreateLinearSampler(device);

            VkFramebufferCreateInfo fbCI{};
            fbCI.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbCI.renderPass      = m_RenderPass;
            fbCI.attachmentCount = 1;
            fbCI.pAttachments    = &m_PingPong[i].View;
            fbCI.width           = m_Width;
            fbCI.height          = m_Height;
            fbCI.layers          = 1;
            vkCreateFramebuffer(device, &fbCI, nullptr, &m_PingPong[i].Framebuffer);
        }
    }

    void PostProcessStack::destroyPingPongResources() {
        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        for (int i = 0; i < 2; i++) {
            if (m_PingPong[i].Framebuffer) {
                vkDestroyFramebuffer(device, m_PingPong[i].Framebuffer, nullptr);
                m_PingPong[i].Framebuffer = VK_NULL_HANDLE;
            }
            if (m_PingPong[i].Sampler) {
                vkDestroySampler(device, m_PingPong[i].Sampler, nullptr);
                m_PingPong[i].Sampler = VK_NULL_HANDLE;
            }
            if (m_PingPong[i].View) {
                vkDestroyImageView(device, m_PingPong[i].View, nullptr);
                m_PingPong[i].View = VK_NULL_HANDLE;
            }
            if (m_PingPong[i].Image) {
                vmaDestroyImage(allocator, m_PingPong[i].Image, m_PingPong[i].Allocation);
                m_PingPong[i].Image      = VK_NULL_HANDLE;
                m_PingPong[i].Allocation = VK_NULL_HANDLE;
            }
        }
    }

    void PostProcessStack::sortEffects() {
        std::sort(m_Effects.begin(), m_Effects.end(),
            [](const Scope<PostProcessEffect>& a, const Scope<PostProcessEffect>& b) {
                return a->Priority < b->Priority;
            });
    }

    void PostProcessStack::removeEffect(PostProcessEffect* effect) {
        m_Effects.erase(
            std::remove_if(m_Effects.begin(), m_Effects.end(),
                [effect](const Scope<PostProcessEffect>& e) { return e.get() == effect; }),
            m_Effects.end());
    }

    VkImageView PostProcessStack::apply(VkCommandBuffer cmd,
                                         const VkDescriptorImageInfo& sceneImageInfo,
                                         uint32_t width, uint32_t height) {
        // Build a descriptor-info chain: scene → pp[0] → pp[1] → ...
        VkDescriptorImageInfo currentInput = sceneImageInfo;
        int ppIndex = 0;

        VkClearValue clearValue{};
        clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderPassBeginInfo rpBI{};
        rpBI.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBI.renderPass  = m_RenderPass;
        rpBI.renderArea  = {{0, 0}, {width, height}};
        rpBI.clearValueCount = 1;
        rpBI.pClearValues    = &clearValue;

        VkViewport viewport{};
        viewport.width    = static_cast<float>(width);
        viewport.height   = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{{0, 0}, {width, height}};

        for (auto& effect : m_Effects) {
            if (!effect->Enabled) continue;

            rpBI.framebuffer = m_PingPong[ppIndex].Framebuffer;
            vkCmdBeginRenderPass(cmd, &rpBI, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            effect->apply(cmd, currentInput, width, height);

            vkCmdEndRenderPass(cmd);

            // Next effect reads from the image we just wrote
            currentInput.imageView   = m_PingPong[ppIndex].View;
            currentInput.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            currentInput.sampler     = m_PingPong[ppIndex].Sampler;

            ppIndex = 1 - ppIndex;
        }

        // Return the last written image view; callers can blit or present it.
        // If no effects ran, return the scene view unchanged.
        if (currentInput.imageView != sceneImageInfo.imageView) {
            return currentInput.imageView;
        }
        return sceneImageInfo.imageView;
    }

    // =========================================================================
    // Built-in effects — Vulkan stubs
    //
    // These effects record a fullscreen-triangle draw using SPIR-V pipelines
    // loaded at init() time.  The actual pipeline creation requires a
    // VulkanPipeline helper (Engine/Graphics/Vulkan/) and SPIR-V in
    // Assets/Shaders/PostProcess/.  This file provides the interface and
    // records the structural Vulkan commands; the heavy SPIR-V plumbing is
    // intentionally left to the build-time shader compiler so that shaders
    // are hot-reloadable.
    //
    // Push-constant layout (std430, 16-byte aligned):
    //   float params[4];   // effect-specific (see each apply())
    // =========================================================================

    // -------------------------------------------------------------------------
    // ToneMappingEffect
    // -------------------------------------------------------------------------

    void ToneMappingEffect::init(uint32_t /*width*/, uint32_t /*height*/,
                                   VkRenderPass /*renderPass*/) {
        Priority = 90;
        // TODO: Load Assets/Shaders/PostProcess/tonemapping.vert.spv + .frag.spv
        //       and create a VkPipeline + descriptor set layout.
        GE_CORE_INFO("ToneMappingEffect: init (pipeline creation deferred to shader build)");
    }

    void ToneMappingEffect::resize(uint32_t, uint32_t) {}

    void ToneMappingEffect::apply(VkCommandBuffer cmd,
                                   const VkDescriptorImageInfo& /*inputImageInfo*/,
                                   uint32_t, uint32_t) {
        // Push constants: [exposure, gamma, algorithm, 0]
        struct PC { float exposure; float gamma; int algorithm; int pad; } pc;
        pc.exposure   = Exposure;
        pc.gamma      = Gamma;
        pc.algorithm  = static_cast<int>(CurrentAlgorithm);
        pc.pad        = 0;

        // vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
        // vkCmdBindDescriptorSets(cmd, ..., &inputDescriptorSet, ...);
        // vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);
        // vkCmdDraw(cmd, 3, 1, 0, 0);  // Fullscreen triangle
        (void)cmd; (void)pc;
    }

    void ToneMappingEffect::renderUI() {}

    // -------------------------------------------------------------------------
    // FXAAEffect
    // -------------------------------------------------------------------------

    void FXAAEffect::init(uint32_t /*width*/, uint32_t /*height*/,
                           VkRenderPass /*renderPass*/) {
        Priority = 100;
        GE_CORE_INFO("FXAAEffect: init");
    }

    void FXAAEffect::resize(uint32_t, uint32_t) {}

    void FXAAEffect::apply(VkCommandBuffer cmd,
                            const VkDescriptorImageInfo& /*inputImageInfo*/,
                            uint32_t width, uint32_t height) {
        struct PC { float texelW; float texelH; float subpixel; float edgeThreshold; } pc;
        pc.texelW          = 1.0f / static_cast<float>(width);
        pc.texelH          = 1.0f / static_cast<float>(height);
        pc.subpixel        = SubpixelQuality;
        pc.edgeThreshold   = EdgeThreshold;
        (void)cmd; (void)pc;
    }

    // -------------------------------------------------------------------------
    // VignetteEffect
    // -------------------------------------------------------------------------

    void VignetteEffect::init(uint32_t /*width*/, uint32_t /*height*/,
                               VkRenderPass /*renderPass*/) {
        Priority = 95;
        GE_CORE_INFO("VignetteEffect: init");
    }

    void VignetteEffect::resize(uint32_t, uint32_t) {}

    void VignetteEffect::apply(VkCommandBuffer cmd,
                                const VkDescriptorImageInfo& /*inputImageInfo*/,
                                uint32_t, uint32_t) {
        struct PC { float intensity; float smoothness; glm::vec2 pad; } pc;
        pc.intensity   = Intensity;
        pc.smoothness  = Smoothness;
        (void)cmd; (void)pc;
    }

    // -------------------------------------------------------------------------
    // ColorGradingEffect
    // -------------------------------------------------------------------------

    void ColorGradingEffect::init(uint32_t /*width*/, uint32_t /*height*/,
                                   VkRenderPass /*renderPass*/) {
        Priority = 85;
        GE_CORE_INFO("ColorGradingEffect: init");
    }

    void ColorGradingEffect::resize(uint32_t, uint32_t) {}

    void ColorGradingEffect::apply(VkCommandBuffer cmd,
                                    const VkDescriptorImageInfo& /*inputImageInfo*/,
                                    uint32_t, uint32_t) {
        struct PC {
            float saturation;
            float contrast;
            float brightness;
            float temperature;
            glm::vec3 colorFilter;
            float pad;
        } pc;
        pc.saturation  = Saturation;
        pc.contrast    = Contrast;
        pc.brightness  = Brightness;
        pc.temperature = Temperature;
        pc.colorFilter = ColorFilter;
        (void)cmd; (void)pc;
    }

    void ColorGradingEffect::renderUI() {}

    // -------------------------------------------------------------------------
    // BloomEffect
    // -------------------------------------------------------------------------

    void BloomEffect::init(uint32_t /*width*/, uint32_t /*height*/,
                            VkRenderPass /*renderPass*/) {
        Priority = 10;
        GE_CORE_INFO("BloomEffect: init");
    }

    void BloomEffect::resize(uint32_t, uint32_t) {}

    void BloomEffect::apply(VkCommandBuffer cmd,
                             const VkDescriptorImageInfo& /*inputImageInfo*/,
                             uint32_t, uint32_t) {
        // Three-pass approach:
        //   1. Bright-pass into half-resolution FBO
        //   2. Separable Gaussian blur (BlurPasses * 2 render passes)
        //   3. Additive combine into output FBO
        // Each pass records vkCmdBeginRenderPass / draw fullscreen triangle / vkCmdEndRenderPass.
        struct PC { float threshold; float intensity; float pad[2]; } pc;
        pc.threshold = Threshold;
        pc.intensity = Intensity;
        (void)cmd; (void)pc;
    }

    void BloomEffect::renderUI() {}

} // namespace GameEngine
