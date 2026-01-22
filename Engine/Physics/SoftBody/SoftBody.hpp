#pragma once

#include "ClothSolver.hpp"
#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics {

    /**
     * @brief General soft body class
     * 
     * Base class for deformable objects like cloth, ropes, etc.
     * Uses position-based dynamics (PBD) for simulation.
     */
    class SoftBody {
    public:
        SoftBody() = default;
        virtual ~SoftBody() = default;
        
        /**
         * @brief Update physics simulation
         */
        virtual void Update(float deltaTime) = 0;
        
        /**
         * @brief Reset to initial state
         */
        virtual void Reset() = 0;
        
        /**
         * @brief Apply external force
         */
        virtual void ApplyForce(const glm::vec3& force) = 0;
        
        /**
         * @brief Get positions for rendering
         */
        virtual const std::vector<glm::vec3>& GetPositions() const = 0;
        
        /**
         * @brief Get normals for rendering
         */
        virtual const std::vector<glm::vec3>& GetNormals() const = 0;
        
        /**
         * @brief Simulation parameters
         */
        glm::vec3 Gravity = glm::vec3(0, -9.81f, 0);
        float Damping = 0.99f;
        int SolverIterations = 5;
    };

    /**
     * @brief Cloth soft body - wrapper around ClothSolver
     */
    class ClothBody : public SoftBody {
    public:
        ClothBody(int width, int height, float spacing = 0.1f)
            : m_Solver(width, height, spacing * width) {
            
            // Initialize positions and normals cache
            int numParticles = width * height;
            m_Positions.resize(numParticles);
            m_Normals.resize(numParticles, glm::vec3(0, 1, 0));
            
            UpdateCache();
        }
        
        void Update(float deltaTime) override {
            m_Solver.Gravity = Gravity;
            m_Solver.Damping = Damping;
            m_Solver.SolverIterations = SolverIterations;
            
            m_Solver.Update(deltaTime);
            UpdateCache();
        }
        
        void Reset() override {
            m_Solver.Reset();
            UpdateCache();
        }
        
        void ApplyForce(const glm::vec3& force) override {
            m_Solver.ApplyForce(force);
        }
        
        const std::vector<glm::vec3>& GetPositions() const override {
            return m_Positions;
        }
        
        const std::vector<glm::vec3>& GetNormals() const override {
            return m_Normals;
        }
        
        /**
         * @brief Access underlying solver for advanced control
         */
        ClothSolver& GetSolver() { return m_Solver; }
        const ClothSolver& GetSolver() const { return m_Solver; }
        
        /**
         * @brief Apply wind
         */
        void ApplyWind(const glm::vec3& direction, float strength) {
            m_Solver.ApplyWind(direction, strength);
        }
        
        /**
         * @brief Pin/unpin corners
         */
        void PinCorner(int corner) {
            int w = m_Solver.GetWidth();
            int h = m_Solver.GetHeight();
            
            switch (corner) {
                case 0: m_Solver.PinParticle(0, 0); break;
                case 1: m_Solver.PinParticle(w - 1, 0); break;
                case 2: m_Solver.PinParticle(0, h - 1); break;
                case 3: m_Solver.PinParticle(w - 1, h - 1); break;
            }
        }
        
    private:
        void UpdateCache() {
            const auto& particles = m_Solver.GetParticles();
            for (size_t i = 0; i < particles.size(); ++i) {
                m_Positions[i] = particles[i].Position;
            }
            
            // Calculate normals
            int w = m_Solver.GetWidth();
            int h = m_Solver.GetHeight();
            
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    int idx = y * w + x;
                    glm::vec3 normal(0, 0, 0);
                    
                    // Average normals from adjacent triangles
                    if (x > 0 && y > 0) {
                        glm::vec3 v1 = m_Positions[idx - 1] - m_Positions[idx];
                        glm::vec3 v2 = m_Positions[idx - w] - m_Positions[idx];
                        normal += glm::cross(v1, v2);
                    }
                    if (x < w - 1 && y < h - 1) {
                        glm::vec3 v1 = m_Positions[idx + 1] - m_Positions[idx];
                        glm::vec3 v2 = m_Positions[idx + w] - m_Positions[idx];
                        normal += glm::cross(v1, v2);
                    }
                    
                    if (glm::length(normal) > 0.001f) {
                        m_Normals[idx] = glm::normalize(normal);
                    } else {
                        m_Normals[idx] = glm::vec3(0, 1, 0);
                    }
                }
            }
        }
        
    private:
        ClothSolver m_Solver;
        std::vector<glm::vec3> m_Positions;
        std::vector<glm::vec3> m_Normals;
    };

}}
