#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace GameEngine::Physics {

    class SpatialHashGrid {
    public:
        explicit SpatialHashGrid(float cellSize = 0.1f);
        void Clear();
        void Insert(int particleIndex, const glm::vec3& position);
        std::vector<int> Query(const glm::vec3& position, float radius) const;
        void SetCellSize(float cellSize) { m_CellSize = cellSize; m_InvCellSize = 1.0f / cellSize; }

    private:
        uint64_t HashPosition(const glm::vec3& pos) const;
        glm::ivec3 GetCell(const glm::vec3& pos) const;

        float m_CellSize;
        float m_InvCellSize;
        std::unordered_map<uint64_t, std::vector<int>> m_Grid;
    };

}
