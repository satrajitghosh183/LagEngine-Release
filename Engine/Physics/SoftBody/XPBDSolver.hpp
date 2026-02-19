#pragma once

#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace GameEngine::Physics {

    struct XPBDParticle {
        glm::vec3 position{0.0f};
        glm::vec3 prevPosition{0.0f};
        glm::vec3 velocity{0.0f};
        float invMass = 1.0f;
        bool pinned = false;
    };

    class XPBDConstraint {
    public:
        virtual ~XPBDConstraint() = default;
        virtual void Project(std::vector<XPBDParticle>& particles, float dt) = 0;
        float Compliance = 0.0f;
    protected:
        float m_Lambda = 0.0f;
    };

    class DistanceConstraint : public XPBDConstraint {
    public:
        DistanceConstraint(int p1, int p2, float restLength)
            : m_P1(p1), m_P2(p2), m_RestLength(restLength) {}
        void Project(std::vector<XPBDParticle>& particles, float dt) override;
    private:
        int m_P1, m_P2;
        float m_RestLength;
    };

    class BendConstraint : public XPBDConstraint {
    public:
        BendConstraint(int p1, int p2, int p3, int p4, float restAngle)
            : m_P1(p1), m_P2(p2), m_P3(p3), m_P4(p4), m_RestAngle(restAngle) {}
        void Project(std::vector<XPBDParticle>& particles, float dt) override;
    private:
        int m_P1, m_P2, m_P3, m_P4;
        float m_RestAngle;
    };

    class VolumeConstraint : public XPBDConstraint {
    public:
        VolumeConstraint(int p1, int p2, int p3, int p4, float restVolume)
            : m_P1(p1), m_P2(p2), m_P3(p3), m_P4(p4), m_RestVolume(restVolume) {}
        void Project(std::vector<XPBDParticle>& particles, float dt) override;
    private:
        int m_P1, m_P2, m_P3, m_P4;
        float m_RestVolume;
    };

    class CollisionConstraint : public XPBDConstraint {
    public:
        CollisionConstraint(int particleIdx, const glm::vec3& normal, float offset)
            : m_ParticleIdx(particleIdx), m_Normal(normal), m_Offset(offset) {}
        void Project(std::vector<XPBDParticle>& particles, float dt) override;
    private:
        int m_ParticleIdx;
        glm::vec3 m_Normal;
        float m_Offset;
    };

    class XPBDSolver {
    public:
        void AddParticle(const glm::vec3& pos, float mass);
        void PinParticle(int index);
        void UnpinParticle(int index);
        void AddConstraint(std::unique_ptr<XPBDConstraint> constraint);
        void SetGravity(const glm::vec3& g) { m_Gravity = g; }
        void SetDamping(float d) { m_Damping = d; }
        void Solve(float dt, int substeps = 4);
        void Clear();

        const std::vector<XPBDParticle>& GetParticles() const { return m_Particles; }
        std::vector<XPBDParticle>& GetParticles() { return m_Particles; }
        size_t GetParticleCount() const { return m_Particles.size(); }
        size_t GetConstraintCount() const { return m_Constraints.size(); }

    private:
        std::vector<XPBDParticle> m_Particles;
        std::vector<std::unique_ptr<XPBDConstraint>> m_Constraints;
        glm::vec3 m_Gravity = {0.0f, -9.81f, 0.0f};
        float m_Damping = 0.99f;
    };

}
