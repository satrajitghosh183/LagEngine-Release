#include "Sprite.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    Sprite::Sprite() = default;

    // =========================================================================
    // Factory helpers
    // =========================================================================

    Sprite Sprite::createFromFile(const std::string& filePath) {
        Sprite sprite;
        sprite.m_Texture = CreateRef<Texture2D>(filePath);

        if (sprite.m_Texture) {
            float w = static_cast<float>(sprite.m_Texture->GetWidth());
            float h = static_cast<float>(sprite.m_Texture->GetHeight());
            sprite.m_Size = glm::vec2(w, h);
        }

        return sprite;
    }

    Sprite Sprite::createFromTexture(const Ref<Texture2D>& texture) {
        Sprite sprite;
        sprite.m_Texture = texture;
        if (texture) {
            sprite.m_Size = glm::vec2(
                static_cast<float>(texture->GetWidth()),
                static_cast<float>(texture->GetHeight()));
        }
        return sprite;
    }

    Sprite Sprite::createFromRegion(const Ref<Texture2D>& texture,
                                     int regionX, int regionY,
                                     int regionW, int regionH) {
        Sprite sprite;
        sprite.m_Texture = texture;

        if (texture && texture->GetWidth() > 0 && texture->GetHeight() > 0) {
            float tw = static_cast<float>(texture->GetWidth());
            float th = static_cast<float>(texture->GetHeight());
            sprite.m_UVMin = { regionX / tw, regionY / th };
            sprite.m_UVMax = { (regionX + regionW) / tw, (regionY + regionH) / th };
        }

        sprite.m_Size = glm::vec2(static_cast<float>(regionW),
                                   static_cast<float>(regionH));
        return sprite;
    }

    Sprite Sprite::createFromGrid(const Ref<Texture2D>& texture,
                                   int cols, int rows, int index) {
        if (!texture || cols <= 0 || rows <= 0) return {};

        int cellW = static_cast<int>(texture->GetWidth())  / cols;
        int cellH = static_cast<int>(texture->GetHeight()) / rows;
        int col   = index % cols;
        int row   = index / cols;

        return createFromRegion(texture,
                                col * cellW, row * cellH, cellW, cellH);
    }

    // =========================================================================
    // Texture descriptor access
    // =========================================================================

    VkDescriptorImageInfo Sprite::getDescriptorInfo() const {
        if (!m_Texture) {
            return VkDescriptorImageInfo{VK_NULL_HANDLE, VK_NULL_HANDLE,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        }
        return m_Texture->GetDescriptorInfo();
    }

    // =========================================================================
    // UV helpers
    // =========================================================================

    void Sprite::setUVRect(const glm::vec2& uvMin, const glm::vec2& uvMax) {
        m_UVMin = uvMin;
        m_UVMax = uvMax;
    }

    void Sprite::setSourceRect(int x, int y, int w, int h) {
        if (!m_Texture || m_Texture->GetWidth() == 0 || m_Texture->GetHeight() == 0) return;
        float tw = static_cast<float>(m_Texture->GetWidth());
        float th = static_cast<float>(m_Texture->GetHeight());
        m_UVMin = { x / tw, y / th };
        m_UVMax = { (x + w) / tw, (y + h) / th };
    }

    // =========================================================================
    // Sorting
    // =========================================================================

    int64_t Sprite::getSortKey() const {
        return (static_cast<int64_t>(m_SortingLayer) << 32) |
               static_cast<int64_t>(m_OrderInLayer + 0x7FFFFFFF);
    }

    // =========================================================================
    // Effective UV (with flip)
    // =========================================================================

    void Sprite::getEffectiveUVs(glm::vec2& outMin, glm::vec2& outMax) const {
        outMin = m_UVMin;
        outMax = m_UVMax;
        if (m_FlipX) std::swap(outMin.x, outMax.x);
        if (m_FlipY) std::swap(outMin.y, outMax.y);
    }

} // namespace GameEngine
