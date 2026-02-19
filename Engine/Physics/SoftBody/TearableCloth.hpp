#pragma once

#include "OctreeAccelerator.hpp"
#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Cloth simulation with tearing support
     *
     * Extends basic cloth with the ability to tear when constraints
     * are stretched beyond a threshold. Uses an OctreeAccelerator for
     * efficient self-collision detection.
     */
    class TearableCloth {
    public:
        struct Particle {
            glm::vec3 position;
            glm::vec3 prevPosition;
            glm::vec3 velocity;
            glm::vec3 acceleration;
            float invMass = 1.0f;
            bool pinned = false;
        };

        struct DistanceConstraint {
            int indexA;
            int indexB;
            float restLength;
            bool alive = true;
        };

        TearableCloth() = default;
        ~TearableCloth() = default;

        /**
         * @brief Initialize a rectangular cloth grid
         * @param width Physical width of the cloth
         * @param height Physical height of the cloth
         * @param resX Number of particles along X axis
         * @param resY Number of particles along Y axis
         */
        void Initialize(float width, float height, int resX, int resY);

        /**
         * @brief Advance the simulation
         * @param dt Delta time
         * @param iterations Number of constraint solver iterations
         */
        void Update(float dt, int iterations = 4);

        /**
         * @brief Get particle positions (for rendering)
         */
        const std::vector<glm::vec3>& GetPositions() const { return m_PositionCache; }

        /**
         * @brief Get triangle indices (updated when tears occur)
         */
        const std::vector<uint32_t>& GetIndices() const { return m_IndexCache; }

        /**
         * @brief Pin a particle so it doesn't move
         */
        void PinParticle(int index);

        /**
         * @brief Unpin a particle
         */
        void UnpinParticle(int index);

        /**
         * @brief Get particle count
         */
        int GetParticleCount() const { return static_cast<int>(m_Particles.size()); }

        /**
         * @brief Get resolution
         */
        int GetResX() const { return m_ResX; }
        int GetResY() const { return m_ResY; }

        /**
         * @brief Simulation parameters
         */
        float Stiffness = 1.0f;
        float Damping = 0.99f;
        float TearingThreshold = 0.0f;  // 0 = no tearing; ratio above rest length
        glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
        float SelfCollisionRadius = 0.05f;
        float SelfCollisionStiffness = 0.5f;

    private:
        /**
         * @brief Solve distance constraints with optional tearing
         */
        void SolveConstraints();

        /**
         * @brief Handle self-collision using the octree
         */
        void SolveSelfCollision();

        /**
         * @brief Rebuild the index buffer after constraints are torn
         */
        void RebuildIndices();

        /**
         * @brief Update the position cache from particles
         */
        void UpdatePositionCache();

        std::vector<Particle> m_Particles;
        std::vector<DistanceConstraint> m_Constraints;

        // Cached output data
        std::vector<glm::vec3> m_PositionCache;
        std::vector<uint32_t> m_IndexCache;

        // Grid dimensions
        int m_ResX = 0;
        int m_ResY = 0;
        float m_Width = 0.0f;
        float m_Height = 0.0f;

        bool m_IndicesDirty = true;

        // Self-collision octree
        Scope<OctreeAccelerator> m_Octree;
    };

}}
