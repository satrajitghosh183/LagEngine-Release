#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Single particle
     */
    struct Particle {
        glm::vec3 Position;
        glm::vec3 Velocity;
        glm::vec4 Color;
        float Size;
        float Rotation;
        float LifeRemaining;
        float LifeTime;
        
        bool Active = false;
    };

    /**
     * @brief Particle emitter
     * 
     * GPU-accelerated particle system
     */
    class ParticleEmitter {
    public:
        ParticleEmitter(uint32_t maxParticles = 1000);
        ~ParticleEmitter() = default;
        
        /**
         * @brief Update particles
         */
        void Update(float deltaTime);
        
        /**
         * @brief Emit particle
         */
        void Emit(const glm::vec3& position);
        
        /**
         * @brief Get particles for rendering
         */
        const std::vector<Particle>& GetParticles() const { return m_Particles; }
        
        /**
         * @brief Emission properties
         */
        float EmissionRate = 10.0f;        // Particles per second
        float ParticleLifetime = 2.0f;     // Seconds
        float ParticleLifetimeVariance = 0.5f;
        
        glm::vec3 StartVelocity = glm::vec3(0, 5, 0);
        glm::vec3 VelocityVariance = glm::vec3(2, 1, 2);
        
        glm::vec4 StartColor = glm::vec4(1, 1, 1, 1);
        glm::vec4 EndColor = glm::vec4(1, 1, 1, 0);
        
        float StartSize = 1.0f;
        float EndSize = 0.0f;
        
        glm::vec3 Gravity = glm::vec3(0, -9.81f, 0);
        
        bool Loop = true;
        bool PlayOnStart = true;
        
    private:
        uint32_t FindUnusedParticle();
        
    private:
        std::vector<Particle> m_Particles;
        uint32_t m_MaxParticles;
        uint32_t m_LastUsedParticle;
        float m_EmissionAccumulator;
        bool m_IsPlaying;
    };
}