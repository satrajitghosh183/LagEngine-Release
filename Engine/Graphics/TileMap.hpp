#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace GameEngine {

    struct TileDefinition {
        uint16_t ID = 0;
        glm::vec2 UVMin{0.0f};
        glm::vec2 UVMax{1.0f};
        bool IsSolid = false;
        bool IsOneWayPlatform = false;
        bool IsLadder = false;
    };

    struct TileLayer {
        std::string Name = "Layer";
        std::vector<uint16_t> Tiles; // row-major, 0 = empty
        bool Visible = true;
        float ParallaxFactor = 1.0f;
    };

    class TileMap {
    public:
        TileMap() = default;
        TileMap(int width, int height, int tileSize = 32);

        // Dimensions
        void resize(int width, int height);
        int getWidth() const { return m_Width; }
        int getHeight() const { return m_Height; }
        int getTileSize() const { return m_TileSize; }
        void setTileSize(int size) { m_TileSize = size; }

        // Tileset texture
        void setTilesetTexture(uint32_t textureID, int texW, int texH, int tileW, int tileH);
        uint32_t getTilesetTexture() const { return m_TilesetTextureID; }

        // Layers
        int addLayer(const std::string& name);
        void removeLayer(int index);
        int getLayerCount() const { return static_cast<int>(m_Layers.size()); }
        TileLayer& getLayer(int index) { return m_Layers[index]; }
        const TileLayer& getLayer(int index) const { return m_Layers[index]; }

        // Tile access
        void setTile(int layer, int x, int y, uint16_t tileID);
        uint16_t getTile(int layer, int x, int y) const;

        // World position queries
        uint16_t getTileAtWorldPos(int layer, const glm::vec2& worldPos) const;
        glm::ivec2 worldToTile(const glm::vec2& worldPos) const;
        glm::vec2 tileToWorld(int x, int y) const;

        // Tile definitions
        void defineTile(uint16_t id, const TileDefinition& def);
        const TileDefinition* getTileDefinition(uint16_t id) const;

        // Auto-tiling: generate UV coords based on 4-bit neighbor bitmask
        void autoTile(int layer, int x, int y);

        // Collision
        bool isSolidAt(const glm::vec2& worldPos) const;
        bool isOneWayPlatformAt(const glm::vec2& worldPos) const;

        // Serialization
        std::string serializeToJson() const;
        bool deserializeFromJson(const std::string& json);

        // Chunk rendering helpers
        struct ChunkInfo {
            int StartX, StartY, EndX, EndY;
        };
        ChunkInfo getVisibleChunks(const glm::vec2& cameraPos, const glm::vec2& viewSize) const;

    private:
        int m_Width = 0;
        int m_Height = 0;
        int m_TileSize = 32;

        uint32_t m_TilesetTextureID = 0;
        int m_TilesetTexW = 0, m_TilesetTexH = 0;
        int m_TilesetTileW = 0, m_TilesetTileH = 0;
        int m_TilesetCols = 0, m_TilesetRows = 0;

        std::vector<TileLayer> m_Layers;
        std::unordered_map<uint16_t, TileDefinition> m_TileDefinitions;
    };

}
