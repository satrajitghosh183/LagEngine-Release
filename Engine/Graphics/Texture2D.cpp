#include "Texture2D.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>

// stb_image for loading images
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace GameEngine {

    Texture2D::Texture2D(const std::string& path)
        : m_RendererID(0), m_Width(0), m_Height(0), m_Format(TextureFormat::None), m_Path(path) {
        
        // Load image using stb_image
        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        
        if (!data) {
            GE_CORE_ERROR("Failed to load texture: {0}", path);
            return;
        }
        
        m_Width = width;
        m_Height = height;
        
        // Determine format
        if (channels == 4) {
            m_Format = TextureFormat::RGBA;
        } else if (channels == 3) {
            m_Format = TextureFormat::RGB;
        } else if (channels == 1) {
            m_Format = TextureFormat::RED;
        } else {
            GE_CORE_ERROR("Unsupported texture format: {0} channels", channels);
            stbi_image_free(data);
            return;
        }
        
        // Create OpenGL texture
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        // Upload texture data
        uint32_t internalFormat = GetOpenGLFormat(m_Format);
        uint32_t dataFormat = internalFormat;
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        
        // Free image data
        stbi_image_free(data);
        
        GE_CORE_INFO("Texture loaded: {0} ({1}x{2})", path, m_Width, m_Height);
    }

    Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format)
        : m_Width(width), m_Height(height), m_Format(format) {
        
        // Create empty texture
        glGenTextures(1, &m_RendererID);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        
        // Allocate texture storage
        uint32_t internalFormat = GetOpenGLFormat(m_Format);
        uint32_t dataFormat = internalFormat;
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, dataFormat, GL_UNSIGNED_BYTE, nullptr);
        
        GE_CORE_INFO("Texture created: {0}x{1}", m_Width, m_Height);
    }

    Texture2D::~Texture2D() {
        if (m_RendererID != 0) {
            glDeleteTextures(1, &m_RendererID);
            m_RendererID = 0;
        }
    }

    void Texture2D::Bind(uint32_t slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
    }

    void Texture2D::Unbind() const {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture2D::SetData(void* data, uint32_t size) {
        uint32_t bpp = m_Format == TextureFormat::RGBA ? 4 : 3;
        GE_CORE_ASSERT(size == m_Width * m_Height * bpp, "Data must be entire texture!");
        
        uint32_t dataFormat = GetOpenGLFormat(m_Format);
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, dataFormat, GL_UNSIGNED_BYTE, data);
    }

    void Texture2D::SetFilter(TextureFilter minFilter, TextureFilter magFilter) {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GetOpenGLFilter(minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GetOpenGLFilter(magFilter));
    }

    void Texture2D::SetWrap(TextureWrap wrapS, TextureWrap wrapT) {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GetOpenGLWrap(wrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GetOpenGLWrap(wrapT));
    }

    void Texture2D::GenerateMipmaps() {
        glBindTexture(GL_TEXTURE_2D, m_RendererID);
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    uint32_t Texture2D::GetOpenGLFormat(TextureFormat format) {
        switch (format) {
            case TextureFormat::RGB:  return GL_RGB;
            case TextureFormat::RGBA: return GL_RGBA;
            case TextureFormat::RED:  return GL_RED;
            default: return 0;
        }
    }

    uint32_t Texture2D::GetOpenGLFilter(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::Nearest: return GL_NEAREST;
            case TextureFilter::Linear:  return GL_LINEAR;
            default: return GL_LINEAR;
        }
    }

    uint32_t Texture2D::GetOpenGLWrap(TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat:         return GL_REPEAT;
            case TextureWrap::ClampToEdge:    return GL_CLAMP_TO_EDGE;
            case TextureWrap::ClampToBorder:  return GL_CLAMP_TO_BORDER;
            case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            default: return GL_REPEAT;
        }
    }
}