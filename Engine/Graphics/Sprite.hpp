#pragma once

#include "../Core/Base.hpp"
#include "Texture2D.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief 2D sprite with texture region, tint, flip, pivot, and sorting support.
     *
     * Represents a rectangular region of a Texture2D that can be submitted to
     * BatchRenderer2D.  Supports atlas / spritesheet workflows:
     *   - Create from a file (loads a Texture2D internally)
     *   - Create from an existing Texture2D + optional sub-rectangle
     *   - Slice a uniform grid (spritesheet)
     *
     * The texture is stored as a Ref<Texture2D> (Vulkan VkImage + VkSampler).
     * The textureID getter returns a VkDescriptorImageInfo-compatible opaque
     * handle (the VkImageView cast to uint64_t) for backward-compat with code
     * that uses uint32_t IDs — prefer getTexture() / getDescriptorInfo() in
     * new Vulkan-aware code.
     *
     * Usage:
     *   auto sprite = Sprite::createFromFile("assets/player.png");
     *   sprite.setColor({1, 1, 1, 1});
     *   sprite.setFlipX(true);
     *
     *   // Atlas workflow
     *   auto frame = Sprite::createFromGrid(atlasTexture, 4, 4, 0);
     */
    class Sprite {
    public:
        Sprite();
        ~Sprite() = default;

        // -------------------------------------------------------------------
        // Factory helpers
        // -------------------------------------------------------------------

        /** @brief Load a texture from disk and build a full-frame sprite. */
        static Sprite createFromFile(const std::string& filePath);

        /** @brief Build a sprite from an existing Texture2D (full frame). */
        static Sprite createFromTexture(const Ref<Texture2D>& texture);

        /**
         * @brief Build a sprite from a pixel sub-rectangle of a texture.
         * @param regionX  Pixel X of the top-left corner
         * @param regionY  Pixel Y of the top-left corner
         * @param regionW  Width in pixels
         * @param regionH  Height in pixels
         */
        static Sprite createFromRegion(const Ref<Texture2D>& texture,
                                       int regionX, int regionY,
                                       int regionW, int regionH);

        /**
         * @brief Slice a uniform grid and return the sprite for cell [index].
         * @param cols  Number of columns
         * @param rows  Number of rows
         * @param index Zero-based cell index (row-major)
         */
        static Sprite createFromGrid(const Ref<Texture2D>& texture,
                                     int cols, int rows, int index);

        // -------------------------------------------------------------------
        // Texture access
        // -------------------------------------------------------------------

        void setTexture(const Ref<Texture2D>& tex) { m_Texture = tex; }
        const Ref<Texture2D>& getTexture() const { return m_Texture; }

        /** @brief Get descriptor info for binding to a descriptor set. */
        VkDescriptorImageInfo getDescriptorInfo() const;

        // -------------------------------------------------------------------
        // Source rectangle (UV)
        // -------------------------------------------------------------------

        /** Set UV rectangle in normalised [0..1] coordinates. */
        void setUVRect(const glm::vec2& uvMin, const glm::vec2& uvMax);

        /** Set UV rectangle from pixel coordinates within the texture. */
        void setSourceRect(int x, int y, int w, int h);

        const glm::vec2& getUVMin() const { return m_UVMin; }
        const glm::vec2& getUVMax() const { return m_UVMax; }

        // -------------------------------------------------------------------
        // Visual properties
        // -------------------------------------------------------------------

        void setColor(const glm::vec4& color) { m_Color = color; }
        const glm::vec4& getColor() const { return m_Color; }

        void setFlipX(bool flip) { m_FlipX = flip; }
        void setFlipY(bool flip) { m_FlipY = flip; }
        bool getFlipX() const { return m_FlipX; }
        bool getFlipY() const { return m_FlipY; }

        /** Pivot / origin for rotation, normalised [0..1].  (0.5,0.5) = center. */
        void setPivot(const glm::vec2& pivot) { m_Pivot = pivot; }
        const glm::vec2& getPivot() const { return m_Pivot; }

        void setSize(const glm::vec2& size) { m_Size = size; }
        const glm::vec2& getSize() const { return m_Size; }

        // -------------------------------------------------------------------
        // Sorting
        // -------------------------------------------------------------------

        void setSortingLayer(int layer) { m_SortingLayer = layer; }
        int getSortingLayer() const { return m_SortingLayer; }

        void setOrderInLayer(int order) { m_OrderInLayer = order; }
        int getOrderInLayer() const { return m_OrderInLayer; }

        /** Compute a 64-bit sort key: high 32 bits = layer, low 32 bits = order. */
        int64_t getSortKey() const;

        // -------------------------------------------------------------------
        // Helpers
        // -------------------------------------------------------------------

        /** Return UVs with flip applied (ready for vertex submission). */
        void getEffectiveUVs(glm::vec2& outMin, glm::vec2& outMax) const;

    private:
        Ref<Texture2D> m_Texture;

        glm::vec2 m_UVMin = {0.0f, 0.0f};
        glm::vec2 m_UVMax = {1.0f, 1.0f};

        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};

        bool      m_FlipX = false;
        bool      m_FlipY = false;

        glm::vec2 m_Pivot = {0.5f, 0.5f};
        glm::vec2 m_Size  = {1.0f, 1.0f};

        int m_SortingLayer  = 0;
        int m_OrderInLayer  = 0;
    };

} // namespace GameEngine
