/**
 * @file SPHSolverTests.cpp
 * @brief Unit tests for SPH fluid solver and kernels
 */

#include <gtest/gtest.h>
#include "Physics/Fluids/SPHKernels.hpp"
#include "Physics/Fluids/SpatialHashGrid.hpp"
#include "Physics/Fluids/SPHFluidSolver.hpp"
#include <glm/glm.hpp>

using namespace GameEngine::Physics;

constexpr float EPSILON = 1e-4f;

// ==================== Kernel Tests ====================

TEST(SPHKernels, Poly6AtZero) {
    float h = 0.1f;
    float val = SPH::Poly6(0.0f, h);
    EXPECT_GT(val, 0.0f);  // Peak at r=0
}

TEST(SPHKernels, Poly6AtBoundary) {
    float h = 0.1f;
    float val = SPH::Poly6(h, h);
    EXPECT_NEAR(val, 0.0f, EPSILON);  // Zero at r=h
}

TEST(SPHKernels, Poly6BeyondBoundary) {
    float h = 0.1f;
    float val = SPH::Poly6(h + 0.01f, h);
    EXPECT_NEAR(val, 0.0f, EPSILON);  // Zero beyond h
}

TEST(SPHKernels, SpikyGradDirection) {
    float h = 0.1f;
    glm::vec3 r(0.05f, 0.0f, 0.0f);
    glm::vec3 grad = SPH::SpikyGrad(r, h);
    // Gradient should point away from origin (repulsive)
    EXPECT_LT(grad.x, 0.0f);
}

// ==================== Spatial Hash Grid Tests ====================

TEST(SpatialHashGrid, InsertAndQuery) {
    SpatialHashGrid grid(0.1f);
    grid.Clear();
    grid.Insert(0, glm::vec3(0.0f, 0.0f, 0.0f));
    grid.Insert(1, glm::vec3(0.05f, 0.0f, 0.0f));
    grid.Insert(2, glm::vec3(10.0f, 0.0f, 0.0f));  // Far away

    auto neighbors = grid.Query(glm::vec3(0.0f), 0.1f);
    // Should find at least particles 0 and 1
    bool found0 = false, found1 = false, found2 = false;
    for (int id : neighbors) {
        if (id == 0) found0 = true;
        if (id == 1) found1 = true;
        if (id == 2) found2 = true;
    }
    EXPECT_TRUE(found0);
    EXPECT_TRUE(found1);
    EXPECT_FALSE(found2);
}

TEST(SpatialHashGrid, ClearRemovesAll) {
    SpatialHashGrid grid(0.1f);
    grid.Insert(0, glm::vec3(0.0f));
    grid.Clear();
    auto neighbors = grid.Query(glm::vec3(0.0f), 0.1f);
    EXPECT_TRUE(neighbors.empty());
}

// ==================== SPH Solver Tests ====================

class SPHSolverTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_Solver = std::make_unique<SPHFluidSolver>();
    }
    std::unique_ptr<SPHFluidSolver> m_Solver;
};

TEST_F(SPHSolverTest, AddParticles) {
    m_Solver->AddParticle(glm::vec3(0.0f));
    m_Solver->AddParticle(glm::vec3(0.1f, 0.0f, 0.0f));
    EXPECT_EQ(m_Solver->GetParticleCount(), 2);
}

TEST_F(SPHSolverTest, GravityMovesParticles) {
    m_Solver->AddParticle(glm::vec3(0.0f, 1.0f, 0.0f));
    m_Solver->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    m_Solver->Step(1.0f / 60.0f);

    glm::vec3 pos = m_Solver->GetPosition(0);
    EXPECT_LT(pos.y, 1.0f);  // Particle should fall
}

TEST_F(SPHSolverTest, BoundaryContainment) {
    m_Solver->AddParticle(glm::vec3(0.0f, -5.0f, 0.0f));
    m_Solver->SetBounds(glm::vec3(-1.0f), glm::vec3(1.0f));
    m_Solver->Step(1.0f / 60.0f);

    glm::vec3 pos = m_Solver->GetPosition(0);
    EXPECT_GE(pos.y, -1.0f);  // Should be contained within bounds
}
