#include "SpatialHashGrid.hpp"
#include <cmath>

namespace GameEngine::Physics {

    SpatialHashGrid::SpatialHashGrid(float cellSize)
        : m_CellSize(cellSize), m_InvCellSize(1.0f / cellSize) {}

    void SpatialHashGrid::Clear() { m_Grid.clear(); }

    glm::ivec3 SpatialHashGrid::GetCell(const glm::vec3& pos) const {
        return glm::ivec3(
            static_cast<int>(std::floor(pos.x * m_InvCellSize)),
            static_cast<int>(std::floor(pos.y * m_InvCellSize)),
            static_cast<int>(std::floor(pos.z * m_InvCellSize))
        );
    }

    uint64_t SpatialHashGrid::HashPosition(const glm::vec3& pos) const {
        auto cell = GetCell(pos);
        uint64_t h = static_cast<uint64_t>(cell.x * 73856093) ^
                     static_cast<uint64_t>(cell.y * 19349663) ^
                     static_cast<uint64_t>(cell.z * 83492791);
        return h;
    }

    void SpatialHashGrid::Insert(int particleIndex, const glm::vec3& position) {
        uint64_t hash = HashPosition(position);
        m_Grid[hash].push_back(particleIndex);
    }

    std::vector<int> SpatialHashGrid::Query(const glm::vec3& position, float radius) const {
        std::vector<int> results;
        int cellRadius = static_cast<int>(std::ceil(radius * m_InvCellSize));
        auto centerCell = GetCell(position);

        for (int dx = -cellRadius; dx <= cellRadius; dx++) {
            for (int dy = -cellRadius; dy <= cellRadius; dy++) {
                for (int dz = -cellRadius; dz <= cellRadius; dz++) {
                    glm::vec3 queryPos = glm::vec3(
                        (centerCell.x + dx) * m_CellSize,
                        (centerCell.y + dy) * m_CellSize,
                        (centerCell.z + dz) * m_CellSize
                    );
                    uint64_t hash = HashPosition(queryPos);
                    auto it = m_Grid.find(hash);
                    if (it != m_Grid.end()) {
                        results.insert(results.end(), it->second.begin(), it->second.end());
                    }
                }
            }
        }
        return results;
    }

}
