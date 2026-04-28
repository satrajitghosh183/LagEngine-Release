#include "BatchRenderer2D.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>
#include <array>
#include <cassert>

namespace GameEngine {

    // =========================================================================
    // Internal state
    // =========================================================================

    struct Batch2DData {
        // ---- Vertex buffer (host-visible, persistently mapped) ----
        VkBuffer      VertexBuffer     = VK_NULL_HANDLE;
        VmaAllocation VertexAlloc      = VK_NULL_HANDLE;
        void*         VertexMapped     = nullptr;

        // ---- Index buffer (device-local, written once) ----
        VkBuffer      IndexBuffer      = VK_NULL_HANDLE;
        VmaAllocation IndexAlloc       = VK_NULL_HANDLE;

        // ---- CPU-side staging for vertices ----
        BatchRenderer2D::Vertex* VertexBase = nullptr;
        BatchRenderer2D::Vertex* VertexPtr  = nullptr;
        int QuadCount = 0;

        // ---- White fallback texture ----
        Ref<Texture2D> WhiteTexture;

        // ---- Texture slot management ----
        std::array<Ref<Texture2D>, BatchRenderer2D::MaxTextureSlots> TextureSlots;
        int TextureSlotIndex = 1; // slot 0 = white texture

        // ---- Descriptor resources ----
        std::unique_ptr<VulkanDescriptorSetLayout> SetLayout;
        std::unique_ptr<VulkanDescriptorPool>      DescPool;
        VkDescriptorSet DescSet = VK_NULL_HANDLE;
        VkSampler Sampler       = VK_NULL_HANDLE;

        // ---- Pipeline ----
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        VulkanPipeline   Pipeline;

        // ---- Per-frame state ----
        VkCommandBuffer ActiveCmd        = VK_NULL_HANDLE;
        glm::mat4       CurrentProjection = glm::mat4(1.0f);

        BatchRenderer2D::Stats RenderStats;
    };

    static Batch2DData s_Data;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    void BatchRenderer2D::Init(VkRenderPass renderPass) {
        s_Data.VertexBase = new Vertex[MaxVertices];

        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateWhiteTexture();
        CreateDescriptorResources();
        CreatePipeline(renderPass);

        GE_CORE_INFO("BatchRenderer2D initialized (Vulkan, max {} quads, {} texture slots)",
                     MaxQuads, MaxTextureSlots);
    }

    void BatchRenderer2D::Shutdown() {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        vkDeviceWaitIdle(d);

        s_Data.Pipeline.Destroy();
        if (s_Data.PipelineLayout) { vkDestroyPipelineLayout(d, s_Data.PipelineLayout, nullptr); }
        if (s_Data.Sampler)        { vkDestroySampler(d, s_Data.Sampler, nullptr); }

        s_Data.SetLayout.reset();
        s_Data.DescPool.reset();

        if (s_Data.VertexBuffer) { vmaDestroyBuffer(a, s_Data.VertexBuffer, s_Data.VertexAlloc); }
        if (s_Data.IndexBuffer)  { vmaDestroyBuffer(a, s_Data.IndexBuffer,  s_Data.IndexAlloc);  }

        s_Data.WhiteTexture.reset();
        for (auto& t : s_Data.TextureSlots) t.reset();

        delete[] s_Data.VertexBase;
        s_Data.VertexBase = nullptr;
    }

    // =========================================================================
    // Resource creation helpers
    // =========================================================================

    void BatchRenderer2D::CreateVertexBuffer() {
        auto& dev = VulkanDevice::Get();
        VkDeviceSize size = MaxVertices * sizeof(Vertex);

        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = size;
        bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo aci{};
        aci.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo ai{};
        vmaCreateBuffer(dev.GetAllocator(), &bci, &aci,
                        &s_Data.VertexBuffer, &s_Data.VertexAlloc, &ai);
        s_Data.VertexMapped = ai.pMappedData;
    }

    void BatchRenderer2D::CreateIndexBuffer() {
        auto& dev = VulkanDevice::Get();
        VmaAllocator a = dev.GetAllocator();

        // Generate indices on CPU, then upload to device-local buffer.
        std::vector<uint32_t> indices(MaxIndices);
        uint32_t offset = 0;
        for (int i = 0; i < MaxIndices; i += 6) {
            indices[i + 0] = offset + 0; indices[i + 1] = offset + 1; indices[i + 2] = offset + 2;
            indices[i + 3] = offset + 2; indices[i + 4] = offset + 3; indices[i + 5] = offset + 0;
            offset += 4;
        }
        VkDeviceSize size = indices.size() * sizeof(uint32_t);

        // Staging buffer
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
        std::memcpy(stageInfo.pMappedData, indices.data(), size);

        // Device-local index buffer
        VkBufferCreateInfo ibci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        ibci.size  = size;
        ibci.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        VmaAllocationCreateInfo iaci{};
        iaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateBuffer(a, &ibci, &iaci, &s_Data.IndexBuffer, &s_Data.IndexAlloc, nullptr);

        // Upload
        VkCommandBuffer cmd = dev.BeginSingleTimeCommands();
        VkBufferCopy copy{0, 0, size};
        vkCmdCopyBuffer(cmd, stageBuf, s_Data.IndexBuffer, 1, &copy);
        dev.EndSingleTimeCommands(cmd);

        vmaDestroyBuffer(a, stageBuf, stageAlloc);
    }

    void BatchRenderer2D::CreateWhiteTexture() {
        s_Data.WhiteTexture = CreateRef<Texture2D>(1, 1, TextureFormat::RGBA);
        uint32_t white = 0xFFFFFFFF;
        s_Data.WhiteTexture->SetData(&white, sizeof(white));
        s_Data.TextureSlots[0] = s_Data.WhiteTexture;
    }

    void BatchRenderer2D::CreateDescriptorResources() {
        VkDevice d = VulkanDevice::Get().GetDevice();

        // Sampler (linear, clamp-to-edge)
        VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sci.magFilter    = VK_FILTER_LINEAR;
        sci.minFilter    = VK_FILTER_LINEAR;
        sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(d, &sci, nullptr, &s_Data.Sampler);

        // Descriptor layout: binding 0 = array of MaxTextureSlots combined-image-samplers
        s_Data.SetLayout = VulkanDescriptorSetLayout::Builder()
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        VK_SHADER_STAGE_FRAGMENT_BIT,
                        static_cast<uint32_t>(MaxTextureSlots))
            .Build();

        s_Data.DescPool = VulkanDescriptorPool::Builder()
            .AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaxTextureSlots * 2)
            .SetMaxSets(4)
            .SetPoolFlags(VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT)
            .Build();
    }

    void BatchRenderer2D::CreatePipeline(VkRenderPass renderPass) {
        VkDevice d = VulkanDevice::Get().GetDevice();

        // Push-constant: mat4 projection
        VkPushConstantRange pcr{};
        pcr.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcr.offset     = 0;
        pcr.size       = sizeof(PushConstants);

        VkDescriptorSetLayout setLayout = s_Data.SetLayout->GetDescriptorSetLayout();
        VkPipelineLayoutCreateInfo plci{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        plci.setLayoutCount         = 1;
        plci.pSetLayouts            = &setLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges    = &pcr;
        vkCreatePipelineLayout(d, &plci, nullptr, &s_Data.PipelineLayout);

        PipelineConfigInfo cfg{};
        VulkanPipeline::DefaultPipelineConfigInfo(cfg);
        VulkanPipeline::EnableAlphaBlending(cfg);
        cfg.RenderPass                   = renderPass;
        cfg.PipelineLayout               = s_Data.PipelineLayout;
        cfg.DepthStencilInfo.depthTestEnable  = VK_FALSE;
        cfg.DepthStencilInfo.depthWriteEnable = VK_FALSE;

        s_Data.Pipeline.CreateGraphicsPipeline(
            "Assets/Shaders/Sprite2D.vert.spv",
            "Assets/Shaders/Sprite2D.frag.spv",
            cfg);
    }

    // =========================================================================
    // Per-frame helpers
    // =========================================================================

    void BatchRenderer2D::UpdateTextureDescriptors() {
        // Build image infos for all active slots
        std::vector<VkDescriptorImageInfo> imageInfos(MaxTextureSlots);
        for (int i = 0; i < MaxTextureSlots; i++) {
            auto& tex = s_Data.TextureSlots[i];
            if (tex) {
                imageInfos[i].sampler     = tex->GetSampler();
                imageInfos[i].imageView   = tex->GetImageView();
                imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            } else {
                // Fill unused slots with the white texture to keep the descriptor valid
                imageInfos[i].sampler     = s_Data.WhiteTexture->GetSampler();
                imageInfos[i].imageView   = s_Data.WhiteTexture->GetImageView();
                imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
        }

        VulkanDescriptorWriter(*s_Data.SetLayout, *s_Data.DescPool)
            .WriteImages(0, imageInfos)
            .Build(s_Data.DescSet);
    }

    // =========================================================================
    // Per-frame API
    // =========================================================================

    void BatchRenderer2D::BeginBatch(VkCommandBuffer cmd, const glm::mat4& projection) {
        s_Data.ActiveCmd        = cmd;
        s_Data.CurrentProjection = projection;
        s_Data.VertexPtr        = s_Data.VertexBase;
        s_Data.QuadCount        = 0;
        s_Data.TextureSlotIndex = 1; // slot 0 = white
    }

    void BatchRenderer2D::EndBatch() {
        if (s_Data.QuadCount > 0) Flush();
    }

    void BatchRenderer2D::Flush() {
        if (s_Data.QuadCount == 0) return;

        VkCommandBuffer cmd = s_Data.ActiveCmd;
        assert(cmd != VK_NULL_HANDLE && "BatchRenderer2D::Flush called outside BeginBatch/EndBatch");

        // Upload vertices to GPU (persistent map)
        uint32_t vertexBytes = static_cast<uint32_t>(
            reinterpret_cast<uint8_t*>(s_Data.VertexPtr) -
            reinterpret_cast<uint8_t*>(s_Data.VertexBase));
        std::memcpy(s_Data.VertexMapped, s_Data.VertexBase, vertexBytes);

        // Flush VMA mapping (only needed for non-coherent memory)
        vmaFlushAllocation(VulkanDevice::Get().GetAllocator(),
                           s_Data.VertexAlloc, 0, vertexBytes);

        // Update texture descriptor array
        UpdateTextureDescriptors();

        // Bind pipeline + descriptors
        s_Data.Pipeline.Bind(cmd);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                s_Data.PipelineLayout, 0, 1, &s_Data.DescSet, 0, nullptr);

        // Push projection matrix
        PushConstants pc{s_Data.CurrentProjection};
        vkCmdPushConstants(cmd, s_Data.PipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

        // Bind vertex + index buffers
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &s_Data.VertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, s_Data.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(cmd,
                         static_cast<uint32_t>(s_Data.QuadCount) * 6,
                         1, 0, 0, 0);

        s_Data.RenderStats.DrawCalls++;
        s_Data.QuadCount = 0;
        s_Data.VertexPtr = s_Data.VertexBase;
    }

    // =========================================================================
    // Draw commands
    // =========================================================================

    void BatchRenderer2D::DrawQuad(const glm::vec2& position,
                                    const glm::vec2& size,
                                    const glm::vec4& color) {
        if (s_Data.QuadCount >= MaxQuads) {
            Flush();
        }

        float x = position.x, y = position.y, w = size.x, h = size.y;
        auto* v = s_Data.VertexPtr;
        v[0] = {{x,     y    }, {0, 0}, color, 0.0f};
        v[1] = {{x + w, y    }, {1, 0}, color, 0.0f};
        v[2] = {{x + w, y + h}, {1, 1}, color, 0.0f};
        v[3] = {{x,     y + h}, {0, 1}, color, 0.0f};
        s_Data.VertexPtr += 4;
        s_Data.QuadCount++;
        s_Data.RenderStats.QuadCount++;
    }

    void BatchRenderer2D::DrawTexturedQuad(const glm::vec2& position,
                                            const glm::vec2& size,
                                            const Ref<Texture2D>& texture,
                                            const glm::vec4& tint,
                                            const glm::vec2& uvMin,
                                            const glm::vec2& uvMax) {
        if (s_Data.QuadCount >= MaxQuads) {
            Flush();
        }

        // Resolve texture slot index
        float texIndex = 0.0f;
        if (texture && texture != s_Data.WhiteTexture) {
            // Search existing slots (skip slot 0 = white)
            for (int i = 1; i < s_Data.TextureSlotIndex; i++) {
                if (s_Data.TextureSlots[i] == texture) {
                    texIndex = static_cast<float>(i);
                    break;
                }
            }
            if (texIndex == 0.0f) {
                if (s_Data.TextureSlotIndex >= MaxTextureSlots) {
                    Flush(); // make room
                }
                texIndex = static_cast<float>(s_Data.TextureSlotIndex);
                s_Data.TextureSlots[s_Data.TextureSlotIndex++] = texture;
            }
        }

        float x = position.x, y = position.y, w = size.x, h = size.y;
        auto* v = s_Data.VertexPtr;
        v[0] = {{x,     y    }, uvMin,                   tint, texIndex};
        v[1] = {{x + w, y    }, {uvMax.x, uvMin.y},      tint, texIndex};
        v[2] = {{x + w, y + h}, uvMax,                   tint, texIndex};
        v[3] = {{x,     y + h}, {uvMin.x, uvMax.y},      tint, texIndex};
        s_Data.VertexPtr += 4;
        s_Data.QuadCount++;
        s_Data.RenderStats.QuadCount++;
    }

    // =========================================================================
    // Stats
    // =========================================================================

    BatchRenderer2D::Stats BatchRenderer2D::GetStats() { return s_Data.RenderStats; }
    void BatchRenderer2D::ResetStats() { s_Data.RenderStats = {}; }

} // namespace GameEngine
