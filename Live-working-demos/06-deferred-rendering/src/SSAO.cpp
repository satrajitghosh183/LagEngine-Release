#include "SSAO.h"
#include "GBuffer.h"
#include "VulkanBase.hpp"
#include <cuda_runtime.h>
#include <random>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// CUDA-Vulkan interop headers
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

SSAO::~SSAO() {
    cleanup();
}

void SSAO::init(vkdemo::VulkanBase* vkBase, int width, int height) {
    m_vkBase  = vkBase;
    m_width   = width;
    m_height  = height;

    generateKernel();
    generateNoise();

    VkDevice dev = m_vkBase->GetDevice();
    uint32_t w = static_cast<uint32_t>(m_width);
    uint32_t h = static_cast<uint32_t>(m_height);

    // Result image (R32_SFLOAT)
    createImage(w, h, VK_FORMAT_R32_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                m_resultImage, m_resultMemory, false);
    m_resultView = createImageView(m_resultImage, VK_FORMAT_R32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);

    // Transition result image to shader read optimal and fill with 1.0 (white = no occlusion)
    VkCommandBuffer cmd = m_vkBase->BeginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_resultImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_vkBase->EndSingleTimeCommands(cmd);

    // Sampler
    VkSamplerCreateInfo sci{};
    sci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sci.magFilter = VK_FILTER_LINEAR;
    sci.minFilter = VK_FILTER_LINEAR;
    sci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sci.maxLod = 1.0f;
    vkCreateSampler(dev, &sci, nullptr, &m_sampler);

    // Noise texture (4x4 RGB16F)
    createImage(4, 4, VK_FORMAT_R16G16B16A16_SFLOAT,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                m_noiseImage, m_noiseMemory, false);
    m_noiseView = createImageView(m_noiseImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void SSAO::cleanup() {
    if (!m_vkBase) return;
    VkDevice dev = m_vkBase->GetDevice();
    if (dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(dev);

    if (m_cudaExtMemory) {
        cudaDestroyExternalMemory(m_cudaExtMemory);
        m_cudaExtMemory = nullptr;
    }

    auto destroyImg = [&](VkImage& img, VkDeviceMemory& mem, VkImageView& view) {
        if (view) { vkDestroyImageView(dev, view, nullptr); view = VK_NULL_HANDLE; }
        if (img)  { vkDestroyImage(dev, img, nullptr); img = VK_NULL_HANDLE; }
        if (mem)  { vkFreeMemory(dev, mem, nullptr); mem = VK_NULL_HANDLE; }
    };

    destroyImg(m_resultImage, m_resultMemory, m_resultView);
    destroyImg(m_noiseImage,  m_noiseMemory,  m_noiseView);

    if (m_sampler) { vkDestroySampler(dev, m_sampler, nullptr); m_sampler = VK_NULL_HANDLE; }
}

void SSAO::generateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    m_kernel.resize(64);
    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = (float)i / 64.0f;
        scale = 0.1f + scale * scale * 0.9f;
        sample *= scale;
        m_kernel[i] = sample;
    }
}

void SSAO::generateNoise() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    m_noise.resize(16);
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        m_noise[i] = noise;
    }
}

void SSAO::process(GBuffer* gbuffer, const glm::mat4& projectionMatrix) {
    // Placeholder -- actual SSAO computation via CUDA would happen here
    // using Vulkan-CUDA interop on the result image
}

void SSAO::resize(int width, int height) {
    if (m_width == width && m_height == height) return;
    cleanup();
    m_width  = width;
    m_height = height;
    if (m_vkBase) {
        init(m_vkBase, width, height);
    }
}

uint32_t SSAO::findMemoryType(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(m_vkBase->GetPhysicalDevice(), &memProps);
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((filter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
            return i;
    }
    throw std::runtime_error("SSAO: failed to find suitable memory type");
}

void SSAO::createImage(uint32_t w, uint32_t h, VkFormat fmt, VkImageUsageFlags usage,
                        VkImage& image, VkDeviceMemory& memory, bool exportable) {
    VkImageCreateInfo ici{};
    ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.extent = {w, h, 1};
    ici.mipLevels = 1;
    ici.arrayLayers = 1;
    ici.format = fmt;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.usage = usage;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice dev = m_vkBase->GetDevice();
    vkCreateImage(dev, &ici, nullptr, &image);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(dev, image, &memReq);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = memReq.size;
    mai.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // If exportable, add external memory flags for CUDA interop
    VkExportMemoryAllocateInfo exportInfo{};
    if (exportable) {
#ifdef _WIN32
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
        exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
        exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
        mai.pNext = &exportInfo;
    }

    vkAllocateMemory(dev, &mai, nullptr, &memory);
    vkBindImageMemory(dev, image, memory, 0);
}

VkImageView SSAO::createImageView(VkImage image, VkFormat fmt, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    ci.image = image;
    ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
    ci.format = fmt;
    ci.subresourceRange.aspectMask = aspect;
    ci.subresourceRange.baseMipLevel = 0;
    ci.subresourceRange.levelCount = 1;
    ci.subresourceRange.baseArrayLayer = 0;
    ci.subresourceRange.layerCount = 1;

    VkImageView view;
    vkCreateImageView(m_vkBase->GetDevice(), &ci, nullptr, &view);
    return view;
}
