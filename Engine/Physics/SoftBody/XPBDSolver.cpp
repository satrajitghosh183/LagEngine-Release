#include "XPBDSolver.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine::Physics {

    // ========== DistanceConstraint ==========
    void DistanceConstraint::Project(std::vector<XPBDParticle>& particles, float dt) {
        auto& p1 = particles[m_P1];
        auto& p2 = particles[m_P2];
        if (p1.pinned && p2.pinned) return;

        glm::vec3 diff = p1.position - p2.position;
        float dist = glm::length(diff);
        if (dist < 1e-7f) return;

        float C = dist - m_RestLength;
        float alphaTilde = Compliance / (dt * dt);
        float w = p1.invMass + p2.invMass;
        if (w < 1e-7f) return;

        float deltaLambda = -(C + alphaTilde * m_Lambda) / (w + alphaTilde);
        m_Lambda += deltaLambda;

        glm::vec3 grad = diff / dist;
        if (!p1.pinned) p1.position += p1.invMass * deltaLambda * grad;
        if (!p2.pinned) p2.position -= p2.invMass * deltaLambda * grad;
    }

    // ========== BendConstraint ==========
    void BendConstraint::Project(std::vector<XPBDParticle>& particles, float dt) {
        auto& p1 = particles[m_P1];
        auto& p2 = particles[m_P2];
        auto& p3 = particles[m_P3];
        auto& p4 = particles[m_P4];

        // Compute dihedral angle between triangles (p1,p3,p2) and (p1,p2,p4)
        glm::vec3 e = p2.position - p1.position;
        float eLen = glm::length(e);
        if (eLen < 1e-7f) return;

        glm::vec3 n1 = glm::cross(p3.position - p1.position, e);
        glm::vec3 n2 = glm::cross(e, p4.position - p1.position);
        float n1Len = glm::length(n1);
        float n2Len = glm::length(n2);
        if (n1Len < 1e-7f || n2Len < 1e-7f) return;

        n1 /= n1Len;
        n2 /= n2Len;

        float cosAngle = glm::clamp(glm::dot(n1, n2), -1.0f, 1.0f);
        float angle = std::acos(cosAngle);

        float C = angle - m_RestAngle;
        if (std::abs(C) < 1e-6f) return;

        // Simplified: apply correction along normals
        float alphaTilde = Compliance / (dt * dt);
        float w = p3.invMass + p4.invMass;
        if (w < 1e-7f) return;

        float deltaLambda = -(C + alphaTilde * m_Lambda) / (w + alphaTilde);
        m_Lambda += deltaLambda;

        if (!p3.pinned) p3.position += p3.invMass * deltaLambda * n1 * 0.5f;
        if (!p4.pinned) p4.position += p4.invMass * deltaLambda * n2 * 0.5f;
    }

    // ========== VolumeConstraint ==========
    void VolumeConstraint::Project(std::vector<XPBDParticle>& particles, float dt) {
        auto& p1 = particles[m_P1];
        auto& p2 = particles[m_P2];
        auto& p3 = particles[m_P3];
        auto& p4 = particles[m_P4];

        glm::vec3 d1 = p2.position - p1.position;
        glm::vec3 d2 = p3.position - p1.position;
        glm::vec3 d3 = p4.position - p1.position;

        float volume = glm::dot(d1, glm::cross(d2, d3)) / 6.0f;
        float C = volume - m_RestVolume;
        if (std::abs(C) < 1e-8f) return;

        // Gradients
        glm::vec3 g1 = glm::cross(d2, d3) / 6.0f;
        glm::vec3 g2 = glm::cross(d3, d1) / 6.0f;
        glm::vec3 g3 = glm::cross(d1, d2) / 6.0f;
        glm::vec3 g0 = -(g1 + g2 + g3);

        float alphaTilde = Compliance / (dt * dt);
        float w = p1.invMass * glm::dot(g0, g0) + p2.invMass * glm::dot(g1, g1)
                + p3.invMass * glm::dot(g2, g2) + p4.invMass * glm::dot(g3, g3);
        if (w < 1e-7f) return;

        float deltaLambda = -(C + alphaTilde * m_Lambda) / (w + alphaTilde);
        m_Lambda += deltaLambda;

        if (!p1.pinned) p1.position += p1.invMass * deltaLambda * g0;
        if (!p2.pinned) p2.position += p2.invMass * deltaLambda * g1;
        if (!p3.pinned) p3.position += p3.invMass * deltaLambda * g2;
        if (!p4.pinned) p4.position += p4.invMass * deltaLambda * g3;
    }

    // ========== CollisionConstraint ==========
    void CollisionConstraint::Project(std::vector<XPBDParticle>& particles, float dt) {
        auto& p = particles[m_ParticleIdx];
        if (p.pinned) return;

        float C = glm::dot(p.position, m_Normal) - m_Offset;
        if (C >= 0.0f) return; // Not penetrating

        p.position -= C * m_Normal;
    }

    // ========== XPBDSolver ==========
    void XPBDSolver::AddParticle(const glm::vec3& pos, float mass) {
        XPBDParticle p;
        p.position = pos;
        p.prevPosition = pos;
        p.velocity = glm::vec3(0.0f);
        p.invMass = (mass > 1e-7f) ? 1.0f / mass : 0.0f;
        p.pinned = false;
        m_Particles.push_back(p);
    }

    void XPBDSolver::PinParticle(int index) {
        if (index >= 0 && index < static_cast<int>(m_Particles.size())) {
            m_Particles[index].pinned = true;
            m_Particles[index].invMass = 0.0f;
        }
    }

    void XPBDSolver::UnpinParticle(int index) {
        if (index >= 0 && index < static_cast<int>(m_Particles.size())) {
            m_Particles[index].pinned = false;
            m_Particles[index].invMass = 1.0f;
        }
    }

    void XPBDSolver::AddConstraint(std::unique_ptr<XPBDConstraint> constraint) {
        m_Constraints.push_back(std::move(constraint));
    }

    void XPBDSolver::Clear() {
        m_Particles.clear();
        m_Constraints.clear();
    }

    void XPBDSolver::Solve(float dt, int substeps) {
        float subDt = dt / static_cast<float>(substeps);

        for (int s = 0; s < substeps; s++) {
            // Predict positions
            for (auto& p : m_Particles) {
                if (p.pinned) continue;
                p.velocity += m_Gravity * subDt;
                p.prevPosition = p.position;
                p.position += p.velocity * subDt;
            }

            // Reset constraint lambdas
            for (auto& c : m_Constraints) {
                c->Project(m_Particles, subDt);
            }

            // Update velocities
            for (auto& p : m_Particles) {
                if (p.pinned) continue;
                p.velocity = (p.position - p.prevPosition) / subDt;
                p.velocity *= m_Damping;
            }
        }
    }

}
