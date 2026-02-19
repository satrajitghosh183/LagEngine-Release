#include "SpatialHash.hpp"
#include "../../Core/Logger.hpp"
#include "../Shapes/CollisionShape.hpp"
#include <cmath>
#include <algorithm>
#include <set>

namespace GameEngine {
namespace Physics {

    SpatialHash::SpatialHash(float cellSize)
        : m_CellSize(cellSize) {
        
        if (m_CellSize <= 0) {
            m_CellSize = 2.0f;
            GE_CORE_WARN("SpatialHash: Invalid cell size, defaulting to 2.0");
        }
    }

    void SpatialHash::Clear() {
        m_Cells.clear();
        m_BodyToCells.clear();
    }

    void SpatialHash::Insert(const Ref<RigidBody>& body) {
        if (!body) return;
        
        glm::vec3 min, max;
        GetBodyAABB(body, min, max);
        
        auto cells = GetCellsForAABB(min, max);
        
        for (const auto& cell : cells) {
            m_Cells[cell].push_back(body);
        }
        
        m_BodyToCells[body.get()] = cells;
    }

    void SpatialHash::Remove(const Ref<RigidBody>& body) {
        if (!body) return;
        
        auto it = m_BodyToCells.find(body.get());
        if (it == m_BodyToCells.end()) return;
        
        for (const auto& cell : it->second) {
            auto cellIt = m_Cells.find(cell);
            if (cellIt != m_Cells.end()) {
                auto& bodies = cellIt->second;
                bodies.erase(
                    std::remove(bodies.begin(), bodies.end(), body),
                    bodies.end()
                );
                
                // Remove empty cells
                if (bodies.empty()) {
                    m_Cells.erase(cellIt);
                }
            }
        }
        
        m_BodyToCells.erase(it);
    }

    void SpatialHash::Update(const std::vector<Ref<RigidBody>>& bodies) {
        Clear();
        
        for (const auto& body : bodies) {
            Insert(body);
        }
    }

    std::vector<std::pair<Ref<RigidBody>, Ref<RigidBody>>> SpatialHash::GetPotentialPairs() const {
        std::vector<std::pair<Ref<RigidBody>, Ref<RigidBody>>> pairs;
        
        // Use a set to track unique pairs (avoid duplicates)
        std::set<std::pair<RigidBody*, RigidBody*>> uniquePairs;
        
        // For each cell
        for (const auto& [cell, bodies] : m_Cells) {
            // Check pairs within the same cell
            for (size_t i = 0; i < bodies.size(); i++) {
                for (size_t j = i + 1; j < bodies.size(); j++) {
                    RigidBody* a = bodies[i].get();
                    RigidBody* b = bodies[j].get();
                    
                    // Ensure consistent ordering
                    if (a > b) std::swap(a, b);
                    
                    uniquePairs.insert({a, b});
                }
            }
        }
        
        // Convert to result format
        pairs.reserve(uniquePairs.size());
        for (const auto& [a, b] : uniquePairs) {
            // Find the shared_ptrs for these bodies
            for (const auto& [cell, bodies] : m_Cells) {
                Ref<RigidBody> bodyA, bodyB;
                
                for (const auto& body : bodies) {
                    if (body.get() == a) bodyA = body;
                    if (body.get() == b) bodyB = body;
                }
                
                if (bodyA && bodyB) {
                    pairs.push_back({bodyA, bodyB});
                    break;
                }
            }
        }
        
        return pairs;
    }

    std::vector<Ref<RigidBody>> SpatialHash::QueryPoint(const glm::vec3& point, float radius) const {
        std::vector<Ref<RigidBody>> result;
        std::set<RigidBody*> seen;
        
        glm::vec3 min = point - glm::vec3(radius);
        glm::vec3 max = point + glm::vec3(radius);
        
        auto cells = GetCellsForAABB(min, max);
        
        for (const auto& cell : cells) {
            auto it = m_Cells.find(cell);
            if (it != m_Cells.end()) {
                for (const auto& body : it->second) {
                    if (seen.find(body.get()) == seen.end()) {
                        seen.insert(body.get());
                        result.push_back(body);
                    }
                }
            }
        }
        
        return result;
    }

    std::vector<Ref<RigidBody>> SpatialHash::QueryAABB(const glm::vec3& min, const glm::vec3& max) const {
        std::vector<Ref<RigidBody>> result;
        std::set<RigidBody*> seen;
        
        auto cells = GetCellsForAABB(min, max);
        
        for (const auto& cell : cells) {
            auto it = m_Cells.find(cell);
            if (it != m_Cells.end()) {
                for (const auto& body : it->second) {
                    if (seen.find(body.get()) == seen.end()) {
                        seen.insert(body.get());
                        
                        // Additional AABB overlap test
                        glm::vec3 bodyMin, bodyMax;
                        GetBodyAABB(body, bodyMin, bodyMax);
                        
                        // Check AABB overlap
                        if (bodyMax.x >= min.x && bodyMin.x <= max.x &&
                            bodyMax.y >= min.y && bodyMin.y <= max.y &&
                            bodyMax.z >= min.z && bodyMin.z <= max.z) {
                            result.push_back(body);
                        }
                    }
                }
            }
        }
        
        return result;
    }

    void SpatialHash::GetDebugInfo(int& maxBodiesPerCell, float& avgBodiesPerCell) const {
        maxBodiesPerCell = 0;
        int totalBodies = 0;
        
        for (const auto& [cell, bodies] : m_Cells) {
            int count = static_cast<int>(bodies.size());
            maxBodiesPerCell = std::max(maxBodiesPerCell, count);
            totalBodies += count;
        }
        
        avgBodiesPerCell = m_Cells.empty() ? 0.0f : 
            static_cast<float>(totalBodies) / static_cast<float>(m_Cells.size());
    }

    glm::ivec3 SpatialHash::WorldToCell(const glm::vec3& position) const {
        return glm::ivec3(
            static_cast<int>(std::floor(position.x / m_CellSize)),
            static_cast<int>(std::floor(position.y / m_CellSize)),
            static_cast<int>(std::floor(position.z / m_CellSize))
        );
    }

    std::vector<glm::ivec3> SpatialHash::GetCellsForAABB(const glm::vec3& min, const glm::vec3& max) const {
        std::vector<glm::ivec3> cells;
        
        glm::ivec3 minCell = WorldToCell(min);
        glm::ivec3 maxCell = WorldToCell(max);
        
        for (int x = minCell.x; x <= maxCell.x; x++) {
            for (int y = minCell.y; y <= maxCell.y; y++) {
                for (int z = minCell.z; z <= maxCell.z; z++) {
                    cells.push_back(glm::ivec3(x, y, z));
                }
            }
        }
        
        return cells;
    }

    void SpatialHash::GetBodyAABB(const Ref<RigidBody>& body, glm::vec3& outMin, glm::vec3& outMax) const {
        glm::vec3 position = body->GetPosition();
        
        if (body->HasCollisionShape()) {
            auto aabb = body->GetCollisionShape()->CalculateAABB(
                position, body->GetRotation()
            );
            outMin = aabb.Min;
            outMax = aabb.Max;
        } else {
            // Default to a small AABB around the position
            float defaultSize = 0.5f;
            outMin = position - glm::vec3(defaultSize);
            outMax = position + glm::vec3(defaultSize);
        }
    }

}}
