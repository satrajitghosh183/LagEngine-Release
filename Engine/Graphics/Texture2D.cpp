#include "Texture2D.hpp"
#include "../Core/Logger.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <cstring>
#include <cmath>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace GameEngine {

    Texture2D::Texture2D(const std::string& path)
        : m_Path(path) {

        int width, height, channels;
        stbi_set_flip_vertically_on_load(0); // Vulkan uses top-left origin
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            GE_CORE_ERROR("Failed to load texture: {0}", path);
            return;
        }

        m_Width = static_cast<uint32_t>(width);
        m_Height = static_cast<uint32_t>(height);

        if (channels == 4) m_Format = TextureFormat::RGBA;
        else if (channels == 3) m_Format = TextureFormat::RGB;
        else if (channels == 1) m_Format = TextureFormat::RED;
        else {
            GE_CORE_ERROR("Unsupported texture format: {0} channels", channels);
            stbi_image_free(data);
            return;
        }

        // Force RGBA for Vulkan compatibility (RGB not universally supported)
        unsigned char* rgbaData = data;
        bool needsConvert = (channels == 3);
        if (needsConvert) {
            rgbaData = new unsigned char[m_Width * m_Height * 4];
            for (uint32_t i = 0; i < m_Width * m_Height; i++) {
                rgbaData[i * 4 + 0] = data[i * 3 + 0];
                rgbaData[i * 4 + 1] = data[i * 3 + 1];
                rgbaData[i * 4 + 2] = data[i * 3 + 2];
                rgbaData[i * 4 + 3] = 255;
            }
            m_Format = TextureFormat::RGBA;
        }

        m_VkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        if (m_Format == TextureFormat::RED) {
            m_VkFormat = VK_FORMAT_R8_UNORM;
        }

        m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Width, m_Height)))) + 1;

        CreateImage(m_Width, m_Height, m_VkFormat, m_MipLevels,
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t uploadChannels = (m_Format == TextureFormat::RED) ? 1 : 4;
        UploadPixelData(rgbaData, m_Width, m_Height, uploadChannels);

        if (needsConvert) {
            delete[] rgbaData;
        }
        stbi_image_free(data);

        CreateImageView(m_VkFormat, m_MipLevels);
        CreateSampler(m_MipLevels);

        GenerateMipmaps();

        GE_CORE_INFO("Texture loaded: {0} ({1}x{2}, {3} mips)", path, m_Width, m_Height, m_MipLevels);
    }

    Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format)
        : m_Width(width), m_Height(height), m_Format(format) {

        m_VkFormat = TextureFormatToVk(format);
        m_MipLevels = 1;

        CreateImage(width, height, m_VkFormat, 1,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        CreateImageView(m_VkFormat, 1);
        CreateSampler(1);

        GE_CORE_INFO("Texture created: {0}x{1}", width, height);
    }

    Texture2D::~Texture2D() {
        auto device = VulkanDevice::Get().GetDevice();
        auto allocator = VulkanDevice::Get().GetAllocator();

        if (m_Sampler != VK_NULL_HANDLE)
            vkDestroySampler(device, m_Sampler, nullptr);
        if (m_ImageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, m_ImageView, nullptr);
        if (m_Image != VK_NULL_HANDLE)
            vmaDestroyImage(allocator, m_Image, m_Allocation);
    }

    VkDescriptorImageInfo Texture2D::GetDescriptorInfo() const {
        VkDescriptorImageInfo info{};
        info.sampler = m_Sampler;
        info.imageView = m_ImageView;
        info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        return info;
    }

    void Texture2D::SetData(const void* data, uint32_t size) {
        uint32_t channels = (m_Format == TextureFormat::RED) ? 1 : 4;
        uint32_t expectedSize = m_Width * m_Height * channels;
        GE_CORE_ASSERT(size == expectedSize, "Data must be entire texture!");

        UploadPixelData(data, m_Width, m_Height, channels);
    }

    void Texture2D::SetFilter(TextureFilter minFilter, TextureFilter magFilter) {
        // Recreate sampler with new filter settings
        auto device = VulkanDevice::Get().GetDevice();
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_Sampler, nullptr);
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = TextureFilterToVk(magFilter);
        samplerInfo.minFilter = TextureFilterToVk(minFilter);
        samplerInfo.mipmapMode = (minFilter == TextureFilter::Linear)
                                  ? VK_SAMPLER_MIPMAP_MODE_LINEAR
                                  : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);

        vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);
    }

    void Texture2D::SetWrap(TextureWrap wrapS, TextureWrap wrapT) {
        auto device = VulkanDevice::Get().GetDevice();
        if (m_Sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, m_Sampler, nullptr);
        }

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = TextureWrapToVk(wrapS);
        samplerInfo.addressModeV = TextureWrapToVk(wrapT);
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.maxLod = static_cast<float>(m_MipLevels);

        vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler);
    }

    void Texture2D::GenerateMipmaps() {
        if (m_MipLevels <= 1) return;

        auto& device = VulkanDevice::Get();

        VkCommandBuffer cmd = device.BeginSingleTimeCommands();

        int32_t mipWidth = static_cast<int32_t>(m_Width);
        int32_t mipHeight = static_cast<int32_t>(m_Height);

        for (uint32_t i = 1; i < m_MipLevels; i++) {
            // Transition previous mip to TRANSFER_SRC
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = m_Image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &barrier);

            // Blit from previous mip to current
            VkImageBlit blit{};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mipWidth > 1 ? mipWidth / 2 : 1,
                mipHeight > 1 ? mipHeight / 2 : 1,
                1
            };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(cmd,
                m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

            // Transition previous mip to SHADER_READ
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(cmd,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        // Transition last mip level to SHADER_READ
        VkImageMemoryBarrier lastBarrier{};
        lastBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        lastBarrier.image = m_Image;
        lastBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lastBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        lastBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        lastBarrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
        lastBarrier.subresourceRange.levelCount = 1;
        lastBarrier.subresourceRange.baseArrayLayer = 0;
        lastBarrier.subresourceRange.layerCount = 1;
        lastBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        lastBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        lastBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        lastBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr, 0, nullptr, 1, &lastBarrier);

        device.EndSingleTimeCommands(cmd);
    }

    void Texture2D::CreateImage(uint32_t width, uint32_t height, VkFormat format,
                                 uint32_t mipLevels, VkImageUsageFlags usage) {
        auto allocator = VulkanDevice::Get().GetAllocator();

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &m_Image, &m_Allocation, nullptr) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create Vulkan image ({0}x{1})", width, height);
        }
    }

    void Texture2D::CreateImageView(VkFormat format, uint32_t mipLevels) {
        auto device = VulkanDevice::Get().GetDevice();

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_Image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create texture image view");
        }
    }

    void Texture2D::CreateSampler(uint32_t mipLevels) {
        auto device = VulkanDevice::Get().GetDevice();

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16.0f;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create texture sampler");
        }
    }

    void Texture2D::TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
                                      uint32_t mipLevels) {
        auto& device = VulkanDevice::Get();
        VkCommandBuffer cmd = device.BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_Image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags srcStage, dstStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
            newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
                   newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

        device.EndSingleTimeCommands(cmd);
    }

    void Texture2D::UploadPixelData(const void* data, uint32_t width, uint32_t height,
                                     uint32_t channels) {
        auto& device = VulkanDevice::Get();
        auto allocator = device.GetAllocator();

        VkDeviceSize imageSize = width * height * channels;

        // Create staging buffer
        VkBuffer stagingBuffer;
        VmaAllocation stagingAlloc;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = imageSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                          VMA_ALLOCATION_CREATE_MAPPED_BIT;

        vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &stagingBuffer, &stagingAlloc, nullptr);

        void* mapped = nullptr;
        vmaMapMemory(allocator, stagingAlloc, &mapped);
        memcpy(mapped, data, imageSize);
        vmaUnmapMemory(allocator, stagingAlloc);
        vmaFlushAllocation(allocator, stagingAlloc, 0, imageSize);

        // Transition to TRANSFER_DST
        TransitionLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_MipLevels);

        // Copy buffer to image
        VkCommandBuffer cmd = device.BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(cmd, stagingBuffer, m_Image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        device.EndSingleTimeCommands(cmd);

        vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);

        // If no mipmaps, transition directly to SHADER_READ
        if (m_MipLevels <= 1) {
            TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        // Otherwise, GenerateMipmaps() handles the final transition
    }

    VkFormat Texture2D::TextureFormatToVk(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGB:  return VK_FORMAT_R8G8B8A8_UNORM; // Force RGBA
            case TextureFormat::RGBA: return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RED:  return VK_FORMAT_R8_UNORM;
            default: return VK_FORMAT_R8G8B8A8_UNORM;
        }
    }

    VkFilter Texture2D::TextureFilterToVk(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::Nearest: return VK_FILTER_NEAREST;
            case TextureFilter::Linear:  return VK_FILTER_LINEAR;
            default: return VK_FILTER_LINEAR;
        }
    }

    VkSamplerAddressMode Texture2D::TextureWrapToVk(TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat:         return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case TextureWrap::ClampToEdge:    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case TextureWrap::ClampToBorder:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case TextureWrap::MirroredRepeat: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            default: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }
}
