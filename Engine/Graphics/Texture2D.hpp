#pragma once

#include "../Core/Base.hpp"
#include <string>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Texture filtering modes
     */
    enum class TextureFilter {
        Nearest,
        Linear
    };

    /**
     * @brief Texture wrapping modes
     */
    enum class TextureWrap {
        Repeat,
        ClampToEdge,
        ClampToBorder,
        MirroredRepeat
    };

    /**
     * @brief Texture format
     */
    enum class TextureFormat {
        None = 0,
        RGB,
        RGBA,
        RED
    };

    /**
     * @brief 2D Texture class
     * 
     * Features:
     * - Load from file (PNG, JPG, TGA, etc.)
     * - Create from raw data
     * - Configurable filtering and wrapping
     * - Mipmap generation
     * - Bind to texture units
     * 
     * Usage:
     *   auto texture = CreateRef<Texture2D>("assets/textures/wall.png");
     *   texture->Bind(0); // Bind to texture unit 0
     */
    class Texture2D {
    public:
        /**
         * @brief Load texture from file
         */
        Texture2D(const std::string& path);
        
        /**
         * @brief Create texture from raw data
         */
        Texture2D(uint32_t width, uint32_t height, TextureFormat format = TextureFormat::RGBA);
        
        ~Texture2D();
        
        /**
         * @brief Get texture width
         */
        uint32_t GetWidth() const { return m_Width; }
        
        /**
         * @brief Get texture height
         */
        uint32_t GetHeight() const { return m_Height; }
        
        /**
         * @brief Get OpenGL texture ID
         */
        uint32_t GetRendererID() const { return m_RendererID; }
        
        /**
         * @brief Get texture path (empty if created from data)
         */
        const std::string& GetPath() const { return m_Path; }
        
        /**
         * @brief Bind texture to specified texture unit (0-31)
         */
        void Bind(uint32_t slot = 0) const;
        
        /**
         * @brief Unbind texture
         */
        void Unbind() const;
        
        /**
         * @brief Set texture data
         */
        void SetData(void* data, uint32_t size);
        
        /**
         * @brief Set filtering mode
         */
        void SetFilter(TextureFilter minFilter, TextureFilter magFilter);
        
        /**
         * @brief Set wrapping mode
         */
        void SetWrap(TextureWrap wrapS, TextureWrap wrapT);
        
        /**
         * @brief Generate mipmaps
         */
        void GenerateMipmaps();
        
        /**
         * @brief Check if textures are equal
         */
        bool operator==(const Texture2D& other) const {
            return m_RendererID == other.m_RendererID;
        }
        
    private:
        uint32_t m_RendererID;
        uint32_t m_Width;
        uint32_t m_Height;
        TextureFormat m_Format;
        std::string m_Path;
        
        uint32_t GetOpenGLFormat(TextureFormat format);
        uint32_t GetOpenGLFilter(TextureFilter filter);
        uint32_t GetOpenGLWrap(TextureWrap wrap);
    };
}