#include "SpatialHashBroadPhase.hpp"
#include "../RigidBody.hpp"
#include "../../core/Logger.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace engine::physics {

SpatialHashBroadPhase::SpatialHashBroadPhase(float cellSize) 
    : cellSize(cellSize), invCellSize(1.0f / cellSize) {
    if (cellSize <= 0.0f) {
        engine::core::log::Logger::log("Invalid cell size for SpatialHashBroadPhase, using default 5.0f", 
                                      engine::core::log::LogLevel::Warning);
        this->cellSize = 5.0f;
        this->invCellSize = 0.2f;
    }
}

void SpatialHashBroadPhase::insertBody(RigidBody* body) {
    if (!body || trackedBodies.find(body) != trackedBodies.end()) {
        return;
    }
    
    trackedBodies.insert(body);
    insertBodyIntoGrid(body);
}

void SpatialHashBroadPhase::removeBody(RigidBody* body) {
    if (!body || trackedBodies.find(body) == trackedBodies.end()) {
        return;
    }
    
    removeBodyFromGrid(body);
    trackedBodies.erase(body);
    bodyToCells.erase(body);
}

void SpatialHashBroadPhase::updateBody(RigidBody* body) {
    if (!body || trackedBodies.find(body) == trackedBodies.end()) {
        return;
    }
    
    updateBodyInGrid(body);
}

void SpatialHashBroadPhase::updateAllBodies() {
    auto start = std::chrono::high_resolution_clock::now();
    
    currentFrame++;
    
    // Update all tracked bodies
    for (RigidBody* body : trackedBodies) {
        updateBodyInGrid(body);
    }
    
    // Cleanup empty cells periodically
    if (currentFrame % 60 == 0) {
        cleanupEmptyCells();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    stats.lastUpdateTime = std::chrono::duration<float, std::milli>(end - start).count();
    
    updateStatistics();
}

std::vector<CollisionPair> SpatialHashBroadPhase::findPotentialCollisions() {
    std::unordered_set<CollisionPair> uniquePairs;
    uniquePairs.reserve(trackedBodies.size() * 2); // Rough estimate
    
    // Process each cell
    for (auto& [key, cell] : spatialGrid) {
        if (cell.bodies.size() >= 2) {
            findPairsInCell(cell, uniquePairs);
        }
    }
    
    // Convert to vector
    std::vector<CollisionPair> pairs;
    pairs.reserve(uniquePairs.size());
    
    for (const auto& pair : uniquePairs) {
        if (shouldTestPair(pair.bodyA, pair.bodyB)) {
            pairs.push_back(pair);
            incrementPairCount();
        } else {
            incrementFilteredCount();
        }
    }
    
    return pairs;
}

std::vector<RigidBody*> SpatialHashBroadPhase::queryRegion(const glm::vec3& min, const glm::vec3& max) {
    BoundingBox aabb(min, max);
    std::vector<HashKey> cells = getAABBCells(aabb);
    
    std::unordered_set<RigidBody*> uniqueBodies;
    
    for (const HashKey& key : cells) {
        auto it = spatialGrid.find(key);
        if (it != spatialGrid.end()) {
            for (RigidBody* body : it->second.bodies) {
                BoundingBox bodyAABB = getBodyAABB(body);
                if (aabb.intersects(bodyAABB)) {
                    uniqueBodies.insert(body);
                }
            }
        }
    }
    
    stats.totalQueries++;
    return std::vector<RigidBody*>(uniqueBodies.begin(), uniqueBodies.end());
}

std::vector<RigidBody*> SpatialHashBroadPhase::queryPoint(const glm::vec3& point) {
    HashKey key = getHashKey(point);
    std::vector<RigidBody*> result;
    
    auto it = spatialGrid.find(key);
    if (it != spatialGrid.end()) {
        for (RigidBody* body : it->second.bodies) {
            BoundingBox bodyAABB = getBodyAABB(body);
            if (point.x >= bodyAABB.min.x && point.x <= bodyAABB.max.x &&
                point.y >= bodyAABB.min.y && point.y <= bodyAABB.max.y &&
                point.z >= bodyAABB.min.z && point.z <= bodyAABB.max.z) {
                result.push_back(body);
            }
        }
    }
    
    stats.totalQueries++;
    return result;
}

std::vector<RigidBody*> SpatialHashBroadPhase::queryRadius(const glm::vec3& center, float radius) {
    glm::vec3 extent(radius);
    return queryRegion(center - extent, center + extent);
}

void SpatialHashBroadPhase::clear() {
    spatialGrid.clear();
    trackedBodies.clear();
    bodyToCells.clear();
    currentFrame = 0;
    maxBodiesPerCell = 0;
    totalCellCount = 0;
    resetStats();
}

void SpatialHashBroadPhase::optimize() {
    // Remove empty cells
    cleanupEmptyCells();
    
    // Shrink containers
    for (auto& [key, cell] : spatialGrid) {
        cell.bodies.shrink_to_fit();
    }
}

size_t SpatialHashBroadPhase::getMemoryUsage() const {
    size_t memory = sizeof(*this);
    memory += spatialGrid.size() * (sizeof(HashKey) + sizeof(CellData));
    memory += trackedBodies.size() * sizeof(RigidBody*);
    memory += bodyToCells.size() * sizeof(std::pair<RigidBody*, std::vector<HashKey>>);
    
    for (const auto& [body, cells] : bodyToCells) {
        memory += cells.size() * sizeof(HashKey);
    }
    
    for (const auto& [key, cell] : spatialGrid) {
        memory += cell.bodies.size() * sizeof(RigidBody*);
    }
    
    return memory;
}

std::string SpatialHashBroadPhase::getDebugInfo() const {
    std::ostringstream oss;
    oss << "SpatialHashBroadPhase Debug Info:\n";
    oss << "  Cell Size: " << cellSize << "\n";
    oss << "  Tracked Bodies: " << trackedBodies.size() << "\n";
    oss << "  Active Cells: " << spatialGrid.size() << "\n";
    oss << "  Max Bodies per Cell: " << maxBodiesPerCell << "\n";
    oss << "  Memory Usage: " << (getMemoryUsage() / 1024) << " KB\n";
    oss << "  Last Update Time: " << stats.lastUpdateTime << " ms\n";
    oss << "  Total Queries: " << stats.totalQueries << "\n";
    oss << "  Pairs Generated: " << stats.pairsGenerated << "\n";
    oss << "  Pairs Filtered: " << stats.pairsFiltered << "\n";
    
    if (!spatialGrid.empty()) {
        size_t totalBodies = 0;
        for (const auto& [key, cell] : spatialGrid) {
            totalBodies += cell.bodies.size();
        }
        float avgBodiesPerCell = static_cast<float>(totalBodies) / spatialGrid.size();
        oss << "  Average Bodies per Cell: " << avgBodiesPerCell << "\n";
    }
    
    return oss.str();
}

void SpatialHashBroadPhase::setCellSize(float size) {
    if (size <= 0.0f) return;
    
    cellSize = size;
    invCellSize = 1.0f / size;
    
    // Rebuild the grid with new cell size
    std::unordered_set<RigidBody*> bodiesToReinsert = trackedBodies;
    clear();
    
    for (RigidBody* body : bodiesToReinsert) {
        insertBody(body);
    }
}

SpatialHashBroadPhase::HashKey SpatialHashBroadPhase::getHashKey(const glm::vec3& position) const {
    return HashKey(
        static_cast<int>(std::floor(position.x * invCellSize)),
        static_cast<int>(std::floor(position.y * invCellSize)),
        static_cast<int>(std::floor(position.z * invCellSize))
    );
}

std::vector<SpatialHashBroadPhase::HashKey> SpatialHashBroadPhase::getAABBCells(const BoundingBox& aabb) const {
    HashKey minKey = getHashKey(aabb.min);
    HashKey maxKey = getHashKey(aabb.max);
    
    std::vector<HashKey> cells;
    
    for (int x = minKey.x; x <= maxKey.x; ++x) {
        for (int y = minKey.y; y <= maxKey.y; ++y) {
            for (int z = minKey.z; z <= maxKey.z; ++z) {
                cells.emplace_back(x, y, z);
            }
        }
    }
    
    return cells;
}

std::vector<SpatialHashBroadPhase::HashKey> SpatialHashBroadPhase::getBodyCells(RigidBody* body) const {
    BoundingBox aabb = getBodyAABB(body);
    return getAABBCells(aabb);
}

void SpatialHashBroadPhase::insertBodyIntoGrid(RigidBody* body) {
    if (!body) return;
    
    std::vector<HashKey> cells = getBodyCells(body);
    bodyToCells[body] = cells;
    
    for (const HashKey& key : cells) {
        spatialGrid[key].addBody(body);
        spatialGrid[key].lastUpdateFrame = currentFrame;
    }
}

void SpatialHashBroadPhase::removeBodyFromGrid(RigidBody* body) {
    if (!body) return;
    
    auto it = bodyToCells.find(body);
    if (it != bodyToCells.end()) {
        for (const HashKey& key : it->second) {
            auto cellIt = spatialGrid.find(key);
            if (cellIt != spatialGrid.end()) {
                cellIt->second.removeBody(body);
            }
        }
    }
}

void SpatialHashBroadPhase::updateBodyInGrid(RigidBody* body) {
    if (!body) return;
    
    std::vector<HashKey> newCells = getBodyCells(body);
    
    auto it = bodyToCells.find(body);
    if (it != bodyToCells.end()) {
        const std::vector<HashKey>& oldCells = it->second;
        
        // Check if cells have changed
        if (oldCells != newCells) {
            // Remove from old cells
            for (const HashKey& key : oldCells) {
                auto cellIt = spatialGrid.find(key);
                if (cellIt != spatialGrid.end()) {
                    cellIt->second.removeBody(body);
                }
            }
            
            // Add to new cells
            for (const HashKey& key : newCells) {
                spatialGrid[key].addBody(body);
                spatialGrid[key].lastUpdateFrame = currentFrame;
            }
            
            // Update mapping
            it->second = newCells;
        }
    } else {
        insertBodyIntoGrid(body);
    }
}

void SpatialHashBroadPhase::findPairsInCell(const CellData& cell, std::unordered_set<CollisionPair>& pairs) {
    const std::vector<RigidBody*>& bodies = cell.bodies;
    
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            CollisionPair pair(bodies[i], bodies[j]);
            pairs.insert(pair);
        }
    }
}

void SpatialHashBroadPhase::cleanupEmptyCells() {
    auto it = spatialGrid.begin();
    while (it != spatialGrid.end()) {
        if (it->second.isEmpty()) {
            it = spatialGrid.erase(it);
        } else {
            ++it;
        }
    }
}

Transform SpatialHashBroadPhase::getRigidBodyTransform(RigidBody* body) const {
    Transform transform;
    transform.position = body->getPosition();
    transform.rotation = body->getOrientation();
    transform.scale = glm::vec3(1.0f);
    return transform;
}

BoundingBox SpatialHashBroadPhase::getBodyAABB(RigidBody* body) const {
    if (!body->getCollisionShape()) {
        return BoundingBox(body->getPosition(), body->getPosition());
    }
    
    Transform transform = getRigidBodyTransform(body);
    return body->getCollisionShape()->getAABB(transform);
}

void SpatialHashBroadPhase::updateStatistics() {
    maxBodiesPerCell = 0;
    totalCellCount = spatialGrid.size();
    
    for (const auto& [key, cell] : spatialGrid) {
        maxBodiesPerCell = std::max(maxBodiesPerCell, cell.bodies.size());
    }
}

} // namespace engine::physics