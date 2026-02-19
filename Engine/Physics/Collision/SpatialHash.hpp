#pragma once

#include "../../Core/Base.hpp"
#include "../RigidBody.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Spatial hash grid for broadphase collision detection
     * 
     * Divides space into uniform cells and tracks which objects
     * occupy each cell. Only objects in the same or adjacent cells
     * are tested for collision.
     * 
     * Time complexity:
     * - Insert: O(k) where k is number of cells object overlaps
     * - Query: O(k*m) where m is average objects per cell
     * - Update: O(k)
     */
    class SpatialHash {
    public:
        /**
         * @brief Constructor
         * @param cellSize Size of each grid cell (larger = fewer cells but more objects per cell)
         */
        SpatialHash(float cellSize = 2.0f);
        ~SpatialHash() = default;
        
        /**
         * @brief Clear all entries
         */
        void Clear();
        
        /**
         * @brief Insert a rigid body into the spatial hash
         */
        void Insert(const Ref<RigidBody>& body);
        
        /**
         * @brief Remove a rigid body from the spatial hash
         */
        void Remove(const Ref<RigidBody>& body);
        
        /**
         * @brief Update the spatial hash (rebuild from scratch)
         * @param bodies All rigid bodies to track
         */
        void Update(const std::vector<Ref<RigidBody>>& bodies);
        
        /**
         * @brief Get all potential collision pairs
         * @return Vector of body pairs that might be colliding
         */
        std::vector<std::pair<Ref<RigidBody>, Ref<RigidBody>>> GetPotentialPairs() const;
        
        /**
         * @brief Query all bodies near a point
         * @param point Query point
         * @param radius Search radius
         * @return Bodies within range
         */
        std::vector<Ref<RigidBody>> QueryPoint(const glm::vec3& point, float radius = 0.0f) const;
        
        /**
         * @brief Query all bodies overlapping an AABB
         * @param min AABB minimum
         * @param max AABB maximum
         * @return Bodies overlapping the AABB
         */
        std::vector<Ref<RigidBody>> QueryAABB(const glm::vec3& min, const glm::vec3& max) const;
        
        /**
         * @brief Get cell size
         */
        float GetCellSize() const { return m_CellSize; }
        
        /**
         * @brief Set cell size (requires rebuild)
         */
        void SetCellSize(float cellSize) { m_CellSize = cellSize; }
        
        /**
         * @brief Get number of occupied cells
         */
        size_t GetOccupiedCellCount() const { return m_Cells.size(); }
        
        /**
         * @brief Get debug info about cell distribution
         */
        void GetDebugInfo(int& maxBodiesPerCell, float& avgBodiesPerCell) const;
        
    private:
        /**
         * @brief Hash function for cell coordinates
         */
        struct CellHash {
            size_t operator()(const glm::ivec3& cell) const {
                // Use FNV-1a hash
                const size_t prime = 16777619;
                size_t hash = 2166136261;
                hash = (hash ^ cell.x) * prime;
                hash = (hash ^ cell.y) * prime;
                hash = (hash ^ cell.z) * prime;
                return hash;
            }
        };
        
        struct CellEqual {
            bool operator()(const glm::ivec3& a, const glm::ivec3& b) const {
                return a.x == b.x && a.y == b.y && a.z == b.z;
            }
        };
        
        /**
         * @brief Convert world position to cell coordinates
         */
        glm::ivec3 WorldToCell(const glm::vec3& position) const;
        
        /**
         * @brief Get all cells that an AABB overlaps
         */
        std::vector<glm::ivec3> GetCellsForAABB(const glm::vec3& min, const glm::vec3& max) const;
        
        /**
         * @brief Get AABB for a rigid body
         */
        void GetBodyAABB(const Ref<RigidBody>& body, glm::vec3& outMin, glm::vec3& outMax) const;
        
    private:
        float m_CellSize;
        
        // Maps cell coordinates to bodies in that cell
        std::unordered_map<glm::ivec3, std::vector<Ref<RigidBody>>, CellHash, CellEqual> m_Cells;
        
        // Track which cells each body is in (for efficient removal)
        std::unordered_map<RigidBody*, std::vector<glm::ivec3>> m_BodyToCells;
    };

}}
