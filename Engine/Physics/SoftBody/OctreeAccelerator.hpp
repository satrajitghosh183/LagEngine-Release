#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <memory>
#include <utility>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Octree spatial accelerator for particle-based simulations
     *
     * Provides O(log n) spatial queries for collision detection and
     * neighbor searches in soft body / cloth simulations.
     */
    class OctreeAccelerator {
    public:
        /**
         * @brief Construct octree with given bounds
         * @param center Center of the octree volume
         * @param halfSize Half-extent of the root cube
         * @param maxDepth Maximum subdivision depth (default 5)
         */
        OctreeAccelerator(const glm::vec3& center, float halfSize, int maxDepth = 5);

        /**
         * @brief Remove all particles and reset tree
         */
        void Clear();

        /**
         * @brief Insert a particle into the octree
         * @param particleIndex Index of the particle in the external array
         * @param position World-space position of the particle
         */
        void Insert(int particleIndex, const glm::vec3& position);

        /**
         * @brief Query all particles within a sphere
         * @param position Center of the query sphere
         * @param radius Radius of the query sphere
         * @return Vector of particle indices within range
         */
        std::vector<int> Query(const glm::vec3& position, float radius) const;

    private:
        struct Node {
            glm::vec3 center;
            float halfSize;
            std::vector<std::pair<int, glm::vec3>> particles;
            std::array<std::unique_ptr<Node>, 8> children;
            bool isLeaf = true;
        };

        /**
         * @brief Subdivide a leaf node into 8 children
         */
        void Subdivide(Node& node);

        /**
         * @brief Determine which octant a position falls into
         */
        int GetOctant(const Node& node, const glm::vec3& pos) const;

        /**
         * @brief Recursively insert a particle into a node
         */
        void InsertIntoNode(Node& node, int index, const glm::vec3& pos, int depth);

        /**
         * @brief Recursively query particles in range from a node
         */
        void QueryNode(const Node& node, const glm::vec3& pos, float radius,
                        std::vector<int>& results) const;

        std::unique_ptr<Node> m_Root;
        int m_MaxDepth;
        int m_MaxParticlesPerNode = 8;
    };

}}
