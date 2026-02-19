#include "OctreeAccelerator.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {
namespace Physics {

    OctreeAccelerator::OctreeAccelerator(const glm::vec3& center, float halfSize, int maxDepth)
        : m_MaxDepth(maxDepth)
    {
        m_Root = std::make_unique<Node>();
        m_Root->center = center;
        m_Root->halfSize = halfSize;
    }

    void OctreeAccelerator::Clear() {
        glm::vec3 center = m_Root->center;
        float halfSize = m_Root->halfSize;

        m_Root = std::make_unique<Node>();
        m_Root->center = center;
        m_Root->halfSize = halfSize;
    }

    void OctreeAccelerator::Insert(int particleIndex, const glm::vec3& position) {
        InsertIntoNode(*m_Root, particleIndex, position, 0);
    }

    std::vector<int> OctreeAccelerator::Query(const glm::vec3& position, float radius) const {
        std::vector<int> results;
        if (m_Root) {
            QueryNode(*m_Root, position, radius, results);
        }
        return results;
    }

    void OctreeAccelerator::Subdivide(Node& node) {
        float childHalf = node.halfSize * 0.5f;

        for (int i = 0; i < 8; ++i) {
            node.children[i] = std::make_unique<Node>();
            node.children[i]->halfSize = childHalf;

            // Compute child center based on octant index:
            // bit 0 = x direction, bit 1 = y direction, bit 2 = z direction
            glm::vec3 offset(
                (i & 1) ? childHalf : -childHalf,
                (i & 2) ? childHalf : -childHalf,
                (i & 4) ? childHalf : -childHalf
            );
            node.children[i]->center = node.center + offset;
        }

        node.isLeaf = false;

        // Re-insert existing particles into children
        for (auto& [idx, pos] : node.particles) {
            int octant = GetOctant(node, pos);
            node.children[octant]->particles.emplace_back(idx, pos);
        }
        node.particles.clear();
        node.particles.shrink_to_fit();
    }

    int OctreeAccelerator::GetOctant(const Node& node, const glm::vec3& pos) const {
        int octant = 0;
        if (pos.x >= node.center.x) octant |= 1;
        if (pos.y >= node.center.y) octant |= 2;
        if (pos.z >= node.center.z) octant |= 4;
        return octant;
    }

    void OctreeAccelerator::InsertIntoNode(Node& node, int index, const glm::vec3& pos, int depth) {
        // If this is a leaf node
        if (node.isLeaf) {
            // If under capacity or at max depth, store here
            if (static_cast<int>(node.particles.size()) < m_MaxParticlesPerNode || depth >= m_MaxDepth) {
                node.particles.emplace_back(index, pos);
                return;
            }

            // Over capacity: subdivide and redistribute
            Subdivide(node);
        }

        // Insert into the appropriate child
        int octant = GetOctant(node, pos);
        InsertIntoNode(*node.children[octant], index, pos, depth + 1);
    }

    void OctreeAccelerator::QueryNode(const Node& node, const glm::vec3& pos,
                                       float radius, std::vector<int>& results) const
    {
        // AABB-sphere overlap test for early rejection
        // Compute closest point on the node's AABB to the query sphere center
        float distSq = 0.0f;

        for (int axis = 0; axis < 3; ++axis) {
            float minBound = node.center[axis] - node.halfSize;
            float maxBound = node.center[axis] + node.halfSize;
            float v = pos[axis];

            if (v < minBound) {
                float d = minBound - v;
                distSq += d * d;
            } else if (v > maxBound) {
                float d = v - maxBound;
                distSq += d * d;
            }
        }

        // If the sphere doesn't overlap this node's AABB, skip entirely
        if (distSq > radius * radius) {
            return;
        }

        if (node.isLeaf) {
            // Check each particle in this leaf
            float radiusSq = radius * radius;
            for (const auto& [idx, particlePos] : node.particles) {
                glm::vec3 diff = particlePos - pos;
                float d2 = glm::dot(diff, diff);
                if (d2 <= radiusSq) {
                    results.push_back(idx);
                }
            }
        } else {
            // Recurse into children
            for (int i = 0; i < 8; ++i) {
                if (node.children[i]) {
                    QueryNode(*node.children[i], pos, radius, results);
                }
            }
        }
    }

}}
