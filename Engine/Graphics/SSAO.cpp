#include "SSAO.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include <array>
#include <cstring>

namespace GameEngine {

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    SSAO::~SSAO() { Shutdown(); }

    void SSAO::Init(uint32_t width, uint32_t height) {
        m_Width  = width;
        m_Height = height;

        GenerateKernel();
        CreateSampler();
        CreateNoiseTexture();
        CreateImages(width, height);
        CreateRenderPasses();
        CreateFramebuffers(width, height);
        CreateDescriptorLayouts();
        CreatePipelines();

        GE_CORE_INFO("SSAO initialized ({}x{}, kernel {})", width, height, KernelSize);
    }

    void SSAO::Resize(uint32_t width, uint32_t height) {
        if (m_Width == width && m_Height == height) return;

        auto& dev = VulkanDevice::Get();
        vkDeviceWaitIdle(dev.GetDevice());

        // Destroy size-dependent resources only
        if (m_SSAOFB)   { vkDestroyFramebuffer(dev.GetDevice(), m_SSAOFB, nullptr);   m_SSAOFB   = VK_NULL_HANDLE; }
        if (m_BlurFB)   { vkDestroyFramebuffer(dev.GetDevice(), m_BlurFB, nullptr);   m_BlurFB   = VK_NULL_HANDLE; }
        DestroyImages();

        m_Width  = width;
        m_Height = height;
        CreateImages(width, height);
        CreateFramebuffers(width, height);
    }

    void SSAO::Shutdown() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        vkDeviceWaitIdle(d);

        if (m_SSAOPipelineLayout) { vkDestroyPipelineLayout(d, m_SSAOPipelineLayout, nullptr); m_SSAOPipelineLayout = VK_NULL_HANDLE; }
        if (m_BlurPipelineLayout) { vkDestroyPipelineLayout(d, m_BlurPipelineLayout, nullptr); m_BlurPipelineLayout = VK_NULL_HANDLE; }

        m_SSAOPipeline.Destroy();
        m_BlurPipeline.Destroy();

        m_SSAOLayout.reset();
        m_BlurLayout.reset();
        m_DescPool.reset();

        if (m_SSAOFB)         { vkDestroyFramebuffer(d, m_SSAOFB, nullptr);          m_SSAOFB          = VK_NULL_HANDLE; }
        if (m_BlurFB)         { vkDestroyFramebuffer(d, m_BlurFB, nullptr);           m_BlurFB          = VK_NULL_HANDLE; }
        if (m_SSAORenderPass) { vkDestroyRenderPass(d, m_SSAORenderPass, nullptr);    m_SSAORenderPass  = VK_NULL_HANDLE; }
        if (m_BlurRenderPass) { vkDestroyRenderPass(d, m_BlurRenderPass, nullptr);    m_BlurRenderPass  = VK_NULL_HANDLE; }

        DestroyImages();

        if (m_NoiseView)  { vkDestroyImageView(d, m_NoiseView, nullptr);              m_NoiseView   = VK_NULL_HANDLE; }
        if (m_NoiseImage) { vmaDestroyImage(a, m_NoiseImage, m_NoiseAlloc);           m_NoiseImage  = VK_NULL_HANDLE; }

        if (m_NoiseSampler)  { vkDestroySampler(d, m_NoiseSampler, nullptr);         m_NoiseSampler  = VK_NULL_HANDLE; }
        if (m_ResultSampler) { vkDestroySampler(d, m_ResultSampler, nullptr);        m_ResultSampler = VK_NULL_HANDLE; }

        if (m_KernelBuffer) { vmaDestroyBuffer(a, m_KernelBuffer, m_KernelAlloc);    m_KernelBuffer = VK_NULL_HANDLE; }
    }

    // -------------------------------------------------------------------------
    // Kernel
    // -------------------------------------------------------------------------

    void SSAO::GenerateKernel() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::default_random_engine gen;
        m_Kernel.clear();
        m_Kernel.reserve(64);
        for (int i = 0; i < 64; i++) {
            glm::vec3 s(dist(gen) * 2.0f - 1.0f,
                        dist(gen) * 2.0f - 1.0f,
                        dist(gen));
            s = glm::normalize(s) * dist(gen);
            float scale = float(i) / 64.0f;
            scale = 0.1f + scale * scale * 0.9f;
            s *= scale;
            m_Kernel.push_back(glm::vec4(s, 0.0f));
        }

        // Upload kernel to a GPU buffer so the shader can read it as a UBO/SSBO
        auto& dev = VulkanDevice::Get();
        VkDeviceSize bufSize = 64 * sizeof(glm::vec4);

        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = bufSize;
        bci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo aci{};
        aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo ai{};
        vmaCreateBuffer(dev.GetAllocator(), &bci, &aci, &m_KernelBuffer, &m_KernelAlloc, &ai);
        std::memcpy(ai.pMappedData, m_Kernel.data(), bufSize);
    }

    // -------------------------------------------------------------------------
    // Noise texture (4x4, VK_FORMAT_R32G32B32A32_SFLOAT)
    // -------------------------------------------------------------------------

    void SSAO::CreateNoiseTexture() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::default_random_engine gen;
        std::array<glm::vec4, 16> noise{};
        for (auto& n : noise) {
            n = glm::vec4(dist(gen) * 2.0f - 1.0f, dist(gen) * 2.0f - 1.0f, 0.0f, 0.0f);
        }

        VkExtent3D ext{4, 4, 1};
        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType   = VK_IMAGE_TYPE_2D;
        ici.format      = VK_FORMAT_R32G32B32A32_SFLOAT;
        ici.extent      = ext;
        ici.mipLevels   = 1;
        ici.arrayLayers = 1;
        ici.samples     = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling      = VK_IMAGE_TILING_OPTIMAL;
        ici.usage       = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateImage(a, &ici, &vaci, &m_NoiseImage, &m_NoiseAlloc, nullptr);

        // Upload via staging buffer
        VkDeviceSize dataSize = sizeof(noise);
        VkBuffer stageBuf;
        VmaAllocation stageAlloc;
        VmaAllocationInfo stageInfo{};

        VkBufferCreateInfo sbci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        sbci.size  = dataSize;
        sbci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo saci{};
        saci.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        saci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(a, &sbci, &saci, &stageBuf, &stageAlloc, &stageInfo);
        std::memcpy(stageInfo.pMappedData, noise.data(), dataSize);

        VkCommandBuffer cmd = dev.BeginSingleTimeCommands();
        TransitionLayout(cmd, m_NoiseImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = ext;
        vkCmdCopyBufferToImage(cmd, stageBuf, m_NoiseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        TransitionLayout(cmd, m_NoiseImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        dev.EndSingleTimeCommands(cmd);
        vmaDestroyBuffer(a, stageBuf, stageAlloc);

        // Image view
        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image    = m_NoiseImage;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(d, &ivci, nullptr, &m_NoiseView);
    }

    // -------------------------------------------------------------------------
    // Samplers
    // -------------------------------------------------------------------------

    void SSAO::CreateSampler() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();

        auto makeSampler = [&](VkSamplerAddressMode wrap, VkSampler& out) {
            VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            sci.magFilter    = VK_FILTER_NEAREST;
            sci.minFilter    = VK_FILTER_NEAREST;
            sci.addressModeU = wrap;
            sci.addressModeV = wrap;
            sci.addressModeW = wrap;
            vkCreateSampler(d, &sci, nullptr, &out);
        };

        makeSampler(VK_SAMPLER_ADDRESS_MODE_REPEAT,          m_NoiseSampler);
        makeSampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,   m_ResultSampler);
    }

    // -------------------------------------------------------------------------
    // Images (size-dependent)
    // -------------------------------------------------------------------------

    void SSAO::CreateColorImage(uint32_t width, uint32_t height, VkFormat fmt,
                                VkImage& outImage, VmaAllocation& outAlloc,
                                VkImageView& outView) {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType   = VK_IMAGE_TYPE_2D;
        ici.format      = fmt;
        ici.extent      = {width, height, 1};
        ici.mipLevels   = 1;
        ici.arrayLayers = 1;
        ici.samples     = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling      = VK_IMAGE_TILING_OPTIMAL;
        ici.usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateImage(a, &ici, &vaci, &outImage, &outAlloc, nullptr);

        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image    = outImage;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format   = fmt;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(d, &ivci, nullptr, &outView);
    }

    void SSAO::CreateImages(uint32_t width, uint32_t height) {
        // Both SSAO and blur results use a single-channel float format
        CreateColorImage(width, height, VK_FORMAT_R8_UNORM,
                         m_SSAOImage, m_SSAOAlloc, m_SSAOView);
        CreateColorImage(width, height, VK_FORMAT_R8_UNORM,
                         m_BlurAOImage, m_BlurAOAlloc, m_BlurAOView);
    }

    void SSAO::DestroyImages() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        if (m_SSAOView)   { vkDestroyImageView(d, m_SSAOView, nullptr);               m_SSAOView   = VK_NULL_HANDLE; }
        if (m_SSAOImage)  { vmaDestroyImage(a, m_SSAOImage, m_SSAOAlloc);             m_SSAOImage  = VK_NULL_HANDLE; }
        if (m_BlurAOView) { vkDestroyImageView(d, m_BlurAOView, nullptr);             m_BlurAOView = VK_NULL_HANDLE; }
        if (m_BlurAOImage){ vmaDestroyImage(a, m_BlurAOImage, m_BlurAOAlloc);         m_BlurAOImage= VK_NULL_HANDLE; }
    }

    // -------------------------------------------------------------------------
    // Render passes
    // -------------------------------------------------------------------------

    static VkRenderPass CreateSingleColorRenderPass(VkDevice d, VkFormat fmt) {
        VkAttachmentDescription att{};
        att.format         = fmt;
        att.samples        = VK_SAMPLE_COUNT_1_BIT;
        att.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        att.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        att.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        att.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        att.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference ref{};
        ref.attachment = 0;
        ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription sub{};
        sub.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        sub.colorAttachmentCount = 1;
        sub.pColorAttachments    = &ref;

        std::array<VkSubpassDependency, 2> deps{};
        deps[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        deps[0].dstSubpass    = 0;
        deps[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        deps[1].srcSubpass    = 0;
        deps[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
        deps[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        deps[1].dstStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        rpci.attachmentCount = 1;
        rpci.pAttachments    = &att;
        rpci.subpassCount    = 1;
        rpci.pSubpasses      = &sub;
        rpci.dependencyCount = static_cast<uint32_t>(deps.size());
        rpci.pDependencies   = deps.data();

        VkRenderPass rp;
        vkCreateRenderPass(d, &rpci, nullptr, &rp);
        return rp;
    }

    void SSAO::CreateRenderPasses() {
        VkDevice d = VulkanDevice::Get().GetDevice();
        m_SSAORenderPass = CreateSingleColorRenderPass(d, VK_FORMAT_R8_UNORM);
        m_BlurRenderPass = CreateSingleColorRenderPass(d, VK_FORMAT_R8_UNORM);
    }

    // -------------------------------------------------------------------------
    // Framebuffers
    // -------------------------------------------------------------------------

    void SSAO::CreateFramebuffers(uint32_t width, uint32_t height) {
        VkDevice d = VulkanDevice::Get().GetDevice();

        auto makeFB = [&](VkRenderPass rp, VkImageView view, VkFramebuffer& out) {
            VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            fci.renderPass      = rp;
            fci.attachmentCount = 1;
            fci.pAttachments    = &view;
            fci.width           = width;
            fci.height          = height;
            fci.layers          = 1;
            vkCreateFramebuffer(d, &fci, nullptr, &out);
        };

        makeFB(m_SSAORenderPass, m_SSAOView,   m_SSAOFB);
        makeFB(m_BlurRenderPass, m_BlurAOView, m_BlurFB);
    }

    // -------------------------------------------------------------------------
    // Descriptor layouts and pool
    // -------------------------------------------------------------------------

    void SSAO::CreateDescriptorLayouts() {
        // SSAO pass reads: position (0), normal (1), noise (2), kernel UBO (3), proj UBO (4)
        m_SSAOLayout = VulkanDescriptorSetLayout::Builder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
            .AddBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT)
            .Build();

        // Blur pass reads: SSAO result (0)
        m_BlurLayout = VulkanDescriptorSetLayout::Builder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .Build();

        m_DescPool = VulkanDescriptorPool::Builder()
            .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8)
            .AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         4)
            .SetMaxSets(4)
            .Build();
    }

    // -------------------------------------------------------------------------
    // Pipelines (full-screen triangle — no vertex input)
    // -------------------------------------------------------------------------

    void SSAO::CreatePipelines() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();

        // ---- SSAO pipeline layout (push-constants for radius/bias/kernelSize/noiseScale) ----
        {
            VkPushConstantRange pcr{};
            pcr.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            pcr.offset     = 0;
            pcr.size       = sizeof(SSAOPushConstants);

            VkDescriptorSetLayout setLayout = m_SSAOLayout->GetDescriptorSetLayout();
            VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            plci.setLayoutCount         = 1;
            plci.pSetLayouts            = &setLayout;
            plci.pushConstantRangeCount = 1;
            plci.pPushConstantRanges    = &pcr;
            vkCreatePipelineLayout(d, &plci, nullptr, &m_SSAOPipelineLayout);
        }

        // ---- Blur pipeline layout ----
        {
            VkDescriptorSetLayout setLayout = m_BlurLayout->GetDescriptorSetLayout();
            VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            plci.setLayoutCount = 1;
            plci.pSetLayouts    = &setLayout;
            vkCreatePipelineLayout(d, &plci, nullptr, &m_BlurPipelineLayout);
        }

        // Full-screen triangle: positions generated in vertex shader via gl_VertexIndex.
        // Shaders are compiled from GLSL to SPIR-V at build time and loaded from
        // Assets/Shaders/SSAO.vert.spv / SSAO.frag.spv / SSAOBlur.frag.spv.
        // PipelineConfigInfo: no vertex input, triangle list, no depth test/write.

        auto makeCfg = [&](VkRenderPass rp, VkPipelineLayout layout) {
            PipelineConfigInfo cfg{};
            VulkanPipeline::DefaultPipelineConfigInfo(cfg);
            cfg.RenderPass                                    = rp;
            cfg.PipelineLayout                                = layout;
            cfg.DepthStencilInfo.depthTestEnable              = VK_FALSE;
            cfg.DepthStencilInfo.depthWriteEnable             = VK_FALSE;
            cfg.InputAssemblyInfo.topology                    = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            // No vertex input bindings — full-screen triangle from vertex shader
            return cfg;
        };

        PipelineConfigInfo ssaoCfg = makeCfg(m_SSAORenderPass, m_SSAOPipelineLayout);
        m_SSAOPipeline.CreateGraphicsPipeline(
            "Assets/Shaders/SSAO.vert.spv",
            "Assets/Shaders/SSAO.frag.spv",
            ssaoCfg);

        PipelineConfigInfo blurCfg = makeCfg(m_BlurRenderPass, m_BlurPipelineLayout);
        m_BlurPipeline.CreateGraphicsPipeline(
            "Assets/Shaders/SSAO.vert.spv",
            "Assets/Shaders/SSAOBlur.frag.spv",
            blurCfg);
    }

    // -------------------------------------------------------------------------
    // Layout transition helper
    // -------------------------------------------------------------------------

    void SSAO::TransitionLayout(VkCommandBuffer cmd, VkImage image,
                                VkImageLayout oldL, VkImageLayout newL) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout           = oldL;
        barrier.newLayout           = newL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;

        VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if (oldL == VK_IMAGE_LAYOUT_UNDEFINED && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newL == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            src = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dst = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(cmd, src, dst, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    // -------------------------------------------------------------------------
    // Compute (SSAO pass)
    // -------------------------------------------------------------------------

    void SSAO::Compute(VkCommandBuffer cmd,
                       VkImageView posView,
                       VkImageView normalView,
                       VkSampler sampler,
                       VkDescriptorBufferInfo projBufInfo) {
        if (!Enabled) return;

        // Update / rebuild descriptor set for this call
        VkDescriptorImageInfo posInfo{};
        posInfo.sampler     = sampler;
        posInfo.imageView   = posView;
        posInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo normInfo{};
        normInfo.sampler     = sampler;
        normInfo.imageView   = normalView;
        normInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo noiseInfo{};
        noiseInfo.sampler     = m_NoiseSampler;
        noiseInfo.imageView   = m_NoiseView;
        noiseInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorBufferInfo kernelInfo{};
        kernelInfo.buffer = m_KernelBuffer;
        kernelInfo.offset = 0;
        kernelInfo.range  = 64 * sizeof(glm::vec4);

        VulkanDescriptorWriter(*m_SSAOLayout, *m_DescPool)
            .WriteImage(0, &posInfo)
            .WriteImage(1, &normInfo)
            .WriteImage(2, &noiseInfo)
            .WriteBuffer(3, &kernelInfo)
            .WriteBuffer(4, &projBufInfo)
            .Build(m_SSAOSet);

        // Begin render pass
        VkClearValue clear{};
        clear.color = {1.0f, 0.0f, 0.0f, 0.0f};

        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass        = m_SSAORenderPass;
        rpbi.framebuffer       = m_SSAOFB;
        rpbi.renderArea.extent = {m_Width, m_Height};
        rpbi.clearValueCount   = 1;
        rpbi.pClearValues      = &clear;
        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        m_SSAOPipeline.Bind(cmd);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_SSAOPipelineLayout, 0, 1, &m_SSAOSet, 0, nullptr);

        SSAOPushConstants pc{};
        pc.Radius      = Radius;
        pc.Bias        = Bias;
        pc.KernelSize  = std::min(KernelSize, 64);
        pc.NoiseScaleX = static_cast<float>(m_Width)  / 4.0f;
        pc.NoiseScaleY = static_cast<float>(m_Height) / 4.0f;
        vkCmdPushConstants(cmd, m_SSAOPipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc), &pc);

        // Full-screen triangle: 3 vertices, 0 instances
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
    }

    // -------------------------------------------------------------------------
    // Blur pass
    // -------------------------------------------------------------------------

    void SSAO::Blur(VkCommandBuffer cmd) {
        // Bind the raw SSAO result as input
        VkDescriptorImageInfo ssaoInfo{};
        ssaoInfo.sampler     = m_ResultSampler;
        ssaoInfo.imageView   = m_SSAOView;
        ssaoInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VulkanDescriptorWriter(*m_BlurLayout, *m_DescPool)
            .WriteImage(0, &ssaoInfo)
            .Build(m_BlurSet);

        VkClearValue clear{};
        clear.color = {1.0f, 0.0f, 0.0f, 0.0f};

        VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        rpbi.renderPass        = m_BlurRenderPass;
        rpbi.framebuffer       = m_BlurFB;
        rpbi.renderArea.extent = {m_Width, m_Height};
        rpbi.clearValueCount   = 1;
        rpbi.pClearValues      = &clear;
        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        m_BlurPipeline.Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_BlurPipelineLayout, 0, 1, &m_BlurSet, 0, nullptr);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRenderPass(cmd);
    }

    // -------------------------------------------------------------------------
    // Descriptor info for downstream passes
    // -------------------------------------------------------------------------

    VkDescriptorImageInfo SSAO::GetBlurredAODescriptorInfo() const {
        VkDescriptorImageInfo info{};
        info.sampler     = m_ResultSampler;
        info.imageView   = m_BlurAOView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

} // namespace GameEngine
