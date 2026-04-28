#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <string>
#include <cstdint>

namespace GameEngine {

    enum class TextureFilter {
        Nearest,
        Linear
    };

    enum class TextureWrap {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        MirroredRepeat
    };

    enum class TextureFormat {
        None = 0,
        RGB,
        RGBA,
        RED
    };

    /**
     * @brief Vulkan 2D Texture (VkImage + VkImageView + VkSampler)
     *
     * Features:
     * - Load from file (PNG, JPG, TGA, etc.) via stb_image
     * - Create from raw data
     * - Staging buffer upload with layout transitions
     * - Mipmap generation via vkCmdBlitImage
     * - Configurable filtering and wrapping
     * - Descriptor info for binding to shaders
     */
    class Texture2D {
    public:
        Texture2D(const std::string& path);
        Texture2D(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA);
        ~Texture2D();

        Texture2D(const Texture2D&) = delete;
        Texture2D& operator=(const Texture2D&) = delete;

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        uint32_t GetMipLevels() const { return m_MipLevels; }
        const std::string& GetPath() const { return m_Path; }

        VkImage GetImage() const { return m_Image; }
        VkImageView GetImageView() const { return m_ImageView; }
        VkSampler GetSampler() const { return m_Sampler; }

        /**
         * @brief Get descriptor image info for binding to descriptor sets
         */
        VkDescriptorImageInfo GetDescriptorInfo() const;

        /**
         * @brief Set texture data from CPU memory
         */
        void SetData(const void* data, uint32_t size);

        void SetFilter(TextureFilter minFilter, TextureFilter magFilter);
        void SetWrap(TextureWrap wrapS, TextureWrap wrapT);

        /**
         * @brief Generate mipmaps (requires image to be in TRANSFER_DST layout)
         */
        void GenerateMipmaps();

        bool operator==(const Texture2D& other) const {
            return m_Image == other.m_Image;
        }

    private:
        void CreateImage(uint32_t width, uint32_t height, VkFormat format,
                         uint32_t mipLevels, VkImageUsageFlags usage);
        void CreateImageView(VkFormat format, uint32_t mipLevels);
        void CreateSampler(uint32_t mipLevels);
        void TransitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
                               uint32_t mipLevels = 1);
        void UploadPixelData(const void* data, uint32_t width, uint32_t height,
                              uint32_t channels);

        static VkFormat TextureFormatToVk(TextureFormat format);
        static VkFilter TextureFilterToVk(TextureFilter filter);
        static VkSamplerAddressMode TextureWrapToVk(TextureWrap wrap);

    private:
        VkImage m_Image = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        VkImageView m_ImageView = VK_NULL_HANDLE;
        VkSampler m_Sampler = VK_NULL_HANDLE;

        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_MipLevels = 1;
        VkFormat m_VkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        TextureFormat m_Format = TextureFormat::None;
        std::string m_Path;
    };
}
