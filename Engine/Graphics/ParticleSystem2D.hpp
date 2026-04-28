#pragma once

#include "../Core/Base.hpp"
#include "Texture2D.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * ParticleSystem2D — CPU-simulated 2D particle emitter with batched rendering.
     *
     * Emits particles with position, velocity, color, size, and lifetime.
     * Supports over-lifetime color/size curves, gravity, drag, and angular velocity.
     */
    struct Particle2D {
        glm::vec2 Position{0.0f};
        glm::vec2 Velocity{0.0f};
        glm::vec4 Color{1.0f};
        float Rotation = 0.0f;
        float AngularVelocity = 0.0f;
        float Size = 8.0f;
        float Life = 0.0f;
        float MaxLife = 1.0f;
        bool Alive = false;
    };

    struct ParticleEmitter2D {
        // Emission
        float EmissionRate = 20.0f;      // particles per second
        int   BurstCount = 0;            // emit N at once on Trigger()

        // Position spawn area
        glm::vec2 Position{0.0f};
        glm::vec2 SpawnBoxSize{0.0f};    // 0 = point, >0 = rect

        // Velocity
        glm::vec2 VelocityMin{-50.0f};
        glm::vec2 VelocityMax{ 50.0f};

        // Life
        float LifeMin = 0.5f;
        float LifeMax = 1.5f;

        // Size
        float SizeStart = 10.0f;
        float SizeEnd = 0.0f;

        // Color (linear RGBA)
        glm::vec4 ColorStart{1.0f, 0.8f, 0.2f, 1.0f};
        glm::vec4 ColorEnd{1.0f, 0.2f, 0.0f, 0.0f};

        // Forces
        glm::vec2 Gravity{0.0f, 200.0f};
        float Drag = 0.2f;

        // Rotation
        float RotationMin = 0.0f;
        float RotationMax = 0.0f;
        float AngularVelocityMin = 0.0f;
        float AngularVelocityMax = 0.0f;

        // Texture
        Ref<Texture2D> Texture;

        // Control
        bool Playing = true;
        bool Looping = true;
        float Duration = 2.0f;
    };

    class ParticleSystem2D {
    public:
        ParticleSystem2D(int maxParticles = 1024);

        void Update(float deltaTime);
        void Render(); // Submits to BatchRenderer2D

        // One-shot burst
        void Trigger();
        void EmitAt(const glm::vec2& pos, int count = 1);

        // Config
        ParticleEmitter2D& GetEmitter() { return m_Emitter; }
        const ParticleEmitter2D& GetEmitter() const { return m_Emitter; }

        int GetLiveCount() const { return m_LiveCount; }
        int GetMaxCount() const { return static_cast<int>(m_Particles.size()); }

    private:
        void SpawnOne();

        std::vector<Particle2D> m_Particles;
        ParticleEmitter2D m_Emitter;
        float m_EmissionAccum = 0.0f;
        float m_RunTime = 0.0f;
        int m_LiveCount = 0;
    };

}
