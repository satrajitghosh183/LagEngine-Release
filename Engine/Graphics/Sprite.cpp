#include "Sprite.hpp"

#ifdef GE_PLATFORM_WINDOWS
    #include <glad/glad.h>
#else
    #include <glad/glad.h>
#endif

// stb_image for loading from file
#include <stb_image.h>

namespace GameEngine {

    Sprite::Sprite() = default;

    Sprite Sprite::createFromFile(const std::string& filePath) {
        Sprite sprite;

        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 4);
        if (!data) return sprite;

        uint32_t texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        sprite.m_TextureID = texID;
        sprite.m_TextureWidth = width;
        sprite.m_TextureHeight = height;
        sprite.m_Size = glm::vec2(static_cast<float>(width), static_cast<float>(height));
        return sprite;
    }

    Sprite Sprite::createFromTexture(uint32_t textureID, int textureWidth, int textureHeight) {
        Sprite sprite;
        sprite.m_TextureID = textureID;
        sprite.m_TextureWidth = textureWidth;
        sprite.m_TextureHeight = textureHeight;
        sprite.m_Size = glm::vec2(static_cast<float>(textureWidth), static_cast<float>(textureHeight));
        return sprite;
    }

    Sprite Sprite::createFromRegion(uint32_t textureID, int textureWidth, int textureHeight,
                                     int regionX, int regionY, int regionW, int regionH) {
        Sprite sprite;
        sprite.m_TextureID = textureID;
        sprite.m_TextureWidth = textureWidth;
        sprite.m_TextureHeight = textureHeight;

        float tw = static_cast<float>(textureWidth);
        float th = static_cast<float>(textureHeight);
        sprite.m_UVMin = {regionX / tw, regionY / th};
        sprite.m_UVMax = {(regionX + regionW) / tw, (regionY + regionH) / th};
        sprite.m_Size = glm::vec2(static_cast<float>(regionW), static_cast<float>(regionH));
        return sprite;
    }

    Sprite Sprite::createFromGrid(uint32_t textureID, int textureWidth, int textureHeight,
                                   int cols, int rows, int index) {
        int cellW = textureWidth / cols;
        int cellH = textureHeight / rows;
        int col = index % cols;
        int row = index / cols;
        return createFromRegion(textureID, textureWidth, textureHeight,
                                col * cellW, row * cellH, cellW, cellH);
    }

    void Sprite::setUVRect(const glm::vec2& uvMin, const glm::vec2& uvMax) {
        m_UVMin = uvMin;
        m_UVMax = uvMax;
    }

    void Sprite::setSourceRect(int x, int y, int w, int h) {
        if (m_TextureWidth <= 0 || m_TextureHeight <= 0) return;
        float tw = static_cast<float>(m_TextureWidth);
        float th = static_cast<float>(m_TextureHeight);
        m_UVMin = {x / tw, y / th};
        m_UVMax = {(x + w) / tw, (y + h) / th};
    }

    int64_t Sprite::getSortKey() const {
        return (static_cast<int64_t>(m_SortingLayer) << 32) | static_cast<int64_t>(m_OrderInLayer + 0x7FFFFFFF);
    }

    void Sprite::getEffectiveUVs(glm::vec2& outMin, glm::vec2& outMax) const {
        outMin = m_UVMin;
        outMax = m_UVMax;
        if (m_FlipX) std::swap(outMin.x, outMax.x);
        if (m_FlipY) std::swap(outMin.y, outMax.y);
    }

}
