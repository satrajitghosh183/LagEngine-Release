#pragma once
#include "BroadPhase.hpp"
#include "../CollisonShape.hpp"
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <algorithm> 
namespace engine::physics {

/**
 * @brief Spatial hash broad phase collision detection
 * Efficient for uniform distributions of objects
 */
class SpatialHashBroadPhase : public BroadPhase {
public:
    explicit SpatialHashBroadPhase(float cellSize = 5.0f);
    ~SpatialHashBroadPhase() override = default;
    
    // BroadPhase interface
    void insertBody(RigidBody* body) override;
    void removeBody(RigidBody* body) override;
    void updateBody(RigidBody* body) override;
    void updateAllBodies() override;
    
    std::vector<CollisionPair> findPotentialCollisions() override;
    std::vector<RigidBody*> queryRegion(const glm::vec3& min, const glm::vec3& max) override;
    std::vector<RigidBody*> queryPoint(const glm::vec3& point) override;
    
    void clear() override;
    void optimize() override;
    
    // Statistics
    size_t getBodyCount() const override { return trackedBodies.size(); }
    size_t getMemoryUsage() const override;
    std::string getDebugInfo() const override;
    
    // Configuration
    void setCellSize(float size);
    float getCellSize() const { return cellSize; }
    
    // Advanced queries
    std::vector<RigidBody*> queryRadius(const glm::vec3& center, float radius);
    
private:
    struct HashKey {
        int x, y, z;
        
        HashKey(int x = 0, int y = 0, int z = 0) : x(x), y(y), z(z) {}
        
        bool operator==(const HashKey& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
        
        bool operator<(const HashKey& other) const {
            if (x != other.x) return x < other.x;
            if (y != other.y) return y < other.y;
            return z < other.z;
        }
    };
    
    struct HashKeyHash {
        size_t operator()(const HashKey& key) const {
            // Robert Jenkins' 32 bit integer hash function
            size_t hash = 0;
            hash = ((hash << 5) + hash) + static_cast<size_t>(key.x);
            hash = ((hash << 5) + hash) + static_cast<size_t>(key.y);
            hash = ((hash << 5) + hash) + static_cast<size_t>(key.z);
            return hash;
        }
    };
    
    struct CellData {
        std::vector<RigidBody*> bodies;
        uint32_t lastUpdateFrame = 0;
        
        void addBody(RigidBody* body) {
            if (std::find(bodies.begin(), bodies.end(), body) == bodies.end()) {
                bodies.push_back(body);
            }
        }
        
        void removeBody(RigidBody* body) {
            bodies.erase(
                std::remove(bodies.begin(), bodies.end(), body),
                bodies.end()
            );
        }
        
        bool isEmpty() const { return bodies.empty(); }
    };
    
    float cellSize;
    float invCellSize;  // 1.0f / cellSize for faster division
    
    std::unordered_map<HashKey, CellData, HashKeyHash> spatialGrid;
    std::unordered_set<RigidBody*> trackedBodies;
    std::unordered_map<RigidBody*, std::vector<HashKey>> bodyToCells;
    
    uint32_t currentFrame = 0;
    size_t maxBodiesPerCell = 0;
    size_t totalCellCount = 0;
    
    // Hash computation
    HashKey getHashKey(const glm::vec3& position) const;
    std::vector<HashKey> getAABBCells(const BoundingBox& aabb) const;
    std::vector<HashKey> getBodyCells(RigidBody* body) const;
    
    // Grid management
    void insertBodyIntoGrid(RigidBody* body);
    void removeBodyFromGrid(RigidBody* body);
    void updateBodyInGrid(RigidBody* body);
    
    // Collision detection helpers
    void findPairsInCell(const CellData& cell, std::unordered_set<CollisionPair>& pairs);
    void cleanupEmptyCells();
    
    // Transform helpers
    Transform getRigidBodyTransform(RigidBody* body) const;
    BoundingBox getBodyAABB(RigidBody* body) const;
    
    // Statistics
    void updateStatistics();
};

} // namespace engine::physics