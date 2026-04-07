#include "ParticleEmitter.hpp"
#include "../Core/RandomUtils.hpp"
#include <algorithm>

namespace GameEngine {

    ParticleEmitter::ParticleEmitter(uint32_t maxParticles)
        : m_MaxParticles(maxParticles)
        , m_LastUsedParticle(0)
        , m_EmissionAccumulator(0.0f)
        , m_IsPlaying(false) {
        
        m_Particles.resize(maxParticles);
    }

    void ParticleEmitter::Update(float deltaTime) {
        if (!m_IsPlaying && !PlayOnStart) return;
        
        if (PlayOnStart) {
            m_IsPlaying = true;
        }
        
        // Update existing particles
        for (auto& particle : m_Particles) {
            if (!particle.Active) continue;
            
            particle.LifeRemaining -= deltaTime;
            
            if (particle.LifeRemaining <= 0) {
                particle.Active = false;
                continue;
            }
            
            // Update physics
            particle.Velocity += Gravity * deltaTime;
            particle.Position += particle.Velocity * deltaTime;
            
            // Interpolate color
            float t = 1.0f - (particle.LifeRemaining / particle.LifeTime);
            particle.Color = glm::mix(StartColor, EndColor, t);
            
            // Interpolate size
            particle.Size = glm::mix(StartSize, EndSize, t);
        }
        
        // Emit new particles
        if (Loop || m_IsPlaying) {
            m_EmissionAccumulator += EmissionRate * deltaTime;
            
            int particlesToEmit = static_cast<int>(m_EmissionAccumulator);
            m_EmissionAccumulator -= particlesToEmit;
            
            for (int i = 0; i < particlesToEmit; i++) {
                Emit(glm::vec3(0));  // Position should be set by component
            }
        }
    }

    void ParticleEmitter::Emit(const glm::vec3& position) {
        uint32_t index = FindUnusedParticle();
        if (index >= m_MaxParticles) return;
        
        Particle& particle = m_Particles[index];
        
        particle.Active = true;
        particle.Position = position;
        
        // Random velocity
        particle.Velocity = StartVelocity + glm::vec3(
            Random::Range(-VelocityVariance.x, VelocityVariance.x),
            Random::Range(-VelocityVariance.y, VelocityVariance.y),
            Random::Range(-VelocityVariance.z, VelocityVariance.z)
        );
        
        particle.Color = StartColor;
        particle.Size = StartSize;
        particle.Rotation = Random::Range(0.0f, 360.0f);
        
        particle.LifeTime = ParticleLifetime + Random::Range(-ParticleLifetimeVariance, ParticleLifetimeVariance);
        particle.LifeRemaining = particle.LifeTime;
    }

    uint32_t ParticleEmitter::FindUnusedParticle() {
        // Search from last used
        for (uint32_t i = m_LastUsedParticle; i < m_MaxParticles; i++) {
            if (!m_Particles[i].Active) {
                m_LastUsedParticle = i;
                return i;
            }
        }
        
        // Search from beginning
        for (uint32_t i = 0; i < m_LastUsedParticle; i++) {
            if (!m_Particles[i].Active) {
                m_LastUsedParticle = i;
                return i;
            }
        }
        
        // All particles in use
        return m_MaxParticles;
    }
}