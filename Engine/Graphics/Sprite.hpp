#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief 2D sprite with texture region, tint, flip, pivot, and sorting support
     *
     * Represents a rectangular region of a texture that can be drawn by the
     * BatchRenderer2D.  Supports atlas/spritesheet workflows: create from a
     * full texture, a sub-rectangle, or by slicing a grid.
     *
     * Usage:
     *   auto sprite = Sprite::createFromFile("assets/player.png");
     *   sprite.setColor({1, 1, 1, 1});
     *   sprite.setFlipX(true);
     *
     *   // Atlas workflow
     *   auto frame = Sprite::createFromGrid(atlasTexID, 512, 512, 4, 4, 0);
     */
    class Sprite {
    public:
        Sprite();
        ~Sprite() = default;

        // -------------------------------------------------------------------
        // Factory helpers
        // -------------------------------------------------------------------

        /**
         * @brief Create a sprite that covers the full texture loaded from disk.
         * @param filePath  Path to the image file (PNG, JPG, etc.)
         * @return Configured Sprite (texture ID is 0 if loading fails)
         */
        static Sprite createFromFile(const std::string& filePath);

        /**
         * @brief Create a sprite from an existing OpenGL texture ID.
         */
        static Sprite createFromTexture(uint32_t textureID, int textureWidth, int textureHeight);

        /**
         * @brief Create a sprite from a specific pixel rectangle in a texture.
         */
        static Sprite createFromRegion(uint32_t textureID, int textureWidth, int textureHeight,
                                       int regionX, int regionY, int regionW, int regionH);

        /**
         * @brief Create a sprite by indexing into a uniform grid (spritesheet).
         * @param cols       Number of columns in the grid
         * @param rows       Number of rows in the grid
         * @param index      Zero-based cell index (row-major)
         */
        static Sprite createFromGrid(uint32_t textureID, int textureWidth, int textureHeight,
                                     int cols, int rows, int index);

        // -------------------------------------------------------------------
        // Texture
        // -------------------------------------------------------------------

        void setTextureID(uint32_t id) { m_TextureID = id; }
        uint32_t getTextureID() const { return m_TextureID; }

        void setTextureSize(int w, int h) { m_TextureWidth = w; m_TextureHeight = h; }
        int getTextureWidth() const { return m_TextureWidth; }
        int getTextureHeight() const { return m_TextureHeight; }

        // -------------------------------------------------------------------
        // Source rectangle (UV)
        // -------------------------------------------------------------------

        /** Set the source rectangle in normalised UV coordinates [0..1]. */
        void setUVRect(const glm::vec2& uvMin, const glm::vec2& uvMax);

        /** Set the source rectangle in pixel coordinates. */
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

        /** Pivot / origin for rotation, in normalised [0..1] range.  (0.5,0.5) = center. */
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

        /**
         * @brief Compute a 64-bit sort key: high 32 bits = layer, low 32 bits = order.
         */
        int64_t getSortKey() const;

        // -------------------------------------------------------------------
        // Helpers
        // -------------------------------------------------------------------

        /**
         * @brief Return UVs with flip applied (ready for vertex submission).
         * @param outMin  receives the effective min UV
         * @param outMax  receives the effective max UV
         */
        void getEffectiveUVs(glm::vec2& outMin, glm::vec2& outMax) const;

    private:
        uint32_t m_TextureID = 0;
        int m_TextureWidth = 0;
        int m_TextureHeight = 0;

        glm::vec2 m_UVMin = {0.0f, 0.0f};
        glm::vec2 m_UVMax = {1.0f, 1.0f};

        glm::vec4 m_Color = {1.0f, 1.0f, 1.0f, 1.0f};

        bool m_FlipX = false;
        bool m_FlipY = false;

        glm::vec2 m_Pivot = {0.5f, 0.5f};
        glm::vec2 m_Size = {1.0f, 1.0f};

        int m_SortingLayer = 0;
        int m_OrderInLayer = 0;
    };

} // namespace GameEngine
