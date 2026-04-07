#include "TileMap.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>

namespace GameEngine {

    TileMap::TileMap(int width, int height, int tileSize)
        : m_Width(width), m_Height(height), m_TileSize(tileSize) {
        addLayer("Background");
    }

    void TileMap::resize(int width, int height) {
        for (auto& layer : m_Layers) {
            std::vector<uint16_t> newTiles(width * height, 0);
            int copyW = std::min(width, m_Width);
            int copyH = std::min(height, m_Height);
            for (int y = 0; y < copyH; y++) {
                for (int x = 0; x < copyW; x++) {
                    newTiles[y * width + x] = layer.Tiles[y * m_Width + x];
                }
            }
            layer.Tiles = std::move(newTiles);
        }
        m_Width = width;
        m_Height = height;
    }

    void TileMap::setTilesetTexture(uint32_t textureID, int texW, int texH, int tileW, int tileH) {
        m_TilesetTextureID = textureID;
        m_TilesetTexW = texW;
        m_TilesetTexH = texH;
        m_TilesetTileW = tileW;
        m_TilesetTileH = tileH;
        m_TilesetCols = texW / tileW;
        m_TilesetRows = texH / tileH;

        // Auto-generate tile definitions from tileset grid
        for (int row = 0; row < m_TilesetRows; row++) {
            for (int col = 0; col < m_TilesetCols; col++) {
                uint16_t id = static_cast<uint16_t>(row * m_TilesetCols + col + 1); // 1-based
                TileDefinition def;
                def.ID = id;
                def.UVMin = glm::vec2(
                    static_cast<float>(col * tileW) / texW,
                    static_cast<float>(row * tileH) / texH
                );
                def.UVMax = glm::vec2(
                    static_cast<float>((col + 1) * tileW) / texW,
                    static_cast<float>((row + 1) * tileH) / texH
                );
                if (m_TileDefinitions.find(id) == m_TileDefinitions.end()) {
                    m_TileDefinitions[id] = def;
                }
            }
        }
    }

    int TileMap::addLayer(const std::string& name) {
        TileLayer layer;
        layer.Name = name;
        layer.Tiles.resize(m_Width * m_Height, 0);
        m_Layers.push_back(std::move(layer));
        return static_cast<int>(m_Layers.size()) - 1;
    }

    void TileMap::removeLayer(int index) {
        if (index >= 0 && index < static_cast<int>(m_Layers.size())) {
            m_Layers.erase(m_Layers.begin() + index);
        }
    }

    void TileMap::setTile(int layer, int x, int y, uint16_t tileID) {
        if (layer < 0 || layer >= static_cast<int>(m_Layers.size())) return;
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) return;
        m_Layers[layer].Tiles[y * m_Width + x] = tileID;
    }

    uint16_t TileMap::getTile(int layer, int x, int y) const {
        if (layer < 0 || layer >= static_cast<int>(m_Layers.size())) return 0;
        if (x < 0 || x >= m_Width || y < 0 || y >= m_Height) return 0;
        return m_Layers[layer].Tiles[y * m_Width + x];
    }

    uint16_t TileMap::getTileAtWorldPos(int layer, const glm::vec2& worldPos) const {
        glm::ivec2 tile = worldToTile(worldPos);
        return getTile(layer, tile.x, tile.y);
    }

    glm::ivec2 TileMap::worldToTile(const glm::vec2& worldPos) const {
        return glm::ivec2(
            static_cast<int>(std::floor(worldPos.x / m_TileSize)),
            static_cast<int>(std::floor(worldPos.y / m_TileSize))
        );
    }

    glm::vec2 TileMap::tileToWorld(int x, int y) const {
        return glm::vec2(
            static_cast<float>(x * m_TileSize),
            static_cast<float>(y * m_TileSize)
        );
    }

    void TileMap::defineTile(uint16_t id, const TileDefinition& def) {
        m_TileDefinitions[id] = def;
    }

    const TileDefinition* TileMap::getTileDefinition(uint16_t id) const {
        auto it = m_TileDefinitions.find(id);
        return (it != m_TileDefinitions.end()) ? &it->second : nullptr;
    }

    void TileMap::autoTile(int layer, int x, int y) {
        // 4-bit bitmask auto-tiling
        uint16_t tile = getTile(layer, x, y);
        if (tile == 0) return;

        uint8_t mask = 0;
        if (getTile(layer, x, y - 1) == tile) mask |= 0x01; // top
        if (getTile(layer, x + 1, y) == tile) mask |= 0x02; // right
        if (getTile(layer, x, y + 1) == tile) mask |= 0x04; // bottom
        if (getTile(layer, x - 1, y) == tile) mask |= 0x08; // left

        // Map bitmask to tile variant (tile + mask gives variant ID)
        uint16_t variantID = static_cast<uint16_t>(tile * 16 + mask);
        auto it = m_TileDefinitions.find(variantID);
        if (it != m_TileDefinitions.end()) {
            m_Layers[layer].Tiles[y * m_Width + x] = variantID;
        }
    }

    bool TileMap::isSolidAt(const glm::vec2& worldPos) const {
        glm::ivec2 tile = worldToTile(worldPos);
        for (const auto& layer : m_Layers) {
            if (tile.x < 0 || tile.x >= m_Width || tile.y < 0 || tile.y >= m_Height) continue;
            uint16_t id = layer.Tiles[tile.y * m_Width + tile.x];
            if (id == 0) continue;
            auto it = m_TileDefinitions.find(id);
            if (it != m_TileDefinitions.end() && it->second.IsSolid) return true;
        }
        return false;
    }

    bool TileMap::isOneWayPlatformAt(const glm::vec2& worldPos) const {
        glm::ivec2 tile = worldToTile(worldPos);
        for (const auto& layer : m_Layers) {
            if (tile.x < 0 || tile.x >= m_Width || tile.y < 0 || tile.y >= m_Height) continue;
            uint16_t id = layer.Tiles[tile.y * m_Width + tile.x];
            if (id == 0) continue;
            auto it = m_TileDefinitions.find(id);
            if (it != m_TileDefinitions.end() && it->second.IsOneWayPlatform) return true;
        }
        return false;
    }

    TileMap::ChunkInfo TileMap::getVisibleChunks(const glm::vec2& cameraPos, const glm::vec2& viewSize) const {
        float halfW = viewSize.x * 0.5f;
        float halfH = viewSize.y * 0.5f;

        ChunkInfo info;
        info.StartX = std::max(0, static_cast<int>(std::floor((cameraPos.x - halfW) / m_TileSize)) - 1);
        info.StartY = std::max(0, static_cast<int>(std::floor((cameraPos.y - halfH) / m_TileSize)) - 1);
        info.EndX = std::min(m_Width, static_cast<int>(std::ceil((cameraPos.x + halfW) / m_TileSize)) + 1);
        info.EndY = std::min(m_Height, static_cast<int>(std::ceil((cameraPos.y + halfH) / m_TileSize)) + 1);
        return info;
    }

    std::string TileMap::serializeToJson() const {
        nlohmann::json j;
        j["width"] = m_Width;
        j["height"] = m_Height;
        j["tileSize"] = m_TileSize;

        j["layers"] = nlohmann::json::array();
        for (const auto& layer : m_Layers) {
            nlohmann::json layerJ;
            layerJ["name"] = layer.Name;
            layerJ["visible"] = layer.Visible;
            layerJ["parallaxFactor"] = layer.ParallaxFactor;
            layerJ["tiles"] = layer.Tiles;
            j["layers"].push_back(layerJ);
        }

        return j.dump(2);
    }

    bool TileMap::deserializeFromJson(const std::string& json) {
        try {
            auto j = nlohmann::json::parse(json);
            m_Width = j["width"];
            m_Height = j["height"];
            m_TileSize = j["tileSize"];

            m_Layers.clear();
            for (const auto& layerJ : j["layers"]) {
                TileLayer layer;
                layer.Name = layerJ["name"];
                layer.Visible = layerJ.value("visible", true);
                layer.ParallaxFactor = layerJ.value("parallaxFactor", 1.0f);
                layer.Tiles = layerJ["tiles"].get<std::vector<uint16_t>>();
                m_Layers.push_back(std::move(layer));
            }
            return true;
        } catch (...) {
            return false;
        }
    }

}
