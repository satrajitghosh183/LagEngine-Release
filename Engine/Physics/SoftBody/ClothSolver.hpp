#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics {

    /**
     * @brief Cloth particle
     */
    struct ClothParticle {
        glm::vec3 Position;
        glm::vec3 OldPosition;
        glm::vec3 Velocity;
        glm::vec3 Force;
        float Mass;
        bool Pinned;
        
        ClothParticle() 
            : Position(0), OldPosition(0), Velocity(0), Force(0), Mass(1.0f), Pinned(false) {}
    };

    /**
     * @brief Cloth constraint (spring between particles)
     */
    struct ClothConstraint {
        int ParticleA;
        int ParticleB;
        float RestLength;
        float Stiffness;
        
        ClothConstraint(int a, int b, float length, float stiffness = 0.95f)
            : ParticleA(a), ParticleB(b), RestLength(length), Stiffness(stiffness) {}
    };

    /**
     * @brief Cloth solver using Position-Based Dynamics (PBD)
     * 
     * Features:
     * - Verlet integration
     * - Distance constraints
     * - Collision detection with spheres/planes
     * - Wind forces
     * - Self-collision (optional)
     */
    class ClothSolver {
    public:
        ClothSolver(int width, int height, float size = 1.0f);
        ~ClothSolver() = default;
        
        /**
         * @brief Update simulation
         */
        void Update(float deltaTime);
        
        /**
         * @brief Reset cloth
         */
        void Reset();
        
        /**
         * @brief Pin particle at position
         */
        void PinParticle(int x, int y);
        
        /**
         * @brief Unpin particle
         */
        void UnpinParticle(int x, int y);
        
        /**
         * @brief Apply force to all particles
         */
        void ApplyForce(const glm::vec3& force);
        
        /**
         * @brief Apply wind force
         */
        void ApplyWind(const glm::vec3& windDirection, float strength);
        
        /**
         * @brief Add sphere collision object
         */
        void AddSphereCollider(const glm::vec3& center, float radius);
        
        /**
         * @brief Add plane collision object
         */
        void AddPlaneCollider(const glm::vec3& normal, float distance);
        
        /**
         * @brief Get particles
         */
        const std::vector<ClothParticle>& GetParticles() const { return m_Particles; }
        std::vector<ClothParticle>& GetParticles() { return m_Particles; }
        
        /**
         * @brief Get particle at grid position
         */
        ClothParticle& GetParticle(int x, int y);
        const ClothParticle& GetParticle(int x, int y) const;
        
        /**
         * @brief Get dimensions
         */
        int GetWidth() const { return m_Width; }
        int GetHeight() const { return m_Height; }
        
        /**
         * @brief Simulation parameters
         */
        glm::vec3 Gravity = glm::vec3(0, -9.81f, 0);
        float Damping = 0.99f;
        int SolverIterations = 5;
        bool EnableSelfCollision = false;
        
    private:
        void IntegrateVerlet(float deltaTime);
        void SolveConstraints();
        void SolveCollisions();
        void SolveSelfCollisions();
        
    private:
        int m_Width;
        int m_Height;
        float m_ParticleDistance;
        
        std::vector<ClothParticle> m_Particles;
        std::vector<ClothConstraint> m_Constraints;
        
        struct SphereCollider {
            glm::vec3 Center;
            float Radius;
        };
        
        struct PlaneCollider {
            glm::vec3 Normal;
            float Distance;
        };
        
        std::vector<SphereCollider> m_SphereColliders;
        std::vector<PlaneCollider> m_PlaneColliders;
    };

}}