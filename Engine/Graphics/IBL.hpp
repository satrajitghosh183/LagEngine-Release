#pragma once

#include "../Core/Base.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include "Vulkan/VulkanPipeline.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace GameEngine {

    /**
     * @brief Image-Based Lighting utility (Vulkan)
     *
     * Generates the three IBL textures required for PBR rendering:
     *   - Irradiance cubemap  (diffuse IBL, 32x32 per face, VK_FORMAT_R16G16B16A16_SFLOAT)
     *   - Prefiltered cubemap (specular IBL with roughness mip levels, VK_FORMAT_R16G16B16A16_SFLOAT)
     *   - BRDF integration LUT (split-sum, 512x512, VK_FORMAT_R16G16_SFLOAT)
     *
     * All three textures are generated on the CPU (procedural sky, no HDR file required)
     * and uploaded to the GPU via staging buffers. LoadEnvironment() is available
     * as a future extension point for HDR equirectangular input.
     *
     * Usage:
     *   IBL::Init();
     *   // During lighting pass descriptor set building:
     *   auto irradianceInfo = IBL::GetIrradianceDescriptorInfo();
     *   auto prefilterInfo  = IBL::GetPrefilterDescriptorInfo();
     *   auto brdfInfo       = IBL::GetBRDFLUTDescriptorInfo();
     */
    class IBL {
    public:
        /** @brief Initialize IBL with a procedural sky (no HDR file needed). */
        static void Init();

        /** @brief Load an HDR equirectangular environment map (future). */
        static void LoadEnvironment(const std::string& hdrPath);

        /** @brief Release all GPU resources. */
        static void Shutdown();

        static bool IsReady() { return s_Initialized; }

        // ---- Raw Vulkan handles ----

        static VkImage     GetIrradianceImage()   { return s_IrradianceImage;   }
        static VkImageView GetIrradianceView()    { return s_IrradianceView;    }
        static VkImage     GetPrefilterImage()    { return s_PrefilterImage;    }
        static VkImageView GetPrefilterView()     { return s_PrefilterView;     }
        static VkImage     GetBRDFLUTImage()      { return s_BRDFLUTImage;      }
        static VkImageView GetBRDFLUTView()       { return s_BRDFLUTView;       }
        static VkSampler   GetCubemapSampler()    { return s_CubemapSampler;    }
        static VkSampler   GetLUTSampler()        { return s_LUTSampler;        }
        static uint32_t    GetPrefilterMipLevels(){ return s_PrefilterMipLevels; }

        // ---- Descriptor image infos (ready-to-use in descriptor writes) ----

        static VkDescriptorImageInfo GetIrradianceDescriptorInfo();
        static VkDescriptorImageInfo GetPrefilterDescriptorInfo();
        static VkDescriptorImageInfo GetBRDFLUTDescriptorInfo();

    private:
        static void GenerateBRDFLUT();
        static void GenerateProceduralCubemaps();

        // Shared GPU-side helpers
        static void CreateCubemapImage(uint32_t size, uint32_t mipLevels, VkFormat fmt,
                                        VkImage& outImage, VmaAllocation& outAlloc,
                                        VkImageView& outView);
        static void TransitionCubemapLayout(VkCommandBuffer cmd, VkImage image,
                                             VkImageLayout oldL, VkImageLayout newL,
                                             uint32_t mipLevels = 1);
        static void Create2DImage(uint32_t width, uint32_t height, VkFormat fmt,
                                   VkImage& outImage, VmaAllocation& outAlloc,
                                   VkImageView& outView);

        // Cubemap resources
        static VkImage       s_IrradianceImage;
        static VmaAllocation s_IrradianceAlloc;
        static VkImageView   s_IrradianceView;

        static VkImage       s_PrefilterImage;
        static VmaAllocation s_PrefilterAlloc;
        static VkImageView   s_PrefilterView;
        static uint32_t      s_PrefilterMipLevels;

        // BRDF LUT resources
        static VkImage       s_BRDFLUTImage;
        static VmaAllocation s_BRDFLUTAlloc;
        static VkImageView   s_BRDFLUTView;

        // Samplers
        static VkSampler s_CubemapSampler;  // linear, clamp-to-edge, mip-linear
        static VkSampler s_LUTSampler;      // linear, clamp-to-edge, no mip

        static bool s_Initialized;
    };

} // namespace GameEngine
