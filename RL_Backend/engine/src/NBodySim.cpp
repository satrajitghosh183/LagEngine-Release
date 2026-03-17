#include "rldemo/NBodySim.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <cmath>

namespace rldemo {

NBodySim::NBodySim(uint32_t maxBodies) : m_MaxBodies(maxBodies) {
    m_Bodies.reserve(maxBodies);
}

void NBodySim::Resize(uint32_t count) {
    if (count > m_MaxBodies) count = m_MaxBodies;
    uint32_t oldSize = static_cast<uint32_t>(m_Bodies.size());
    m_Bodies.resize(count);
    if (count > oldSize) {
        std::mt19937 rng(oldSize * 7 + 13);
        std::uniform_real_distribution<float> posDist(-kBoundsHalf * 0.8f, kBoundsHalf * 0.8f);
        std::uniform_real_distribution<float> velDist(-0.3f, 0.3f);
        for (uint32_t i = oldSize; i < count; ++i) {
            auto& b = m_Bodies[i];
            b.pos = glm::vec3(posDist(rng), posDist(rng), posDist(rng));
            b.vel = glm::vec3(velDist(rng), velDist(rng), velDist(rng));
            b.acc = glm::vec3(0.f);
            b.mass = 0.5f + (std::abs(posDist(rng)) / kBoundsHalf) * 1.5f;
            b.radius = 0.3f + b.mass * 0.2f;
        }
    }
}

void NBodySim::RandomizePositionsAndVelocities(float boxHalf, float speedScale) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> posDist(-boxHalf, boxHalf);
    std::uniform_real_distribution<float> velDist(-0.5f * speedScale, 0.5f * speedScale);
    for (auto& b : m_Bodies) {
        b.pos = glm::vec3(posDist(rng), posDist(rng), posDist(rng));
        b.vel = glm::vec3(velDist(rng), velDist(rng), velDist(rng));
        b.acc = glm::vec3(0.f);
        b.mass = 0.5f + (std::abs(posDist(rng)) / boxHalf) * 1.5f;
        b.radius = 0.3f + b.mass * 0.2f;
    }
}

void NBodySim::Step(float dt) {
    computeGravity();
    resolveCollisions(dt);
    integrate(dt);
}

void NBodySim::computeGravity() {
    const size_t count = m_Bodies.size();
    for (size_t i = 0; i < count; ++i) {
        m_Bodies[i].acc = ComputeGravityForBody(static_cast<int>(i), m_Bodies);
    }
}

glm::vec3 NBodySim::ComputeGravityForBody(int i, const std::vector<NBody>& bodies) const {
    glm::vec3 acc(0.f);
    const glm::vec3 pi = bodies[i].pos;
    const size_t count = bodies.size();
    for (size_t j = 0; j < count; ++j) {
        if (j == static_cast<size_t>(i)) continue;
        glm::vec3 d = bodies[j].pos - pi;
        float distSq = glm::dot(d, d) + kSofteningSq;
        float invDist = 1.f / std::sqrt(distSq);
        float f = kGravityConst * bodies[j].mass * invDist * invDist * invDist;
        acc += d * f;
    }
    return acc;
}

void NBodySim::ComputeGravityRange(int start, int end, std::vector<NBody>& bodies) const {
    for (int i = start; i < end; ++i) {
        bodies[i].acc = ComputeGravityForBody(i, bodies);
    }
}

void NBodySim::resolveCollisions(float /*dt*/) {
    const size_t count = m_Bodies.size();
    for (size_t i = 0; i < count; ++i) {
        NBody& a = m_Bodies[i];
        if (a.pos.y - a.radius < kFloorY) {
            a.pos.y = kFloorY + a.radius;
            a.vel.y = -a.vel.y * kRestitution;
        }
        if (a.pos.x < -kBoundsHalf + a.radius) { a.pos.x = -kBoundsHalf + a.radius; a.vel.x = -a.vel.x * kRestitution; }
        if (a.pos.x > kBoundsHalf - a.radius) { a.pos.x = kBoundsHalf - a.radius; a.vel.x = -a.vel.x * kRestitution; }
        if (a.pos.z < -kBoundsHalf + a.radius) { a.pos.z = -kBoundsHalf + a.radius; a.vel.z = -a.vel.z * kRestitution; }
        if (a.pos.z > kBoundsHalf - a.radius) { a.pos.z = kBoundsHalf - a.radius; a.vel.z = -a.vel.z * kRestitution; }
    }
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            NBody& a = m_Bodies[i];
            NBody& b = m_Bodies[j];
            glm::vec3 delta = b.pos - a.pos;
            float dist = glm::length(delta);
            float sumR = a.radius + b.radius;
            if (dist < sumR && dist > 1e-6f) {
                glm::vec3 normal = delta / dist;
                float overlap = sumR - dist;
                float totalMass = a.mass + b.mass;
                a.pos -= normal * (overlap * (b.mass / totalMass));
                b.pos += normal * (overlap * (a.mass / totalMass));
                float vRel = glm::dot(a.vel - b.vel, normal);
                if (vRel < 0.f) {
                    glm::vec3 impulse = normal * (vRel * kRestitution);
                    a.vel -= impulse * (b.mass / totalMass);
                    b.vel += impulse * (a.mass / totalMass);
                }
            }
        }
    }
}

void NBodySim::DetectAndResolveCollisionsRange(int start, int end, std::vector<NBody>& bodies) const {
    const int count = static_cast<int>(bodies.size());
    for (int i = start; i < end; ++i) {
        NBody& a = bodies[i];
        if (a.pos.y - a.radius < kFloorY) {
            a.pos.y = kFloorY + a.radius;
            a.vel.y = -a.vel.y * kRestitution;
        }
        if (a.pos.x < -kBoundsHalf + a.radius) { a.pos.x = -kBoundsHalf + a.radius; a.vel.x = -a.vel.x * kRestitution; }
        if (a.pos.x > kBoundsHalf - a.radius) { a.pos.x = kBoundsHalf - a.radius; a.vel.x = -a.vel.x * kRestitution; }
        if (a.pos.z < -kBoundsHalf + a.radius) { a.pos.z = -kBoundsHalf + a.radius; a.vel.z = -a.vel.z * kRestitution; }
        if (a.pos.z > kBoundsHalf - a.radius) { a.pos.z = kBoundsHalf - a.radius; a.vel.z = -a.vel.z * kRestitution; }
        for (int j = 0; j < count; ++j) {
            if (j == i) continue;
            const NBody& b = bodies[j];
            glm::vec3 delta = b.pos - a.pos;
            float dist = glm::length(delta);
            float sumR = a.radius + b.radius;
            if (dist < sumR && dist > 1e-6f) {
                glm::vec3 normal = delta / dist;
                float overlap = sumR - dist;
                float totalMass = a.mass + b.mass;
                a.pos -= normal * (overlap * (b.mass / totalMass));
                float vRel = glm::dot(a.vel - b.vel, normal);
                if (vRel < 0.f) {
                    glm::vec3 impulse = normal * (vRel * kRestitution);
                    a.vel -= impulse * (b.mass / totalMass);
                }
            }
        }
    }
}

void NBodySim::integrate(float dt) {
    for (auto& b : m_Bodies) {
        b.vel += b.acc * dt;
        b.pos += b.vel * dt;
    }
}

void NBodySim::IntegrateRange(int start, int end, std::vector<NBody>& bodies, float dt) const {
    for (int i = start; i < end; ++i) {
        bodies[i].vel += bodies[i].acc * dt;
        bodies[i].pos += bodies[i].vel * dt;
    }
}

void NBodySim::BuildInstanceDataRange(int start, int end, const std::vector<NBody>& bodies,
                                      std::vector<float>& outModelColors, int floatsPerInstance) {
    for (int i = start; i < end && i < static_cast<int>(bodies.size()); ++i) {
        const NBody& b = bodies[i];
        glm::mat4 model = glm::translate(glm::mat4(1.f), b.pos);
        model = glm::scale(model, glm::vec3(b.radius * 2.f));
        if (floatsPerInstance >= 20) {
            for (int col = 0; col < 4; ++col)
                for (int row = 0; row < 4; ++row)
                    outModelColors.push_back(model[col][row]);
            float hue = (i % 100) / 100.f;
            outModelColors.push_back(hue);
            outModelColors.push_back(0.5f);
            outModelColors.push_back(0.8f);
            outModelColors.push_back(1.f);
        }
    }
}

} // namespace rldemo
