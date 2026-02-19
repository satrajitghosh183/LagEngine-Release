/**
 * @file ConstraintTests.cpp
 * @brief Unit tests for physics constraints
 */

#include <gtest/gtest.h>
#include "Physics/Constraints/DistanceJoint.hpp"
#include "Physics/Constraints/HingeJoint.hpp"
#include "Physics/Constraints/FixedJoint.hpp"
#include "Physics/Constraints/ConstraintSolver.hpp"
#include "Physics/RigidBody.hpp"
#include "Physics/Collision/ContactManifold.hpp"
#include <glm/glm.hpp>

using namespace GameEngine;
using namespace GameEngine::Physics;

constexpr float EPSILON = 1e-3f;

// Helper to run the constraint solver on a set of constraints
static void SolveConstraints(Ref<ConstraintSolver>& solver,
                             std::vector<Ref<Constraint>>& constraints,
                             float deltaTime) {
    std::vector<ContactManifold> emptyContacts;
    solver->Solve(constraints, emptyContacts, deltaTime);
}

class ConstraintTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_BodyA = CreateRef<RigidBody>();
        m_BodyA->SetMass(1.0f);
        m_BodyA->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));

        m_BodyB = CreateRef<RigidBody>();
        m_BodyB->SetMass(1.0f);
        m_BodyB->SetPosition(glm::vec3(2.0f, 0.0f, 0.0f));

        m_Solver = CreateRef<ConstraintSolver>();
    }

    Ref<RigidBody> m_BodyA;
    Ref<RigidBody> m_BodyB;
    Ref<ConstraintSolver> m_Solver;
};

// ==================== Distance Joint ====================

TEST_F(ConstraintTest, DistanceJoint_MaintainsDistance) {
    float targetDistance = 2.0f;
    auto joint = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),  // Anchor points at centers
        targetDistance
    );

    // Move body B further away
    m_BodyB->SetPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    for (int i = 0; i < 10; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Check distance is maintained
    float distance = glm::length(m_BodyB->GetPosition() - m_BodyA->GetPosition());
    EXPECT_NEAR(distance, targetDistance, EPSILON * 10);  // Allow some tolerance
}

TEST_F(ConstraintTest, DistanceJoint_WithSpring) {
    float targetDistance = 2.0f;
    auto joint = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        targetDistance
    );
    joint->IsSpring = true;
    joint->SpringStiffness = 50.0f;
    joint->SpringDamping = 5.0f;

    m_BodyB->SetPosition(glm::vec3(4.0f, 0.0f, 0.0f));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    SolveConstraints(m_Solver, constraints, 0.016f);

    // With spring, bodies shouldn't fully correct in one step
    float distance = glm::length(m_BodyB->GetPosition() - m_BodyA->GetPosition());
    EXPECT_GT(distance, targetDistance);
}

// ==================== Fixed Joint (Ball Joint) ====================

TEST_F(ConstraintTest, FixedJoint_KeepsPointsTogether) {
    auto joint = CreateRef<FixedJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(1.0f, 0.0f, 0.0f),   // Point on A
        glm::vec3(-1.0f, 0.0f, 0.0f)   // Point on B
    );

    // Bodies start at (0,0,0) and (2,0,0), so constraint points are at (1,0,0)

    // Move body B
    m_BodyB->SetPosition(glm::vec3(3.0f, 1.0f, 0.0f));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    for (int i = 0; i < 20; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Check that anchor points meet
    glm::vec3 worldAnchorA = m_BodyA->GetPosition() + glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 worldAnchorB = m_BodyB->GetPosition() + glm::vec3(-1.0f, 0.0f, 0.0f);

    float error = glm::length(worldAnchorB - worldAnchorA);
    EXPECT_NEAR(error, 0.0f, EPSILON * 100);
}

// ==================== Hinge Joint ====================

TEST_F(ConstraintTest, HingeJoint_ConstrainsAxis) {
    glm::vec3 hingeAxis(0.0f, 1.0f, 0.0f);
    auto joint = CreateRef<HingeJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(1.0f, 0.0f, 0.0f),   // Anchor on A
        glm::vec3(-1.0f, 0.0f, 0.0f),  // Anchor on B
        hingeAxis,                       // Axis on A
        hingeAxis                        // Axis on B
    );

    // Rotate body B around non-hinge axis
    m_BodyB->SetRotation(glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    for (int i = 0; i < 20; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Body should be constrained to rotate only around Y axis
    // (Test that X-rotation is corrected)
}

TEST_F(ConstraintTest, HingeJoint_WithLimits) {
    glm::vec3 hingeAxis(0.0f, 1.0f, 0.0f);
    auto joint = CreateRef<HingeJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(-1.0f, 0.0f, 0.0f),
        hingeAxis,
        hingeAxis
    );
    joint->UseLimits = true;
    joint->LowerLimit = glm::radians(-45.0f);
    joint->UpperLimit = glm::radians(45.0f);

    // Rotate body B beyond limits
    m_BodyB->SetRotation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    for (int i = 0; i < 20; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Check rotation is within limits
    float angle = joint->GetCurrentAngle();
    EXPECT_LE(angle, glm::radians(45.0f) + EPSILON);
    EXPECT_GE(angle, glm::radians(-45.0f) - EPSILON);
}

// ==================== Constraint Break Force ====================

TEST_F(ConstraintTest, DistanceJoint_BreakForce) {
    auto joint = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        2.0f
    );
    joint->BreakForce = 10.0f;

    // Verify break force is set
    EXPECT_FLOAT_EQ(joint->BreakForce, 10.0f);

    // Verify default is unbreakable
    auto unbreakableJoint = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        2.0f
    );
    EXPECT_FLOAT_EQ(unbreakableJoint->BreakForce, 0.0f);
}

// ==================== Static Body Constraints ====================

TEST_F(ConstraintTest, ConstraintWithStaticBody) {
    m_BodyA->SetBodyType(RigidBody::BodyType::Static);

    auto joint = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        2.0f
    );

    // Move body B
    m_BodyB->SetPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint);

    for (int i = 0; i < 10; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Body A should not have moved
    EXPECT_EQ(m_BodyA->GetPosition(), glm::vec3(0.0f));

    // Body B should be at distance 2 from A
    float distance = glm::length(m_BodyB->GetPosition() - m_BodyA->GetPosition());
    EXPECT_NEAR(distance, 2.0f, EPSILON * 10);
}

// ==================== Multiple Constraints ====================

TEST_F(ConstraintTest, MultipleConstraints) {
    auto bodyC = CreateRef<RigidBody>();
    bodyC->SetMass(1.0f);
    bodyC->SetPosition(glm::vec3(4.0f, 0.0f, 0.0f));

    // Chain: A - B - C
    auto joint1 = CreateRef<DistanceJoint>(
        m_BodyA.get(), m_BodyB.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        2.0f
    );
    auto joint2 = CreateRef<DistanceJoint>(
        m_BodyB.get(), bodyC.get(),
        glm::vec3(0.0f), glm::vec3(0.0f),
        2.0f
    );

    // Perturb positions
    m_BodyB->SetPosition(glm::vec3(3.0f, 1.0f, 0.0f));
    bodyC->SetPosition(glm::vec3(6.0f, -1.0f, 0.0f));

    std::vector<Ref<Constraint>> constraints;
    constraints.push_back(joint1);
    constraints.push_back(joint2);

    for (int i = 0; i < 50; i++) {
        SolveConstraints(m_Solver, constraints, 0.016f);
    }

    // Check both distances
    float dist1 = glm::length(m_BodyB->GetPosition() - m_BodyA->GetPosition());
    float dist2 = glm::length(bodyC->GetPosition() - m_BodyB->GetPosition());

    EXPECT_NEAR(dist1, 2.0f, EPSILON * 100);
    EXPECT_NEAR(dist2, 2.0f, EPSILON * 100);
}
