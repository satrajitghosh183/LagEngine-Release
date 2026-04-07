#include "ClothSolver.hpp"
#include "../../Core/Logger.hpp"
#include <cmath>

namespace GameEngine {
namespace Physics {

    ClothSolver::ClothSolver(int width, int height, float size)
        : m_Width(width)
        , m_Height(height)
        , m_ParticleDistance(size / width) {
        
        // Create particles in grid
        m_Particles.resize(width * height);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                ClothParticle& p = m_Particles[index];
                
                p.Position = glm::vec3(
                    x * m_ParticleDistance - size * 0.5f,
                    0.0f,
                    y * m_ParticleDistance - size * 0.5f
                );
                p.OldPosition = p.Position;
                p.Mass = 1.0f;
                p.Pinned = false;
            }
        }
        
        // Create structural constraints (grid)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                
                // Right neighbor
                if (x < width - 1) {
                    int right = y * width + (x + 1);
                    m_Constraints.emplace_back(index, right, m_ParticleDistance, 0.95f);
                }
                
                // Bottom neighbor
                if (y < height - 1) {
                    int bottom = (y + 1) * width + x;
                    m_Constraints.emplace_back(index, bottom, m_ParticleDistance, 0.95f);
                }
                
                // Diagonal constraints (shear)
                if (x < width - 1 && y < height - 1) {
                    int bottomRight = (y + 1) * width + (x + 1);
                    m_Constraints.emplace_back(index, bottomRight, 
                        m_ParticleDistance * sqrt(2.0f), 0.9f);
                }
                
                if (x > 0 && y < height - 1) {
                    int bottomLeft = (y + 1) * width + (x - 1);
                    m_Constraints.emplace_back(index, bottomLeft, 
                        m_ParticleDistance * sqrt(2.0f), 0.9f);
                }
            }
        }
        
        // Create bending constraints (skip one particle)
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int index = y * width + x;
                
                if (x < width - 2) {
                    int right2 = y * width + (x + 2);
                    m_Constraints.emplace_back(index, right2, m_ParticleDistance * 2.0f, 0.8f);
                }
                
                if (y < height - 2) {
                    int bottom2 = (y + 2) * width + x;
                    m_Constraints.emplace_back(index, bottom2, m_ParticleDistance * 2.0f, 0.8f);
                }
            }
        }
        
        GE_CORE_INFO("ClothSolver created: {0}x{1} ({2} particles, {3} constraints)", 
                    width, height, m_Particles.size(), m_Constraints.size());
    }

    void ClothSolver::Update(float deltaTime) {
        // Apply forces
        for (auto& particle : m_Particles) {
            if (!particle.Pinned) {
                particle.Force += Gravity * particle.Mass;
            }
        }
        
        // Integrate
        IntegrateVerlet(deltaTime);
        
        // Solve constraints multiple times
        for (int i = 0; i < SolverIterations; i++) {
            SolveConstraints();
            SolveCollisions();
            
            if (EnableSelfCollision) {
                SolveSelfCollisions();
            }
        }
        
        // Reset forces
        for (auto& particle : m_Particles) {
            particle.Force = glm::vec3(0);
        }
    }

    void ClothSolver::Reset() {
        for (int y = 0; y < m_Height; y++) {
            for (int x = 0; x < m_Width; x++) {
                ClothParticle& p = GetParticle(x, y);
                
                p.Position = glm::vec3(
                    x * m_ParticleDistance - (m_Width * m_ParticleDistance) * 0.5f,
                    0.0f,
                    y * m_ParticleDistance - (m_Height * m_ParticleDistance) * 0.5f
                );
                p.OldPosition = p.Position;
                p.Velocity = glm::vec3(0);
                p.Force = glm::vec3(0);
            }
        }
    }

    void ClothSolver::PinParticle(int x, int y) {
        if (x >= 0 && x < m_Width && y >= 0 && y < m_Height) {
            GetParticle(x, y).Pinned = true;
        }
    }

    void ClothSolver::UnpinParticle(int x, int y) {
        if (x >= 0 && x < m_Width && y >= 0 && y < m_Height) {
            GetParticle(x, y).Pinned = false;
        }
    }

    void ClothSolver::ApplyForce(const glm::vec3& force) {
        for (auto& particle : m_Particles) {
            if (!particle.Pinned) {
                particle.Force += force;
            }
        }
    }

    void ClothSolver::ApplyWind(const glm::vec3& windDirection, float strength) {
        // Apply wind force based on triangle normals
        for (int y = 0; y < m_Height - 1; y++) {
            for (int x = 0; x < m_Width - 1; x++) {
                ClothParticle& p1 = GetParticle(x, y);
                ClothParticle& p2 = GetParticle(x + 1, y);
                ClothParticle& p3 = GetParticle(x, y + 1);
                
                glm::vec3 normal = glm::normalize(
                    glm::cross(p2.Position - p1.Position, p3.Position - p1.Position)
                );
                
                float windFactor = glm::dot(normal, glm::normalize(windDirection));
                glm::vec3 windForce = windDirection * windFactor * strength;
                
                p1.Force += windForce;
                p2.Force += windForce;
                p3.Force += windForce;
            }
        }
    }

    void ClothSolver::AddSphereCollider(const glm::vec3& center, float radius) {
        m_SphereColliders.push_back({center, radius});
    }

    void ClothSolver::AddPlaneCollider(const glm::vec3& normal, float distance) {
        m_PlaneColliders.push_back({glm::normalize(normal), distance});
    }

    ClothParticle& ClothSolver::GetParticle(int x, int y) {
        return m_Particles[y * m_Width + x];
    }

    const ClothParticle& ClothSolver::GetParticle(int x, int y) const {
        return m_Particles[y * m_Width + x];
    }

    void ClothSolver::IntegrateVerlet(float deltaTime) {
        for (auto& particle : m_Particles) {
            if (particle.Pinned) continue;
            
            // Verlet integration
            glm::vec3 temp = particle.Position;
            glm::vec3 acceleration = particle.Force / particle.Mass;
            
            particle.Position = particle.Position + 
                               (particle.Position - particle.OldPosition) * Damping + 
                               acceleration * deltaTime * deltaTime;
            
            particle.OldPosition = temp;
            
            // Update velocity for reference
            particle.Velocity = (particle.Position - particle.OldPosition) / deltaTime;
        }
    }

    void ClothSolver::SolveConstraints() {
        for (const auto& constraint : m_Constraints) {
            ClothParticle& p1 = m_Particles[constraint.ParticleA];
            ClothParticle& p2 = m_Particles[constraint.ParticleB];
            
            if (p1.Pinned && p2.Pinned) continue;
            
            glm::vec3 delta = p2.Position - p1.Position;
            float currentLength = glm::length(delta);
            
            if (currentLength < 0.0001f) continue;
            
            float difference = (currentLength - constraint.RestLength) / currentLength;
            glm::vec3 correction = delta * difference * constraint.Stiffness;
            
            if (!p1.Pinned && !p2.Pinned) {
                p1.Position += correction * 0.5f;
                p2.Position -= correction * 0.5f;
            } else if (!p1.Pinned) {
                p1.Position += correction;
            } else if (!p2.Pinned) {
                p2.Position -= correction;
            }
        }
    }

    void ClothSolver::SolveCollisions() {
        // Sphere collisions
        for (const auto& sphere : m_SphereColliders) {
            for (auto& particle : m_Particles) {
                if (particle.Pinned) continue;
                
                glm::vec3 delta = particle.Position - sphere.Center;
                float distance = glm::length(delta);
                
                if (distance < sphere.Radius) {
                    glm::vec3 normal = glm::normalize(delta);
                    particle.Position = sphere.Center + normal * sphere.Radius;
                }
            }
        }
        
        // Plane collisions
        for (const auto& plane : m_PlaneColliders) {
            for (auto& particle : m_Particles) {
                if (particle.Pinned) continue;
                
                float distance = glm::dot(particle.Position, plane.Normal) - plane.Distance;
                
                if (distance < 0) {
                    particle.Position -= plane.Normal * distance;
                }
            }
        }
    }

    void ClothSolver::SolveSelfCollisions() {
        // Simple spatial hashing or brute force
        const float collisionRadius = m_ParticleDistance * 0.5f;
        
        for (size_t i = 0; i < m_Particles.size(); i++) {
            for (size_t j = i + 1; j < m_Particles.size(); j++) {
                // Skip adjacent particles
                if (abs(static_cast<int>(i) - static_cast<int>(j)) <= m_Width + 1) continue;
                
                ClothParticle& p1 = m_Particles[i];
                ClothParticle& p2 = m_Particles[j];
                
                glm::vec3 delta = p2.Position - p1.Position;
                float distance = glm::length(delta);
                
                if (distance < collisionRadius && distance > 0.0001f) {
                    glm::vec3 normal = delta / distance;
                    float correction = (collisionRadius - distance) * 0.5f;
                    
                    if (!p1.Pinned && !p2.Pinned) {
                        p1.Position -= normal * correction;
                        p2.Position += normal * correction;
                    } else if (!p1.Pinned) {
                        p1.Position -= normal * correction * 2.0f;
                    } else if (!p2.Pinned) {
                        p2.Position += normal * correction * 2.0f;
                    }
                }
            }
        }
    }

}}