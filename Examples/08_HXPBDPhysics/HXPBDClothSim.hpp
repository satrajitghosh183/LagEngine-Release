#pragma once
/**
 * HXPBDClothSim.hpp
 *
 * XPBD cloth simulation extracted from old_code/feather/core/physics/HXPbdSolver.h.
 *
 * The velocity integration + constraint-projection algorithm is copied 1:1 from
 * HXPbdSolver::solve() (feather, lines 55-305).  The only change: particle
 * state is stored in plain std::vector<glm::vec3> instead of feather's Mesh-
 * coupled PhysicsBody, so this compiles without the feather scene graph.
 *
 * Algorithm reference: Macklin et al., "XPBD: Position-Based Simulation of
 * Compliant Constrained Dynamics" (2016).
 */

#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ---------------------------------------------------------------------------
// Particle — mirrors feather/core/physics/Particle.h fields used by solver
// ---------------------------------------------------------------------------
struct XPBDParticle {
    glm::vec3 position         = glm::vec3(0.f);
    glm::vec3 predictedPosition= glm::vec3(0.f);
    glm::vec3 velocity         = glm::vec3(0.f);
    float     mass             = 1.0f;
    float     invMass          = 1.0f;   // 0 = fixed/infinite mass
    bool      isFixed          = false;

    // External forces accumulated per step (cleared after each full step — HXPbdSolver::solve line 299)
    std::vector<glm::vec3> externalForces;

    glm::vec3 resultingExternalForce() const {
        glm::vec3 total(0.f);
        for (auto& f : externalForces) total += f;
        return total;
    }
};

// ---------------------------------------------------------------------------
// DistanceConstraint — mirrors feather/core/physics/DistanceConstraint.h
// ---------------------------------------------------------------------------
struct XPBDDistanceConstraint {
    int   p0, p1;
    float restLength;
    float compliance;   // alpha in XPBD; 0 = rigid

    // Solve — 1:1 with DistanceConstraint::solve(dt) in feather
    void solve(std::vector<XPBDParticle>& particles, float dt) const {
        auto& pa = particles[p0];
        auto& pb = particles[p1];
        if (pa.invMass == 0.f && pb.invMass == 0.f) return;

        glm::vec3 diff = pa.predictedPosition - pb.predictedPosition;
        float len = glm::length(diff);
        if (len < 1e-6f) return;

        float C = len - restLength;
        float alpha = compliance / (dt * dt);
        float denom = pa.invMass + pb.invMass + alpha;
        if (denom < 1e-9f) return;

        float lambda = -C / denom;
        glm::vec3 grad = diff / len;

        pa.predictedPosition += pa.invMass * lambda * grad;
        pb.predictedPosition -= pb.invMass * lambda * grad;
    }
};

// ---------------------------------------------------------------------------
// FixedConstraint — mirrors feather/core/physics/FixedConstraint.h
// ---------------------------------------------------------------------------
struct XPBDFixedConstraint {
    int       pIdx;
    glm::vec3 anchor;

    void solve(std::vector<XPBDParticle>& particles, float /*dt*/) const {
        particles[pIdx].predictedPosition = anchor;
    }
};

// ---------------------------------------------------------------------------
// HXPBDClothSim — a cloth grid that runs the XPBD solve loop
// ---------------------------------------------------------------------------
class HXPBDClothSim {
public:
    int gridW, gridH;

    std::vector<XPBDParticle>         particles;
    std::vector<XPBDDistanceConstraint> distConstraints;
    std::vector<XPBDFixedConstraint>    fixedConstraints;

    int   iterations  = 4;     // sub-steps — HXPbdSolver default _iterations = 4
    float damping     = 0.999f; // velocity damping — HXPbdSolver line 81

    // External acceleration fields (gravity, wind)
    std::vector<glm::vec3> accelerations;

    HXPBDClothSim(int w, int h, float spacing, glm::vec3 origin)
        : gridW(w), gridH(h)
    {
        // Build particle grid
        particles.resize(w * h);
        for (int j = 0; j < h; ++j) {
            for (int i = 0; i < w; ++i) {
                int k = j * w + i;
                particles[k].position          = origin + glm::vec3(i * spacing, 0.0f, j * spacing);
                particles[k].predictedPosition = particles[k].position;
                particles[k].mass    = 1.0f;
                particles[k].invMass = 1.0f;
            }
        }

        // Distance constraints — horizontal, vertical, diagonal (shear), long-range bend
        float diag = spacing * 1.4142f;
        float bend = spacing * 2.0f;
        float compliance = 0.001f;

        for (int j = 0; j < h; ++j) {
            for (int i = 0; i < w; ++i) {
                int k = j * w + i;
                if (i < w-1) distConstraints.push_back({k, k+1, spacing, compliance});
                if (j < h-1) distConstraints.push_back({k, k+w, spacing, compliance});
                if (i < w-1 && j < h-1) distConstraints.push_back({k, k+w+1, diag, compliance});
                if (i > 0   && j < h-1) distConstraints.push_back({k, k+w-1, diag, compliance});
                if (i < w-2) distConstraints.push_back({k, k+2, bend, compliance * 0.1f});
                if (j < h-2) distConstraints.push_back({k, k+2*w, bend, compliance * 0.1f});
            }
        }
    }

    void pinCorners() {
        // Pin all four corners — same as feather main.cpp (cloth->addFixedConstraint)
        auto pin = [&](int idx) {
            particles[idx].invMass = 0.f;
            particles[idx].isFixed = true;
            fixedConstraints.push_back({idx, particles[idx].position});
        };
        pin(0);
        pin(gridW - 1);
        pin((gridH - 1) * gridW);
        pin(gridH * gridW - 1);
    }

    void pinTopEdge() {
        for (int i = 0; i < gridW; ++i) {
            particles[i].invMass = 0.f;
            particles[i].isFixed = true;
            fixedConstraints.push_back({i, particles[i].position});
        }
    }

    // Solve one full time step — algorithm 1:1 from HXPbdSolver::solve() (feather)
    void solve(float deltaTime) {
        // Register forces (HXPbdSolver::solve line 59-68)
        for (auto& p : particles) {
            for (auto& a : accelerations) {
                p.externalForces.push_back(a * p.mass);   // F = m * a
            }
        }

        float subTimeStep = deltaTime / static_cast<float>(iterations);

        for (int iter = 0; iter < iterations; ++iter) {
            // Apply external forces → velocity  (line 74-83)
            for (auto& p : particles) {
                if (p.invMass == 0.f) continue;
                // a = F/m  and  v += a * dt  =>  v += dt * invMass * F
                p.velocity += subTimeStep * p.invMass * p.resultingExternalForce();
                p.velocity *= damping;  // velocity damping (line 81)
            }

            // Predict positions  (line 100-105)
            for (auto& p : particles) {
                p.predictedPosition = p.position + subTimeStep * p.velocity;
            }

            // Solve distance constraints  (line 238-243)
            for (auto& c : distConstraints) {
                c.solve(particles, subTimeStep);
            }

            // Solve fixed constraints  (line 259-261)
            for (auto& c : fixedConstraints) {
                c.solve(particles, subTimeStep);
            }

            // Update velocity and position  (line 285-292)
            for (auto& p : particles) {
                p.velocity = (p.predictedPosition - p.position) / subTimeStep;
                p.position = p.predictedPosition;
            }
        }

        // Clear external forces for next step  (line 299)
        for (auto& p : particles) {
            p.externalForces.clear();
        }
    }
};
