/**
 * @file RoboticsTests.cpp
 * @brief Unit tests for DH parameters, FK/IK solver, and PID controller
 */

#include <gtest/gtest.h>
#include "Robotics/DHParameter.hpp"
#include "Robotics/RobotArm.hpp"
#include "Robotics/IKSolver.hpp"
#include "Robotics/PIDController.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

using namespace GameEngine;

constexpr float EPSILON = 1e-3f;

// ==================== DH Parameter Tests ====================

TEST(DHParameter, IdentityTransform) {
    DHParameter dh{0.0f, 0.0f, 0.0f, 0.0f};
    glm::mat4 T = dh.GetTransformMatrix(0.0f);
    // Should be close to identity
    EXPECT_NEAR(T[3][0], 0.0f, EPSILON);
    EXPECT_NEAR(T[3][1], 0.0f, EPSILON);
    EXPECT_NEAR(T[3][2], 0.0f, EPSILON);
}

TEST(DHParameter, TranslationAlongLink) {
    DHParameter dh{1.0f, 0.0f, 0.0f, 0.0f};  // a=1 (link length)
    glm::mat4 T = dh.GetTransformMatrix(0.0f);
    // Translation along X should be 1
    EXPECT_NEAR(T[3][0], 1.0f, EPSILON);
}

TEST(DHParameter, OffsetAlongZ) {
    DHParameter dh{0.0f, 0.0f, 1.0f, 0.0f};  // d=1 (offset along Z)
    glm::mat4 T = dh.GetTransformMatrix(0.0f);
    EXPECT_NEAR(T[3][2], 1.0f, EPSILON);
}

// ==================== Robot Arm FK Tests ====================

TEST(RobotArm, SingleLinkFK) {
    RobotArm arm;
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});

    std::vector<float> angles = {0.0f};
    glm::vec3 pos = arm.ForwardKinematics(angles);
    EXPECT_NEAR(pos.x, 1.0f, EPSILON);
    EXPECT_NEAR(pos.y, 0.0f, EPSILON);
}

TEST(RobotArm, SingleLinkRotated) {
    RobotArm arm;
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});

    std::vector<float> angles = {glm::half_pi<float>()};  // 90 degrees
    glm::vec3 pos = arm.ForwardKinematics(angles);
    EXPECT_NEAR(pos.x, 0.0f, 0.05f);
    EXPECT_NEAR(pos.y, 1.0f, 0.05f);
}

TEST(RobotArm, TwoLinkFK) {
    RobotArm arm;
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});

    std::vector<float> angles = {0.0f, 0.0f};
    glm::vec3 pos = arm.ForwardKinematics(angles);
    // Two unit links stretched out along X
    EXPECT_NEAR(pos.x, 2.0f, EPSILON);
    EXPECT_NEAR(pos.y, 0.0f, EPSILON);
}

// ==================== IK Solver Tests ====================

TEST(IKSolver, ReachableTarget) {
    RobotArm arm;
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});
    arm.AddLink(DHParameter{1.0f, 0.0f, 0.0f, 0.0f});

    IKSolver solver;
    solver.SetTolerance(0.01f);
    solver.SetMaxIterations(200);

    std::vector<float> angles = {0.0f, 0.0f};
    glm::vec3 target(1.0f, 1.0f, 0.0f);

    bool solved = solver.Solve(arm, angles, target);
    EXPECT_TRUE(solved);

    glm::vec3 result = arm.ForwardKinematics(angles);
    float error = glm::length(result - target);
    EXPECT_LT(error, 0.05f);
}

// ==================== PID Controller Tests ====================

TEST(PIDController, ConvergesToSetpoint) {
    PIDController pid(2.0f, 0.1f, 0.5f);

    float value = 0.0f;
    float setpoint = 1.0f;
    float dt = 0.01f;

    for (int i = 0; i < 1000; i++) {
        float output = pid.Compute(setpoint, value, dt);
        value += output * dt;
    }

    EXPECT_NEAR(value, setpoint, 0.1f);
}

TEST(PIDController, Reset) {
    PIDController pid(1.0f, 0.5f, 0.0f);

    pid.Compute(1.0f, 0.0f, 0.01f);
    pid.Compute(1.0f, 0.5f, 0.01f);
    pid.Reset();

    // After reset, integral term should be zero
    float output = pid.Compute(0.0f, 0.0f, 0.01f);
    EXPECT_NEAR(output, 0.0f, EPSILON);
}
