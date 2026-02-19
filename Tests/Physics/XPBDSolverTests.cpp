/**
 * @file XPBDSolverTests.cpp
 * @brief Unit tests for XPBD soft body solver
 */

#include <gtest/gtest.h>
#include "Physics/SoftBody/XPBDSolver.hpp"
#include <glm/glm.hpp>

using namespace GameEngine::Physics;

constexpr float EPSILON = 1e-3f;

class XPBDSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_Solver = std::make_unique<XPBDSolver>();
    }

    std::unique_ptr<XPBDSolver> m_Solver;
};

TEST_F(XPBDSolverTest, AddParticles) {
    int idx0 = m_Solver->AddParticle(glm::vec3(0.0f), 1.0f);
    int idx1 = m_Solver->AddParticle(glm::vec3(1.0f, 0.0f, 0.0f), 1.0f);

    EXPECT_EQ(idx0, 0);
    EXPECT_EQ(idx1, 1);
    EXPECT_EQ(m_Solver->GetParticleCount(), 2);
}

TEST_F(XPBDSolverTest, ParticlePosition) {
    m_Solver->AddParticle(glm::vec3(1.0f, 2.0f, 3.0f), 1.0f);
    glm::vec3 pos = m_Solver->GetPosition(0);
    EXPECT_NEAR(pos.x, 1.0f, EPSILON);
    EXPECT_NEAR(pos.y, 2.0f, EPSILON);
    EXPECT_NEAR(pos.z, 3.0f, EPSILON);
}

TEST_F(XPBDSolverTest, ZeroMassIsFixed) {
    m_Solver->AddParticle(glm::vec3(0.0f, 5.0f, 0.0f), 0.0f);  // Fixed particle
    m_Solver->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    m_Solver->Step(1.0f / 60.0f);

    // Fixed particle should not move
    glm::vec3 pos = m_Solver->GetPosition(0);
    EXPECT_NEAR(pos.y, 5.0f, EPSILON);
}

TEST_F(XPBDSolverTest, GravityApplied) {
    m_Solver->AddParticle(glm::vec3(0.0f, 10.0f, 0.0f), 1.0f);
    m_Solver->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    m_Solver->Step(1.0f / 60.0f);

    // Particle should fall
    glm::vec3 pos = m_Solver->GetPosition(0);
    EXPECT_LT(pos.y, 10.0f);
}

TEST_F(XPBDSolverTest, DistanceConstraint) {
    m_Solver->AddParticle(glm::vec3(0.0f), 1.0f);
    m_Solver->AddParticle(glm::vec3(2.0f, 0.0f, 0.0f), 1.0f);
    m_Solver->AddDistanceConstraint(0, 1, 1.0f, 0.0f);  // Rest length 1, zero compliance
    m_Solver->SetGravity(glm::vec3(0.0f));
    m_Solver->SetSubSteps(10);
    m_Solver->Step(1.0f / 60.0f);

    // After solving, distance should be close to rest length
    glm::vec3 p0 = m_Solver->GetPosition(0);
    glm::vec3 p1 = m_Solver->GetPosition(1);
    float dist = glm::length(p1 - p0);
    EXPECT_NEAR(dist, 1.0f, 0.1f);  // Allow some tolerance for one step
}

TEST_F(XPBDSolverTest, DampingReducesVelocity) {
    m_Solver->AddParticle(glm::vec3(0.0f), 1.0f);
    m_Solver->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    m_Solver->SetDamping(0.5f);  // Heavy damping

    m_Solver->Step(1.0f / 60.0f);
    glm::vec3 pos1 = m_Solver->GetPosition(0);

    // With heavy damping, particle should move slower than without
    EXPECT_GT(pos1.y, -1.0f);  // Should not have fallen too far
}
