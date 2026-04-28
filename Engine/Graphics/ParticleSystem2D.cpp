#include "ParticleSystem2D.hpp"
#include "BatchRenderer2D.hpp"

#include <cmath>
#include <random>

namespace GameEngine {

    static float RandFloat(float min, float max) {
        static std::mt19937 gen{std::random_device{}()};
        std::uniform_real_distribution<float> dist(min, max);
        return dist(gen);
    }

    ParticleSystem2D::ParticleSystem2D(int maxParticles) {
        m_Particles.resize(maxParticles);
    }

    void ParticleSystem2D::Update(float deltaTime) {
        auto& e = m_Emitter;
        if (e.Playing) {
            m_RunTime += deltaTime;
            if (!e.Looping && m_RunTime > e.Duration) e.Playing = false;

            // Steady-rate emission
            if (e.Playing && e.EmissionRate > 0.0f) {
                m_EmissionAccum += deltaTime * e.EmissionRate;
                while (m_EmissionAccum >= 1.0f) {
                    SpawnOne();
                    m_EmissionAccum -= 1.0f;
                }
            }
        }

        // Advance particles
        m_LiveCount = 0;
        for (auto& p : m_Particles) {
            if (!p.Alive) continue;
            p.Life -= deltaTime;
            if (p.Life <= 0.0f) { p.Alive = false; continue; }

            p.Velocity += e.Gravity * deltaTime;
            p.Velocity *= std::exp(-e.Drag * deltaTime);
            p.Position += p.Velocity * deltaTime;
            p.Rotation += p.AngularVelocity * deltaTime;

            float t = 1.0f - (p.Life / p.MaxLife); // 0 → 1
            p.Color = glm::mix(e.ColorStart, e.ColorEnd, t);
            p.Size  = glm::mix(e.SizeStart, e.SizeEnd, t);

            m_LiveCount++;
        }
    }

    void ParticleSystem2D::Render() {
        for (auto& p : m_Particles) {
            if (!p.Alive) continue;
            // Submit to 2D batcher — drawn as a quad with color (+ optional texture)
            BatchRenderer2D::DrawQuad(
                glm::vec3(p.Position.x, p.Position.y, 0.0f),
                glm::vec2(p.Size, p.Size),
                p.Color);
        }
    }

    void ParticleSystem2D::Trigger() {
        for (int i = 0; i < m_Emitter.BurstCount; i++) SpawnOne();
    }

    void ParticleSystem2D::EmitAt(const glm::vec2& pos, int count) {
        glm::vec2 saved = m_Emitter.Position;
        m_Emitter.Position = pos;
        for (int i = 0; i < count; i++) SpawnOne();
        m_Emitter.Position = saved;
    }

    void ParticleSystem2D::SpawnOne() {
        // Find dead slot
        for (auto& p : m_Particles) {
            if (p.Alive) continue;
            const auto& e = m_Emitter;
            p.Alive = true;
            p.Position = e.Position + glm::vec2(
                RandFloat(-e.SpawnBoxSize.x * 0.5f, e.SpawnBoxSize.x * 0.5f),
                RandFloat(-e.SpawnBoxSize.y * 0.5f, e.SpawnBoxSize.y * 0.5f));
            p.Velocity = glm::vec2(
                RandFloat(e.VelocityMin.x, e.VelocityMax.x),
                RandFloat(e.VelocityMin.y, e.VelocityMax.y));
            p.MaxLife = RandFloat(e.LifeMin, e.LifeMax);
            p.Life = p.MaxLife;
            p.Size = e.SizeStart;
            p.Color = e.ColorStart;
            p.Rotation = RandFloat(e.RotationMin, e.RotationMax);
            p.AngularVelocity = RandFloat(e.AngularVelocityMin, e.AngularVelocityMax);
            return;
        }
    }

}
