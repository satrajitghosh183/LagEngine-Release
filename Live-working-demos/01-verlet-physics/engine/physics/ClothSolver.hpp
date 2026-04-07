// #pragma once
// #include <vector>
// #include "Particle3D.hpp"
// #include "Constraint3D.hpp"
// #include <glm/gtc/matrix_transform.hpp>

// namespace engine::physics {

//     /**
//      * @brief Cloth solver: manages particle grid and constraints
//      *        to simulate cloth dynamics using constraint satisfaction.
//      */
//     class ClothSolver {
//     public:
//         std::vector<Particle3D> particles;
//         std::vector<Constraint3D> constraints;
        

//         ClothSolver() = default;

//         /**
//          * @brief Update all particles using Verlet integration.
//          * @param dt Time step
//          * @param acceleration Global acceleration (e.g., gravity)
//          */
//         void update(float dt, const glm::vec3& acceleration);

//         /**
//          * @brief Apply all constraints multiple times for stability.
//          * @param iterations Number of solver iterations (e.g., 10–20)
//          */
//         void solve(int iterations = 20);

//         /**
//          * @brief Add a particle to the cloth grid.
//          */
//         void addParticle(const glm::vec3& position, const glm::vec3& velocity = {0.0f, 0.0f, 0.0f}, bool locked = false);

//         /**
//          * @brief Add a constraint between two particle indices.
//          */
//         void addConstraint(int i1, int i2, float restLength);
//     };

// } // namespace engine::physics


// engine/physics/ClothSolver.hpp
#pragma once
#include <vector>
#include "Particle3D.hpp"
#include "Constraint3D.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace engine::physics {

    /**
     * @brief 3D cloth solver: manages particle grid and constraints
     *        to simulate cloth dynamics using constraint satisfaction
     */
    class ClothSolver {
    public:
        std::vector<Particle3D> particles;
        std::vector<Constraint3D> constraints;
        
        // Physics parameters
        glm::vec3 gravity = {0.0f, -9.81f, 0.0f};
        int solverIterations = 20;
        float timeScale = 1.0f;
        float damping = 0.98f;

        ClothSolver() = default;

        /**
         * @brief Update all particles using Verlet integration
         * @param dt Time step in seconds
         * @param acceleration Global acceleration (overrides gravity if provided)
         */
        void update(float dt, const glm::vec3& acceleration = glm::vec3(0.0f));

        /**
         * @brief Apply all constraints multiple times for stability
         * @param iterations Number of solver iterations (overrides solverIterations if provided)
         */
        void solve(int iterations = 0);

        /**
         * @brief Add a particle to the cloth grid
         * @param position Initial position
         * @param velocity Initial velocity
         * @param locked Whether the particle is fixed in place
         * @return Index of the created particle
         */
        int addParticle(const glm::vec3& position, 
                      const glm::vec3& velocity = {0.0f, 0.0f, 0.0f}, 
                      bool locked = false);

        /**
         * @brief Add a constraint between two particle indices
         * @param i1 Index of first particle
         * @param i2 Index of second particle
         * @param restLength Rest length (relaxed state)
         * @param stiffness Constraint stiffness factor (0-1)
         * @return Index of the created constraint
         */
        int addConstraint(int i1, int i2, float restLength, float stiffness = 1.0f);
        
        /**
         * @brief Create a grid of particles and constraints
         * @param width Number of particles in width
         * @param height Number of particles in height
         * @param spacing Distance between particles
         * @param position Position of top-left corner
         * @param fixTopRow Whether to fix the top row of particles
         * @return Number of particles created
         */
        int createGrid(int width, int height, float spacing, 
                     const glm::vec3& position, bool fixTopRow = true);
        
        /**
         * @brief Apply an impulse force to a particle
         * @param index Particle index
         * @param impulse Impulse force vector
         */
        void applyImpulse(int index, const glm::vec3& impulse);
    };

} // namespace engine::physics