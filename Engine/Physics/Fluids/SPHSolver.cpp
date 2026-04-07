#include "SPHSolver.hpp"
#include "../../Core/Logger.hpp"
#include "../../Core/MathUtils.hpp"
#include <cmath>

namespace GameEngine {
namespace Physics {

    SPHSolver::SPHSolver(int maxParticles)
        : m_MaxParticles(maxParticles)
        , m_GridCellSize(1) {
        
        m_Particles.reserve(maxParticles);
        
        // Precompute kernel constants
        float h = SmoothingRadius;
        float h2 = h * h;
        float h3 = h2 * h;
        float h6 = h3 * h3;
        float h9 = h6 * h3;
        
        m_Poly6Constant = 315.0f / (64.0f * Math::PI * h9);
        m_SpikyConstant = -45.0f / (Math::PI * h6);
        m_ViscosityConstant = 45.0f / (Math::PI * h6);
        
        GE_CORE_INFO("SPHSolver created (max {0} particles)", maxParticles);
    }

    void SPHSolver::Update(float deltaTime) {
        if (m_Particles.empty()) return;
        
        // Fixed timestep for stability
        const float fixedDt = 0.001f;
        static float accumulator = 0.0f;
        accumulator += deltaTime;
        
        while (accumulator >= fixedDt) {
            BuildSpatialHash();
            ComputeDensityPressure();
            ComputeForces();
            Integrate(fixedDt);
            HandleBoundaries();
            
            accumulator -= fixedDt;
        }
    }

    void SPHSolver::AddParticle(const glm::vec3& position, const glm::vec3& velocity) {
        if (m_Particles.size() >= m_MaxParticles) {
            GE_CORE_WARN("Max particles reached!");
            return;
        }
        
        FluidParticle particle;
        particle.Position = position;
        particle.Velocity = velocity;
        m_Particles.push_back(particle);
    }

    void SPHSolver::Clear() {
        m_Particles.clear();
        m_SpatialHash.clear();
    }

    void SPHSolver::ComputeDensityPressure() {
        for (size_t i = 0; i < m_Particles.size(); i++) {
            FluidParticle& pi = m_Particles[i];
            pi.Density = 0.0f;
            
            auto neighbors = GetNearbyParticles(i);
            
            for (int j : neighbors) {
                const FluidParticle& pj = m_Particles[j];
                glm::vec3 rij = pj.Position - pi.Position;
                float r = glm::length(rij);
                
                if (r < SmoothingRadius) {
                    pi.Density += ParticleMass * Poly6Kernel(r);
                }
            }
            
            // Compute pressure from density (Tait equation)
            pi.Pressure = GasConstant * (pi.Density - RestDensity);
        }
    }

    void SPHSolver::ComputeForces() {
        for (size_t i = 0; i < m_Particles.size(); i++) {
            FluidParticle& pi = m_Particles[i];
            glm::vec3 pressureForce(0);
            glm::vec3 viscosityForce(0);
            
            auto neighbors = GetNearbyParticles(i);
            
            for (int j : neighbors) {
                if (i == j) continue;
                
                const FluidParticle& pj = m_Particles[j];
                glm::vec3 rij = pj.Position - pi.Position;
                float r = glm::length(rij);
                
                if (r < SmoothingRadius && r > 0.0001f) {
                    // Pressure force (symmetric formulation)
                    glm::vec3 gradient = SpikyKernelGradient(rij);
                    float pressureTerm = (pi.Pressure + pj.Pressure) / (2.0f * pj.Density);
                    pressureForce += -ParticleMass * pressureTerm * gradient;
                    
                    // Viscosity force
                    glm::vec3 velocityDiff = pj.Velocity - pi.Velocity;
                    float laplacian = ViscosityKernelLaplacian(r);
                    viscosityForce += Viscosity * ParticleMass * (velocityDiff / pj.Density) * laplacian;
                }
            }
            
            // Total force
            pi.Force = pressureForce + viscosityForce + Gravity * pi.Density;
        }
    }

    void SPHSolver::Integrate(float deltaTime) {
        for (auto& particle : m_Particles) {
            // Forward Euler integration
            glm::vec3 acceleration = particle.Force / particle.Density;
            particle.Velocity += acceleration * deltaTime;
            particle.Position += particle.Velocity * deltaTime;
        }
    }

    void SPHSolver::HandleBoundaries() {
        for (auto& particle : m_Particles) {
            // Box boundaries with damping
            if (particle.Position.x < BoundsMin.x) {
                particle.Position.x = BoundsMin.x;
                particle.Velocity.x *= -BoundsDamping;
            }
            if (particle.Position.x > BoundsMax.x) {
                particle.Position.x = BoundsMax.x;
                particle.Velocity.x *= -BoundsDamping;
            }
            
            if (particle.Position.y < BoundsMin.y) {
                particle.Position.y = BoundsMin.y;
                particle.Velocity.y *= -BoundsDamping;
            }
            if (particle.Position.y > BoundsMax.y) {
                particle.Position.y = BoundsMax.y;
                particle.Velocity.y *= -BoundsDamping;
            }
            
            if (particle.Position.z < BoundsMin.z) {
                particle.Position.z = BoundsMin.z;
                particle.Velocity.z *= -BoundsDamping;
            }
            if (particle.Position.z > BoundsMax.z) {
                particle.Position.z = BoundsMax.z;
                particle.Velocity.z *= -BoundsDamping;
            }
        }
    }

    float SPHSolver::Poly6Kernel(float r) {
        if (r >= SmoothingRadius) return 0.0f;
        
        float h2_r2 = SmoothingRadius * SmoothingRadius - r * r;
        return m_Poly6Constant * h2_r2 * h2_r2 * h2_r2;
    }

    glm::vec3 SPHSolver::SpikyKernelGradient(const glm::vec3& r) {
        float len = glm::length(r);
        if (len >= SmoothingRadius || len < 0.0001f) return glm::vec3(0);
        
        float h_r = SmoothingRadius - len;
        return m_SpikyConstant * h_r * h_r * (r / len);
    }

    float SPHSolver::ViscosityKernelLaplacian(float r) {
        if (r >= SmoothingRadius) return 0.0f;
        
        return m_ViscosityConstant * (SmoothingRadius - r);
    }

    void SPHSolver::BuildSpatialHash() {
        m_SpatialHash.clear();
        
        for (size_t i = 0; i < m_Particles.size(); i++) {
            int hash = HashPosition(m_Particles[i].Position);
            m_SpatialHash[hash].push_back(i);
        }
    }

    std::vector<int> SPHSolver::GetNearbyParticles(int particleIndex) {
        std::vector<int> neighbors;
        const auto& particle = m_Particles[particleIndex];
        
        // Check 27 neighboring cells (3x3x3 grid)
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    glm::vec3 offset(dx, dy, dz);
                    int hash = HashPosition(particle.Position + offset * SmoothingRadius);
                    
                    auto it = m_SpatialHash.find(hash);
                    if (it != m_SpatialHash.end()) {
                        neighbors.insert(neighbors.end(), it->second.begin(), it->second.end());
                    }
                }
            }
        }
        
        return neighbors;
    }

    int SPHSolver::HashPosition(const glm::vec3& position) {
        int x = static_cast<int>(floor(position.x / SmoothingRadius));
        int y = static_cast<int>(floor(position.y / SmoothingRadius));
        int z = static_cast<int>(floor(position.z / SmoothingRadius));
        
        // Simple hash function
        return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
    }

}}