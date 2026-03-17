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
using namespace GameEngine::Physics::SPHKernels;

constexpr float EPSILON = 1e-4f;

// ==================== Kernel Tests ====================

TEST(SPHKernels, Poly6AtZero) {
    float h  = 0.1f;
    // Poly6 takes r² as first argument
    float val = Poly6(0.0f, h);   // r=0 → r²=0
    EXPECT_GT(val, 0.0f);
}

TEST(SPHKernels, Poly6AtBoundary) {
    float h   = 0.1f;
    float val = Poly6(h * h, h);  // r=h → r²=h²
    EXPECT_NEAR(val, 0.0f, EPSILON);
}

TEST(SPHKernels, Poly6BeyondBoundary) {
    float h     = 0.1f;
    float rBeyond = h + 0.01f;
    float val  = Poly6(rBeyond * rBeyond, h);
    EXPECT_NEAR(val, 0.0f, EPSILON);
}

TEST(SPHKernels, SpikyGradDirection) {
    float h = 0.1f;
    glm::vec3 r(0.05f, 0.0f, 0.0f);
    float rLen = glm::length(r);
    glm::vec3 grad = SpikyGrad(r, rLen, h);
    // Gradient should point away from origin (repulsive)
    EXPECT_LT(grad.x, 0.0f);
}

// ==================== Spatial Hash Grid Tests ====================

TEST(SpatialHashGrid, InsertAndQuery) {
    SpatialHashGrid grid(0.1f);
    grid.Clear();
    grid.Insert(0, glm::vec3(0.0f, 0.0f, 0.0f));
    grid.Insert(1, glm::vec3(0.05f, 0.0f, 0.0f));
    grid.Insert(2, glm::vec3(10.0f, 0.0f, 0.0f));

    auto neighbors = grid.Query(glm::vec3(0.0f), 0.1f);
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

TEST_F(SPHSolverTest, InitCreatesParticles) {
    m_Solver->Init(10, glm::vec3(-1.0f), glm::vec3(1.0f));
    EXPECT_EQ(m_Solver->GetParticles().size(), 10u);
}

TEST_F(SPHSolverTest, GravityMovesParticles) {
    m_Solver->Init(4, glm::vec3(-1.0f), glm::vec3(1.0f));
    m_Solver->Gravity = glm::vec3(0.0f, -9.81f, 0.0f);

    // Record starting average Y
    float startY = 0.0f;
    for (const auto& p : m_Solver->GetParticles())
        startY += p.position.y;
    startY /= static_cast<float>(m_Solver->GetParticles().size());

    // Simulate a few steps
    for (int i = 0; i < 5; i++)
        m_Solver->Update(1.0f / 60.0f);

    float endY = 0.0f;
    for (const auto& p : m_Solver->GetParticles())
        endY += p.position.y;
    endY /= static_cast<float>(m_Solver->GetParticles().size());

    // Average Y should have decreased (gravity pulled down)
    EXPECT_LT(endY, startY + EPSILON);
}

TEST_F(SPHSolverTest, BoundaryContainment) {
    glm::vec3 domMin(-1.0f), domMax(1.0f);
    m_Solver->Init(8, domMin, domMax);
    m_Solver->DomainMin = domMin;
    m_Solver->DomainMax = domMax;
    m_Solver->Gravity   = glm::vec3(0.0f, -9.81f, 0.0f);

    for (int i = 0; i < 30; i++)
        m_Solver->Update(1.0f / 60.0f);

    auto positions = m_Solver->GetPositions();
    for (const auto& pos : positions) {
        EXPECT_GE(pos.y, domMin.y - EPSILON);
        EXPECT_LE(pos.y, domMax.y + EPSILON);
    }
}
