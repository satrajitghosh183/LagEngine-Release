#pragma once
#include <vector>
#include <memory>
#include <string>

namespace engine::physics {

class RigidBody;

/**
 * @brief Represents a potential collision between two bodies
 */
struct CollisionPair {
    RigidBody* bodyA;
    RigidBody* bodyB;
    
    CollisionPair(RigidBody* a, RigidBody* b) : bodyA(a), bodyB(b) {
        // Ensure consistent ordering for comparison
        if (a > b) {
            std::swap(bodyA, bodyB);
        }
    }
    
    bool operator==(const CollisionPair& other) const {
        return bodyA == other.bodyA && bodyB == other.bodyB;
    }
    
    bool operator<(const CollisionPair& other) const {
        if (bodyA != other.bodyA) return bodyA < other.bodyA;
        return bodyB < other.bodyB;
    }
    
    bool contains(RigidBody* body) const {
        return bodyA == body || bodyB == body;
    }
};

/**
 * @brief Base class for broad phase collision detection algorithms
 */
class BroadPhase {
public:
    virtual ~BroadPhase() = default;
    
    // Body management
    virtual void insertBody(RigidBody* body) = 0;
    virtual void removeBody(RigidBody* body) = 0;
    virtual void updateBody(RigidBody* body) = 0;
    virtual void updateAllBodies() = 0;
    
    // Collision detection
    virtual std::vector<CollisionPair> findPotentialCollisions() = 0;
    virtual std::vector<RigidBody*> queryRegion(const glm::vec3& min, const glm::vec3& max) = 0;
    virtual std::vector<RigidBody*> queryPoint(const glm::vec3& point) = 0;
    
    // Lifecycle
    virtual void clear() = 0;
    virtual void optimize() {} // Optional optimization step
    
    // Statistics
    virtual size_t getBodyCount() const = 0;
    virtual size_t getMemoryUsage() const = 0;
    virtual std::string getDebugInfo() const = 0;
    
    // Performance tracking
    struct Stats {
        size_t pairsGenerated = 0;
        size_t pairsFiltered = 0;
        float lastUpdateTime = 0.0f;
        size_t totalQueries = 0;
    };
    
    const Stats& getStats() const { return stats; }
    void resetStats() { stats = Stats{}; }

protected:
    Stats stats;
    
    // Helper methods for derived classes
    bool shouldTestPair(RigidBody* bodyA, RigidBody* bodyB) const;
    void incrementPairCount() { stats.pairsGenerated++; }
    void incrementFilteredCount() { stats.pairsFiltered++; }
};

} // namespace engine::physics