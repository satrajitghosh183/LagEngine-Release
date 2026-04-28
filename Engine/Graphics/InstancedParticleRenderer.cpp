#include "InstancedParticleRenderer.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <algorithm>
#include <cassert>

namespace GameEngine {

    // =========================================================================
    // Static definitions
    // =========================================================================

    VkBuffer      InstancedParticleRenderer::s_QuadVB          = VK_NULL_HANDLE;
    VmaAllocation InstancedParticleRenderer::s_QuadVBAlloc     = VK_NULL_HANDLE;

    VkBuffer      InstancedParticleRenderer::s_InstanceVB      = VK_NULL_HANDLE;
    VmaAllocation InstancedParticleRenderer::s_InstanceVBAlloc = VK_NULL_HANDLE;
    void*         InstancedParticleRenderer::s_InstanceMapped  = nullptr;
    uint32_t      InstancedParticleRenderer::s_MaxParticles    = 10000;

    Ref<Texture2D> InstancedParticleRenderer::s_DefaultTexture = nullptr;

    std::unique_ptr<VulkanDescriptorSetLayout> InstancedParticleRenderer::s_DescLayout;
    std::unique_ptr<VulkanDescriptorPool>      InstancedParticleRenderer::s_DescPool;
    VkSampler      InstancedParticleRenderer::s_Sampler        = VK_NULL_HANDLE;
    VkPipelineLayout InstancedParticleRenderer::s_PipelineLayout = VK_NULL_HANDLE;

    std::vector<InstancedParticleRenderer::PipelineEntry> InstancedParticleRenderer::s_Pipelines;

    glm::mat4 InstancedParticleRenderer::s_ViewProjection = glm::mat4(1.0f);
    glm::vec3 InstancedParticleRenderer::s_CameraRight    = glm::vec3(1, 0, 0);
    glm::vec3 InstancedParticleRenderer::s_CameraUp       = glm::vec3(0, 1, 0);

    int   InstancedParticleRenderer::s_AtlasColumns    = 1;
    int   InstancedParticleRenderer::s_AtlasRows       = 1;
    bool  InstancedParticleRenderer::s_SoftParticles   = false;
    float InstancedParticleRenderer::s_SoftFadeDistance = 0.5f;

    VkRenderPass InstancedParticleRenderer::s_RenderPass = VK_NULL_HANDLE;

    InstancedParticleRenderer::Statistics InstancedParticleRenderer::s_Stats;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    void InstancedParticleRenderer::Init(VkRenderPass renderPass, uint32_t maxParticles) {
        s_MaxParticles = maxParticles;
        s_RenderPass   = renderPass;

        CreateQuadBuffers();
        CreateInstanceBuffer(maxParticles);
        CreateDefaultTexture();
        CreateDescriptorResources();

        // Pre-create pipeline for default blend mode (Alpha)
        CreatePipeline(renderPass, BlendMode::Alpha);

        GE_CORE_INFO("InstancedParticleRenderer initialized (Vulkan, max {} particles)", maxParticles);
    }

    void InstancedParticleRenderer::Shutdown() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        vkDeviceWaitIdle(d);

        for (auto& e : s_Pipelines) e.Pipeline.Destroy();
        s_Pipelines.clear();

        if (s_PipelineLayout) { vkDestroyPipelineLayout(d, s_PipelineLayout, nullptr); s_PipelineLayout = VK_NULL_HANDLE; }
        if (s_Sampler)        { vkDestroySampler(d, s_Sampler, nullptr);               s_Sampler        = VK_NULL_HANDLE; }

        s_DescLayout.reset();
        s_DescPool.reset();

        if (s_QuadVB)      { vmaDestroyBuffer(a, s_QuadVB,      s_QuadVBAlloc);      s_QuadVB      = VK_NULL_HANDLE; }
        if (s_InstanceVB)  { vmaDestroyBuffer(a, s_InstanceVB,  s_InstanceVBAlloc);  s_InstanceVB  = VK_NULL_HANDLE; }

        s_DefaultTexture.reset();
    }

    // =========================================================================
    // Resource creation
    // =========================================================================

    void InstancedParticleRenderer::CreateQuadBuffers() {
        // Billboard quad: two triangles, positions [-0.5, 0.5] x [-0.5, 0.5]
        // Binding 0, location 0 = vec2 position, location 1 = vec2 texcoord
        struct QuadVertex { glm::vec2 Pos; glm::vec2 UV; };
        static const QuadVertex verts[6] = {
            {{-0.5f, -0.5f}, {0, 0}},
            {{ 0.5f, -0.5f}, {1, 0}},
            {{ 0.5f,  0.5f}, {1, 1}},
            {{-0.5f, -0.5f}, {0, 0}},
            {{ 0.5f,  0.5f}, {1, 1}},
            {{-0.5f,  0.5f}, {0, 1}},
        };

        VkDeviceSize size = sizeof(verts);
        auto& dev = VulkanDevice::Get();
        VmaAllocator a = dev.GetAllocator();

        // Staging
        VkBuffer stageBuf;
        VmaAllocation stageAlloc;
        VmaAllocationInfo stageInfo{};
        VkBufferCreateInfo sbci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        sbci.size  = size;
        sbci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo saci{};
        saci.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        saci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(a, &sbci, &saci, &stageBuf, &stageAlloc, &stageInfo);
        std::memcpy(stageInfo.pMappedData, verts, size);

        // Device-local
        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = size;
        bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(a, &bci, &vaci, &s_QuadVB, &s_QuadVBAlloc, nullptr);

        VkCommandBuffer cmd = dev.BeginSingleTimeCommands();
        VkBufferCopy copy{0, 0, size};
        vkCmdCopyBuffer(cmd, stageBuf, s_QuadVB, 1, &copy);
        dev.EndSingleTimeCommands(cmd);
        vmaDestroyBuffer(a, stageBuf, stageAlloc);
    }

    void InstancedParticleRenderer::CreateInstanceBuffer(uint32_t maxParticles) {
        auto& dev = VulkanDevice::Get();
        VkDeviceSize size = maxParticles * sizeof(ParticleInstance);

        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = size;
        bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo aci{};
        aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo ai{};
        vmaCreateBuffer(dev.GetAllocator(), &bci, &aci,
                        &s_InstanceVB, &s_InstanceVBAlloc, &ai);
        s_InstanceMapped = ai.pMappedData;
    }

    void InstancedParticleRenderer::CreateDefaultTexture() {
        s_DefaultTexture = CreateRef<Texture2D>(1, 1, TextureFormat::RGBA);
        uint32_t white = 0xFFFFFFFF;
        s_DefaultTexture->SetData(&white, sizeof(white));
    }

    void InstancedParticleRenderer::CreateDescriptorResources() {
        VkDevice d = VulkanDevice::Get().GetDevice();

        // Linear sampler, repeat
        VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sci.magFilter    = VK_FILTER_LINEAR;
        sci.minFilter    = VK_FILTER_LINEAR;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        vkCreateSampler(d, &sci, nullptr, &s_Sampler);

        // Descriptor layout: binding 0 = particle texture
        s_DescLayout = VulkanDescriptorSetLayout::Builder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT)
            .Build();

        s_DescPool = VulkanDescriptorPool::Builder()
            .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64)
            .SetMaxSets(64)
            .Build();
    }

    void InstancedParticleRenderer::CreatePipeline(VkRenderPass renderPass, BlendMode blendMode) {
        VkDevice d = VulkanDevice::Get().GetDevice();

        if (!s_PipelineLayout) {
            VkPushConstantRange pcr{};
            pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
            pcr.size       = sizeof(PushConstants);

            VkDescriptorSetLayout setLayout = s_DescLayout->GetDescriptorSetLayout();
            VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
            plci.setLayoutCount         = 1;
            plci.pSetLayouts            = &setLayout;
            plci.pushConstantRangeCount = 1;
            plci.pPushConstantRanges    = &pcr;
            vkCreatePipelineLayout(d, &plci, nullptr, &s_PipelineLayout);
        }

        PipelineConfigInfo cfg{};
        VulkanPipeline::DefaultPipelineConfigInfo(cfg);
        cfg.RenderPass     = renderPass;
        cfg.PipelineLayout = s_PipelineLayout;

        // Depth test on, depth write off (particles are transparent)
        cfg.DepthStencilInfo.depthTestEnable  = VK_TRUE;
        cfg.DepthStencilInfo.depthWriteEnable = VK_FALSE;

        switch (blendMode) {
            case BlendMode::Additive: {
                cfg.ColorBlendAttachment.blendEnable         = VK_TRUE;
                cfg.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
                cfg.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
                cfg.ColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
                cfg.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                cfg.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                cfg.ColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
                break;
            }
            case BlendMode::Multiply: {
                cfg.ColorBlendAttachment.blendEnable         = VK_TRUE;
                cfg.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
                cfg.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                cfg.ColorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
                cfg.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                cfg.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                cfg.ColorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
                break;
            }
            default: // Alpha
                VulkanPipeline::EnableAlphaBlending(cfg);
                break;
        }
        cfg.ColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        cfg.ColorBlendInfo.attachmentCount = 1;
        cfg.ColorBlendInfo.pAttachments    = &cfg.ColorBlendAttachment;

        PipelineEntry entry{};
        entry.Mode = blendMode;
        entry.Pipeline.CreateGraphicsPipeline(
            "Assets/Shaders/Particle.vert.spv",
            "Assets/Shaders/Particle.frag.spv",
            cfg);
        s_Pipelines.push_back(std::move(entry));
    }

    VulkanPipeline* InstancedParticleRenderer::GetOrCreatePipeline(VkRenderPass renderPass,
                                                                     BlendMode mode) {
        for (auto& e : s_Pipelines) {
            if (e.Mode == mode) return &e.Pipeline;
        }
        CreatePipeline(renderPass, mode);
        return &s_Pipelines.back().Pipeline;
    }

    // =========================================================================
    // Per-frame
    // =========================================================================

    void InstancedParticleRenderer::Begin(const Camera3D& camera) {
        s_ViewProjection = camera.GetViewProjectionMatrix();
        glm::mat4 view   = camera.GetViewMatrix();
        // Column-major: view[col][row]
        s_CameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        s_CameraUp    = glm::vec3(view[0][1], view[1][1], view[2][1]);
        ResetStats();
    }

    void InstancedParticleRenderer::End() {}

    // =========================================================================
    // Draw
    // =========================================================================

    void InstancedParticleRenderer::DrawParticles(
        VkCommandBuffer cmd,
        const std::vector<ParticleInstance>& particles,
        const Ref<Texture2D>& texture,
        BlendMode blendMode) {

        if (particles.empty()) return;

        uint32_t count = static_cast<uint32_t>(
            std::min(particles.size(), static_cast<size_t>(s_MaxParticles)));

        // Upload instance data
        std::memcpy(s_InstanceMapped, particles.data(), count * sizeof(ParticleInstance));
        vmaFlushAllocation(VulkanDevice::Get().GetAllocator(),
                           s_InstanceVBAlloc, 0, count * sizeof(ParticleInstance));

        // Bind texture descriptor
        const Ref<Texture2D>& tex = texture ? texture : s_DefaultTexture;
        VkDescriptorImageInfo imgInfo{};
        imgInfo.sampler     = s_Sampler;
        imgInfo.imageView   = tex->GetImageView();
        imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorSet descSet = VK_NULL_HANDLE;
        VulkanDescriptorWriter(*s_DescLayout, *s_DescPool)
            .WriteImage(0, &imgInfo)
            .Build(descSet);

        // Select pipeline
        VulkanPipeline* pipeline = GetOrCreatePipeline(s_RenderPass, blendMode);
        pipeline->Bind(cmd);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                s_PipelineLayout, 0, 1, &descSet, 0, nullptr);

        // Push constants
        PushConstants pc{};
        pc.ViewProjection   = s_ViewProjection;
        pc.CameraRight      = glm::vec4(s_CameraRight, 0.0f);
        pc.CameraUp         = glm::vec4(s_CameraUp, 0.0f);
        pc.AtlasSize        = glm::ivec2(s_AtlasColumns, s_AtlasRows);
        pc.SoftFadeDistance = s_SoftFadeDistance;
        pc.SoftParticles    = s_SoftParticles ? 1 : 0;
        vkCmdPushConstants(cmd, s_PipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pc), &pc);

        // Bind vertex buffers: binding 0 = quad, binding 1 = instance data
        VkBuffer vbs[2]     = {s_QuadVB, s_InstanceVB};
        VkDeviceSize offs[2] = {0, 0};
        vkCmdBindVertexBuffers(cmd, 0, 2, vbs, offs);

        // Draw 6 vertices (2 triangles per quad) x count instances
        vkCmdDraw(cmd, 6, count, 0, 0);

        s_Stats.DrawCalls++;
        s_Stats.ParticlesRendered += count;
        s_Stats.BatchCount++;
    }

    // =========================================================================
    // Config
    // =========================================================================

    void InstancedParticleRenderer::SetAtlasSize(int columns, int rows) {
        s_AtlasColumns = columns;
        s_AtlasRows    = rows;
    }

    void InstancedParticleRenderer::SetSoftParticles(bool enabled, float fadeDistance) {
        s_SoftParticles   = enabled;
        s_SoftFadeDistance = fadeDistance;
    }

} // namespace GameEngine
