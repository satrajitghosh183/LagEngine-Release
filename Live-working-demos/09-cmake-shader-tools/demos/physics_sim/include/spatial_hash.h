#pragma once
#include "vec2.h"
#include <vector>
#include <cstdint>
#include <algorithm>

class SpatialHash {
public:
    void build(const std::vector<Vec2>& positions, float cellSize) {
        size_t n = positions.size();
        if (n == 0) return;

        m_cellSize    = cellSize;
        m_invCellSize = 1.0f / cellSize;
        m_tableSize   = static_cast<uint32_t>(n * 2);  // 2x overallocation
        if (m_tableSize < 64) m_tableSize = 64;

        m_cellCount.assign(m_tableSize, 0);
        m_cellStart.resize(m_tableSize);
        m_indices.resize(n);

        // Pass 1: count particles per cell
        for (size_t i = 0; i < n; ++i) {
            uint32_t h = hashPos(positions[i]);
            m_cellCount[h]++;
        }

        // Prefix sum → cell start offsets
        uint32_t sum = 0;
        for (uint32_t i = 0; i < m_tableSize; ++i) {
            m_cellStart[i] = sum;
            sum += m_cellCount[i];
        }

        // Pass 2: scatter indices (counting sort)
        // Reuse cellCount as write cursors
        std::fill(m_cellCount.begin(), m_cellCount.end(), 0);
        for (size_t i = 0; i < n; ++i) {
            uint32_t h = hashPos(positions[i]);
            uint32_t idx = m_cellStart[h] + m_cellCount[h];
            m_indices[idx] = static_cast<uint32_t>(i);
            m_cellCount[h]++;
        }
    }

    // Call visitor(uint32_t j) for every particle in the 9 neighboring cells
    template<typename Func>
    inline void queryNeighbors(Vec2 pos, Func&& visitor) const {
        int cx = static_cast<int>(pos.x * m_invCellSize);
        int cy = static_cast<int>(pos.y * m_invCellSize);

        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                uint32_t h = hashCell(cx + dx, cy + dy);
                uint32_t start = m_cellStart[h];
                uint32_t end   = start + m_cellCount[h];
                for (uint32_t k = start; k < end; ++k)
                    visitor(m_indices[k]);
            }
        }
    }

private:
    inline uint32_t hashCell(int ix, int iy) const {
        uint32_t h = static_cast<uint32_t>(ix * 92837111) ^
                     static_cast<uint32_t>(iy * 689287499);
        return h % m_tableSize;
    }

    inline uint32_t hashPos(Vec2 p) const {
        int ix = static_cast<int>(p.x * m_invCellSize);
        int iy = static_cast<int>(p.y * m_invCellSize);
        return hashCell(ix, iy);
    }

    float    m_cellSize    = 1.0f;
    float    m_invCellSize = 1.0f;
    uint32_t m_tableSize   = 0;

    std::vector<uint32_t> m_cellCount;
    std::vector<uint32_t> m_cellStart;
    std::vector<uint32_t> m_indices;
};
