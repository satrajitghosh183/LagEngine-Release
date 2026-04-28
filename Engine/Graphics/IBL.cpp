#include "IBL.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include "../Core/Logger.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#include <cmath>
#include <vector>
#include <array>
#include <random>
#include <algorithm>

namespace GameEngine {

    // -------------------------------------------------------------------------
    // Static definitions
    // -------------------------------------------------------------------------

    VkImage       IBL::s_IrradianceImage  = VK_NULL_HANDLE;
    VmaAllocation IBL::s_IrradianceAlloc  = VK_NULL_HANDLE;
    VkImageView   IBL::s_IrradianceView   = VK_NULL_HANDLE;

    VkImage       IBL::s_PrefilterImage   = VK_NULL_HANDLE;
    VmaAllocation IBL::s_PrefilterAlloc   = VK_NULL_HANDLE;
    VkImageView   IBL::s_PrefilterView    = VK_NULL_HANDLE;
    uint32_t      IBL::s_PrefilterMipLevels = 5;

    VkImage       IBL::s_BRDFLUTImage     = VK_NULL_HANDLE;
    VmaAllocation IBL::s_BRDFLUTAlloc     = VK_NULL_HANDLE;
    VkImageView   IBL::s_BRDFLUTView      = VK_NULL_HANDLE;

    VkSampler     IBL::s_CubemapSampler   = VK_NULL_HANDLE;
    VkSampler     IBL::s_LUTSampler       = VK_NULL_HANDLE;

    bool IBL::s_Initialized = false;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void IBL::Init() {
        if (s_Initialized) return;

        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();

        // ---- Samplers ----
        {
            VkSamplerCreateInfo sci{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
            sci.magFilter        = VK_FILTER_LINEAR;
            sci.minFilter        = VK_FILTER_LINEAR;
            sci.mipmapMode       = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sci.addressModeU     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeV     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.addressModeW     = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            sci.maxLod           = static_cast<float>(s_PrefilterMipLevels);
            sci.anisotropyEnable = VK_FALSE;
            vkCreateSampler(d, &sci, nullptr, &s_CubemapSampler);

            sci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            sci.maxLod     = 0.0f;
            vkCreateSampler(d, &sci, nullptr, &s_LUTSampler);
        }

        GenerateProceduralCubemaps();
        GenerateBRDFLUT();

        s_Initialized = true;
        GE_CORE_INFO("IBL initialized (procedural sky, BRDF LUT 512x512, prefilter {}x{} {} mips)",
                     128, 128, s_PrefilterMipLevels);
    }

    void IBL::Shutdown() {
        if (!s_Initialized) return;

        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        vkDeviceWaitIdle(d);

        if (s_IrradianceView)  vkDestroyImageView(d, s_IrradianceView,  nullptr);
        if (s_IrradianceImage) vmaDestroyImage(a, s_IrradianceImage, s_IrradianceAlloc);

        if (s_PrefilterView)   vkDestroyImageView(d, s_PrefilterView,   nullptr);
        if (s_PrefilterImage)  vmaDestroyImage(a, s_PrefilterImage,  s_PrefilterAlloc);

        if (s_BRDFLUTView)     vkDestroyImageView(d, s_BRDFLUTView,     nullptr);
        if (s_BRDFLUTImage)    vmaDestroyImage(a, s_BRDFLUTImage,    s_BRDFLUTAlloc);

        if (s_CubemapSampler)  vkDestroySampler(d, s_CubemapSampler, nullptr);
        if (s_LUTSampler)      vkDestroySampler(d, s_LUTSampler,     nullptr);

        s_IrradianceImage = VK_NULL_HANDLE;  s_IrradianceView  = VK_NULL_HANDLE;
        s_PrefilterImage  = VK_NULL_HANDLE;  s_PrefilterView   = VK_NULL_HANDLE;
        s_BRDFLUTImage    = VK_NULL_HANDLE;  s_BRDFLUTView     = VK_NULL_HANDLE;
        s_CubemapSampler  = VK_NULL_HANDLE;  s_LUTSampler      = VK_NULL_HANDLE;
        s_Initialized     = false;
    }

    // -------------------------------------------------------------------------
    // GPU image helpers
    // -------------------------------------------------------------------------

    void IBL::CreateCubemapImage(uint32_t size, uint32_t mipLevels, VkFormat fmt,
                                  VkImage& outImage, VmaAllocation& outAlloc,
                                  VkImageView& outView) {
        auto& dev = VulkanDevice::Get();
        VkDevice d = dev.GetDevice();
        VmaAllocator a = dev.GetAllocator();

        VkImageCreateInfo ici{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        ici.imageType   = VK_IMAGE_TYPE_2D;
        ici.format      = fmt;
        ici.extent      = {size, size, 1};
        ici.mipLevels   = mipLevels;
        ici.arrayLayers = 6;
        ici.samples     = VK_SAMPLE_COUNT_1_BIT;
        ici.tiling      = VK_IMAGE_TILING_OPTIMAL;
        ici.usage       = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ici.flags       = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        VmaAllocationCreateInfo vaci{};
        vaci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        vmaCreateImage(a, &ici, &vaci, &outImage, &outAlloc, nullptr);

        VkImageViewCreateInfo ivci{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ivci.image    = outImage;
        ivci.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        ivci.format   = fmt;
        ivci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.baseMipLevel   = 0;
        ivci.subresourceRange.levelCount     = mipLevels;
        ivci.subresourceRange.baseArrayLayer = 0;
        ivci.subresourceRange.layerCount     = 6;
        vkCreateImageView(d, &ivci, nullptr, &outView);
    }

    void IBL::Create2DImage(uint32_t width, uint32_t height, VkFormat fmt,
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
        ici.usage       = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

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

    void IBL::TransitionCubemapLayout(VkCommandBuffer cmd, VkImage image,
                                       VkImageLayout oldL, VkImageLayout newL,
                                       uint32_t mipLevels) {
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout           = oldL;
        barrier.newLayout           = newL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 6;

        VkPipelineStageFlags src = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dst = VK_PIPELINE_STAGE_TRANSFER_BIT;

        if (oldL == VK_IMAGE_LAYOUT_UNDEFINED && newL == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
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
    // Upload helper: create staging buffer, copy, destroy
    // -------------------------------------------------------------------------

    static VkBuffer UploadCubemapLayer(VmaAllocator allocator,
                                        const void* data, VkDeviceSize size,
                                        VmaAllocation& outAlloc) {
        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = size;
        bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo aci{};
        aci.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        aci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo ai{};
        VkBuffer buf;
        vmaCreateBuffer(allocator, &bci, &aci, &buf, &outAlloc, &ai);
        std::memcpy(ai.pMappedData, data, size);
        return buf;
    }

    // -------------------------------------------------------------------------
    // Procedural sky cubemaps
    // -------------------------------------------------------------------------

    void IBL::GenerateProceduralCubemaps() {
        auto& dev = VulkanDevice::Get();
        VmaAllocator a = dev.GetAllocator();

        // ---- Irradiance (32x32, simple directional average) ----
        const uint32_t irrSize = 32;
        CreateCubemapImage(irrSize, 1, VK_FORMAT_R16G16B16A16_SFLOAT,
                            s_IrradianceImage, s_IrradianceAlloc, s_IrradianceView);

        glm::vec3 skyAvg(0.35f, 0.50f, 0.75f);
        glm::vec3 groundAvg(0.10f, 0.09f, 0.08f);
        glm::vec3 faceColors[6] = {
            skyAvg * 0.6f, skyAvg * 0.6f, skyAvg, groundAvg, skyAvg * 0.6f, skyAvg * 0.6f
        };

        // ---- Prefilter (128x128 x 5 mips) ----
        const uint32_t preSize = 128;
        CreateCubemapImage(preSize, s_PrefilterMipLevels, VK_FORMAT_R16G16B16A16_SFLOAT,
                            s_PrefilterImage, s_PrefilterAlloc, s_PrefilterView);

        // ---- Upload all faces via single-time commands ----
        VkCommandBuffer cmd = dev.BeginSingleTimeCommands();

        // Transition irradiance + prefilter to TRANSFER_DST
        TransitionCubemapLayout(cmd, s_IrradianceImage,
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        TransitionCubemapLayout(cmd, s_PrefilterImage,
                                 VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 s_PrefilterMipLevels);

        // ---- Irradiance faces ----
        std::vector<std::pair<VkBuffer, VmaAllocation>> stagingBufs;

        for (uint32_t face = 0; face < 6; face++) {
            std::vector<uint16_t> data(irrSize * irrSize * 4);
            glm::vec3 col = faceColors[face];
            // Pack as f16 (approximate via bit cast from f32 truncation for simplicity)
            // Use full uint16 range to represent [0,1]: we store as half-float.
            // Simple approach: store as raw RGBA16F — encode via a lambda.
            auto f32ToF16 = [](float v) -> uint16_t {
                // IEEE 754 half-float conversion
                uint32_t f;
                std::memcpy(&f, &v, 4);
                uint16_t h = static_cast<uint16_t>(
                    ((f >> 16) & 0x8000) |
                    ((((f & 0x7F800000) - 0x38000000) >> 13) & 0x7C00) |
                    ((f >> 13) & 0x03FF));
                return h;
            };
            for (uint32_t i = 0; i < irrSize * irrSize; i++) {
                data[i * 4 + 0] = f32ToF16(col.r);
                data[i * 4 + 1] = f32ToF16(col.g);
                data[i * 4 + 2] = f32ToF16(col.b);
                data[i * 4 + 3] = f32ToF16(1.0f);
            }

            VmaAllocation stageAlloc;
            VkBuffer stageBuf = UploadCubemapLayer(a, data.data(),
                                                    data.size() * sizeof(uint16_t), stageAlloc);
            stagingBufs.push_back({stageBuf, stageAlloc});

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel       = 0;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount     = 1;
            region.imageExtent                     = {irrSize, irrSize, 1};
            vkCmdCopyBufferToImage(cmd, stageBuf, s_IrradianceImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        // ---- Prefilter mip faces ----
        for (uint32_t mip = 0; mip < s_PrefilterMipLevels; mip++) {
            uint32_t mipSize = std::max(1u, preSize >> mip);
            float roughness  = static_cast<float>(mip) / static_cast<float>(s_PrefilterMipLevels - 1);
            glm::vec3 topCol    = glm::mix(glm::vec3(0.20f, 0.40f, 0.80f), skyAvg, roughness);
            glm::vec3 horizCol  = glm::mix(glm::vec3(0.60f, 0.70f, 0.90f), skyAvg, roughness);

            auto f32ToF16 = [](float v) -> uint16_t {
                uint32_t f;
                std::memcpy(&f, &v, 4);
                uint16_t h = static_cast<uint16_t>(
                    ((f >> 16) & 0x8000) |
                    ((((f & 0x7F800000) - 0x38000000) >> 13) & 0x7C00) |
                    ((f >> 13) & 0x03FF));
                return h;
            };

            for (uint32_t face = 0; face < 6; face++) {
                std::vector<uint16_t> data(mipSize * mipSize * 4);
                for (uint32_t y = 0; y < mipSize; y++) {
                    for (uint32_t x = 0; x < mipSize; x++) {
                        float t = (face == 2) ? 1.0f : (face == 3) ? -0.5f :
                                  (static_cast<float>(y) / static_cast<float>(mipSize) - 0.5f) * -2.0f;

                        glm::vec3 col = (t > 0.0f)
                            ? glm::mix(horizCol, topCol, t * 0.5f)
                            : glm::mix(horizCol, skyAvg * 0.3f, -t);

                        uint32_t idx = (y * mipSize + x) * 4;
                        data[idx + 0] = f32ToF16(col.r);
                        data[idx + 1] = f32ToF16(col.g);
                        data[idx + 2] = f32ToF16(col.b);
                        data[idx + 3] = f32ToF16(1.0f);
                    }
                }

                VmaAllocation stageAlloc;
                VkBuffer stageBuf = UploadCubemapLayer(a, data.data(),
                                                        data.size() * sizeof(uint16_t), stageAlloc);
                stagingBufs.push_back({stageBuf, stageAlloc});

                VkBufferImageCopy region{};
                region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                region.imageSubresource.mipLevel       = mip;
                region.imageSubresource.baseArrayLayer = face;
                region.imageSubresource.layerCount     = 1;
                region.imageExtent                     = {mipSize, mipSize, 1};
                vkCmdCopyBufferToImage(cmd, stageBuf, s_PrefilterImage,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
            }
        }

        // Transition to shader read
        TransitionCubemapLayout(cmd, s_IrradianceImage,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        TransitionCubemapLayout(cmd, s_PrefilterImage,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 s_PrefilterMipLevels);

        dev.EndSingleTimeCommands(cmd);

        // Free staging buffers
        for (auto& [buf, alloc] : stagingBufs) {
            vmaDestroyBuffer(a, buf, alloc);
        }

        GE_CORE_DEBUG("IBL: Irradiance ({}x{}) and prefilter ({}x{}, {} mips) generated",
                      irrSize, irrSize, preSize, preSize, s_PrefilterMipLevels);
    }

    // -------------------------------------------------------------------------
    // BRDF LUT — CPU-computed split-sum approximation, uploaded as R16G16F
    // -------------------------------------------------------------------------

    static float RadicalInverse_VdC(uint32_t bits) {
        bits = (bits << 16u) | (bits >> 16u);
        bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
        bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
        bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
        bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
        return static_cast<float>(bits) * 2.3283064365386963e-10f;
    }

    static glm::vec2 Hammersley(uint32_t i, uint32_t N) {
        return {static_cast<float>(i) / static_cast<float>(N), RadicalInverse_VdC(i)};
    }

    static glm::vec3 ImportanceSampleGGX(glm::vec2 Xi, glm::vec3 N, float roughness) {
        const float PI = 3.14159265359f;
        float a = roughness * roughness;
        float phi = 2.0f * PI * Xi.x;
        float cosTheta = std::sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
        float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
        glm::vec3 H(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta);
        glm::vec3 up = std::abs(N.z) < 0.999f ? glm::vec3(0, 0, 1) : glm::vec3(1, 0, 0);
        glm::vec3 tangent   = glm::normalize(glm::cross(up, N));
        glm::vec3 bitangent = glm::cross(N, tangent);
        return glm::normalize(tangent * H.x + bitangent * H.y + N * H.z);
    }

    static float GeometrySchlickGGX(float NdotV, float roughness) {
        float k = (roughness * roughness) / 2.0f;
        return NdotV / (NdotV * (1.0f - k) + k);
    }

    static float GeometrySmith(float NdotV, float NdotL, float roughness) {
        return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
    }

    static glm::vec2 IntegrateBRDF(float NdotV, float roughness) {
        glm::vec3 V(std::sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);
        float A = 0.0f, B = 0.0f;
        constexpr uint32_t SAMPLES = 1024;
        glm::vec3 N(0.0f, 0.0f, 1.0f);
        for (uint32_t i = 0; i < SAMPLES; i++) {
            glm::vec2 Xi = Hammersley(i, SAMPLES);
            glm::vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
            glm::vec3 L  = glm::normalize(2.0f * glm::dot(V, H) * H - V);
            float NdotL  = std::max(L.z, 0.0f);
            float NdotH  = std::max(H.z, 0.0f);
            float VdotH  = std::max(glm::dot(V, H), 0.0f);
            if (NdotL > 0.0f) {
                float G    = GeometrySmith(NdotV, NdotL, roughness);
                float GVis = (G * VdotH) / (NdotH * NdotV + 1e-7f);
                float Fc   = std::pow(1.0f - VdotH, 5.0f);
                A += (1.0f - Fc) * GVis;
                B += Fc * GVis;
            }
        }
        return {A / static_cast<float>(SAMPLES), B / static_cast<float>(SAMPLES)};
    }

    void IBL::GenerateBRDFLUT() {
        const uint32_t LUT_SIZE = 512;
        Create2DImage(LUT_SIZE, LUT_SIZE, VK_FORMAT_R16G16_SFLOAT,
                      s_BRDFLUTImage, s_BRDFLUTAlloc, s_BRDFLUTView);

        // CPU-compute the LUT (1024 samples x 512x512 ≈ 1-2 seconds at init)
        // For a production engine this would be a GPU compute shader; here we
        // keep it on the CPU to avoid depending on a compiled SPIR-V file.
        std::vector<uint16_t> pixels(LUT_SIZE * LUT_SIZE * 2); // RG16F

        auto f32ToF16 = [](float v) -> uint16_t {
            v = std::max(0.0f, std::min(v, 1.0f));
            uint32_t f;
            std::memcpy(&f, &v, 4);
            uint16_t h = static_cast<uint16_t>(
                ((f >> 16) & 0x8000) |
                ((((f & 0x7F800000) - 0x38000000) >> 13) & 0x7C00) |
                ((f >> 13) & 0x03FF));
            return h;
        };

        for (uint32_t y = 0; y < LUT_SIZE; y++) {
            float roughness = (static_cast<float>(y) + 0.5f) / static_cast<float>(LUT_SIZE);
            for (uint32_t x = 0; x < LUT_SIZE; x++) {
                float NdotV = (static_cast<float>(x) + 0.5f) / static_cast<float>(LUT_SIZE);
                glm::vec2 brdf = IntegrateBRDF(NdotV, roughness);
                uint32_t idx = (y * LUT_SIZE + x) * 2;
                pixels[idx + 0] = f32ToF16(brdf.x);
                pixels[idx + 1] = f32ToF16(brdf.y);
            }
        }

        // Upload via staging buffer
        auto& dev = VulkanDevice::Get();
        VmaAllocator a = dev.GetAllocator();
        VkDeviceSize dataSize = pixels.size() * sizeof(uint16_t);

        VkBuffer stageBuf;
        VmaAllocation stageAlloc;
        VmaAllocationInfo stageInfo{};
        VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bci.size  = dataSize;
        bci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VmaAllocationCreateInfo saci{};
        saci.usage = VMA_MEMORY_USAGE_CPU_ONLY;
        saci.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        vmaCreateBuffer(a, &bci, &saci, &stageBuf, &stageAlloc, &stageInfo);
        std::memcpy(stageInfo.pMappedData, pixels.data(), dataSize);

        VkCommandBuffer cmd = dev.BeginSingleTimeCommands();

        // Transition UNDEFINED -> TRANSFER_DST
        VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        barrier.oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image               = s_BRDFLUTImage;
        barrier.subresourceRange    = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        barrier.srcAccessMask       = 0;
        barrier.dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkBufferImageCopy region{};
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        region.imageExtent      = {LUT_SIZE, LUT_SIZE, 1};
        vkCmdCopyBufferToImage(cmd, stageBuf, s_BRDFLUTImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        dev.EndSingleTimeCommands(cmd);
        vmaDestroyBuffer(a, stageBuf, stageAlloc);

        GE_CORE_DEBUG("IBL: BRDF LUT generated ({}x{})", LUT_SIZE, LUT_SIZE);
    }

    // -------------------------------------------------------------------------
    // Descriptor infos
    // -------------------------------------------------------------------------

    VkDescriptorImageInfo IBL::GetIrradianceDescriptorInfo() {
        VkDescriptorImageInfo info{};
        info.sampler     = s_CubemapSampler;
        info.imageView   = s_IrradianceView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    VkDescriptorImageInfo IBL::GetPrefilterDescriptorInfo() {
        VkDescriptorImageInfo info{};
        info.sampler     = s_CubemapSampler;
        info.imageView   = s_PrefilterView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    VkDescriptorImageInfo IBL::GetBRDFLUTDescriptorInfo() {
        VkDescriptorImageInfo info{};
        info.sampler     = s_LUTSampler;
        info.imageView   = s_BRDFLUTView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    void IBL::LoadEnvironment(const std::string& hdrPath) {
        GE_CORE_WARN("IBL::LoadEnvironment('{}') not yet implemented; using procedural sky", hdrPath);
    }

} // namespace GameEngine
