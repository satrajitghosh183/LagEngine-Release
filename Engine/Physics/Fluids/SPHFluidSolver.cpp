#include "SPHFluidSolver.hpp"
#include "SPHKernels.hpp"
#include <cmath>
#include <random>

namespace GameEngine::Physics {

    void SPHFluidSolver::Init(int numParticles, const glm::vec3& domainMin, const glm::vec3& domainMax) {
        DomainMin = domainMin;
        DomainMax = domainMax;
        m_Particles.resize(numParticles);
        m_Grid.SetCellSize(SmoothingLength);

        std::default_random_engine gen(42);
        glm::vec3 range = domainMax - domainMin;
        std::uniform_real_distribution<float> dx(0.0f, range.x * 0.5f);
        std::uniform_real_distribution<float> dy(0.0f, range.y * 0.8f);
        std::uniform_real_distribution<float> dz(0.0f, range.z * 0.5f);

        for (auto& p : m_Particles) {
            p.position = domainMin + glm::vec3(dx(gen), dy(gen), dz(gen));
            p.velocity = glm::vec3(0.0f);
        }
    }

    void SPHFluidSolver::Clear() { m_Particles.clear(); }

    std::vector<glm::vec3> SPHFluidSolver::GetPositions() const {
        std::vector<glm::vec3> pos;
        pos.reserve(m_Particles.size());
        for (auto& p : m_Particles) pos.push_back(p.position);
        return pos;
    }

    void SPHFluidSolver::Update(float dt) {
        m_Grid.Clear();
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            m_Grid.Insert(i, m_Particles[i].position);
        }
        ComputeDensityPressure();
        ComputeForces();
        Integrate(dt);
        HandleBoundaries();
    }

    void SPHFluidSolver::ComputeDensityPressure() {
        float h = SmoothingLength;
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            auto& pi = m_Particles[i];
            pi.density = 0.0f;
            auto neighbors = m_Grid.Query(pi.position, h);
            for (int j : neighbors) {
                glm::vec3 diff = pi.position - m_Particles[j].position;
                float r2 = glm::dot(diff, diff);
                pi.density += SPHKernels::Poly6(r2, h);
            }
            pi.pressure = PressureCoefficient * (pi.density - RestDensity);
        }
    }

    void SPHFluidSolver::ComputeForces() {
        float h = SmoothingLength;
        for (int i = 0; i < static_cast<int>(m_Particles.size()); i++) {
            auto& pi = m_Particles[i];
            glm::vec3 fPressure(0.0f), fViscosity(0.0f);
            auto neighbors = m_Grid.Query(pi.position, h);

            for (int j : neighbors) {
                if (i == j) continue;
                auto& pj = m_Particles[j];
                glm::vec3 diff = pi.position - pj.position;
                float r = glm::length(diff);
                if (r < 1e-6f || r >= h) continue;

                float avgDens = std::max(pi.density * pj.density, 1e-6f);
                fPressure += -glm::normalize(diff) * ((pi.pressure + pj.pressure) / (2.0f * avgDens))
                           * glm::length(SPHKernels::SpikyGrad(diff, r, h));
                fViscosity += ViscosityCoefficient * (pj.velocity - pi.velocity) / std::max(pj.density, 1e-6f)
                            * SPHKernels::ViscosityLap(r, h);
            }

            pi.force = fPressure + fViscosity + Gravity * pi.density;
        }
    }

    void SPHFluidSolver::Integrate(float dt) {
        for (auto& p : m_Particles) {
            glm::vec3 acc = p.force / std::max(p.density, 1e-6f);
            p.velocity += acc * dt;
            p.position += p.velocity * dt;
        }
    }

    void SPHFluidSolver::HandleBoundaries() {
        for (auto& p : m_Particles) {
            for (int axis = 0; axis < 3; axis++) {
                if (p.position[axis] < DomainMin[axis]) {
                    p.position[axis] = DomainMin[axis];
                    p.velocity[axis] *= -BounceCoefficient;
                }
                if (p.position[axis] > DomainMax[axis]) {
                    p.position[axis] = DomainMax[axis];
                    p.velocity[axis] *= -BounceCoefficient;
                }
            }
        }
    }

}
