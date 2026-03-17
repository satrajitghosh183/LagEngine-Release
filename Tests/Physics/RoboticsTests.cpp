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
    glm::mat4 T = dh.GetTransformMatrix();
    EXPECT_NEAR(T[3][0], 0.0f, EPSILON);
    EXPECT_NEAR(T[3][1], 0.0f, EPSILON);
    EXPECT_NEAR(T[3][2], 0.0f, EPSILON);
}

TEST(DHParameter, TranslationAlongLink) {
    DHParameter dh{1.0f, 0.0f, 0.0f, 0.0f};  // a=1 (link length), theta=0
    glm::mat4 T = dh.GetTransformMatrix();
    EXPECT_NEAR(T[3][0], 1.0f, EPSILON);
}

TEST(DHParameter, OffsetAlongZ) {
    DHParameter dh{0.0f, 0.0f, 1.0f, 0.0f};  // d=1 (offset along Z), theta=0
    glm::mat4 T = dh.GetTransformMatrix();
    EXPECT_NEAR(T[3][2], 1.0f, EPSILON);
}

// ==================== Robot Arm FK Tests ====================

TEST(RobotArm, SingleLinkFK) {
    RobotArm arm;
    arm.SetDHParameters({ DHParameter{1.0f, 0.0f, 0.0f, 0.0f} });

    std::vector<float> angles = {0.0f};
    glm::vec3 pos = arm.GetEndEffectorPosition(angles);
    EXPECT_NEAR(pos.x, 1.0f, EPSILON);
    EXPECT_NEAR(pos.y, 0.0f, EPSILON);
}

TEST(RobotArm, SingleLinkRotated) {
    RobotArm arm;
    arm.SetDHParameters({ DHParameter{1.0f, 0.0f, 0.0f, 0.0f} });

    std::vector<float> angles = {glm::half_pi<float>()};  // 90 degrees
    glm::vec3 pos = arm.GetEndEffectorPosition(angles);
    EXPECT_NEAR(pos.x, 0.0f, 0.05f);
    EXPECT_NEAR(pos.y, 1.0f, 0.05f);
}

TEST(RobotArm, TwoLinkFK) {
    RobotArm arm;
    arm.SetDHParameters({
        DHParameter{1.0f, 0.0f, 0.0f, 0.0f},
        DHParameter{1.0f, 0.0f, 0.0f, 0.0f}
    });

    std::vector<float> angles = {0.0f, 0.0f};
    glm::vec3 pos = arm.GetEndEffectorPosition(angles);
    EXPECT_NEAR(pos.x, 2.0f, EPSILON);
    EXPECT_NEAR(pos.y, 0.0f, EPSILON);
}

// ==================== IK Solver Tests ====================

TEST(IKSolver, ReachableTarget) {
    RobotArm arm;
    arm.SetDHParameters({
        DHParameter{1.0f, 0.0f, 0.0f, 0.0f},
        DHParameter{1.0f, 0.0f, 0.0f, 0.0f}
    });

    IKSolver solver;
    solver.Tolerance     = 0.01f;
    solver.MaxIterations = 200;

    std::vector<float> angles = {0.0f, 0.0f};
    glm::vec3 target(1.0f, 1.0f, 0.0f);

    IKResult result = solver.Solve(arm, angles, target);
    EXPECT_TRUE(result.converged);

    glm::vec3 resultPos = arm.GetEndEffectorPosition(result.jointAngles);
    float error = glm::length(resultPos - target);
    EXPECT_LT(error, 0.05f);
}

// ==================== PID Controller Tests ====================

TEST(PIDController, ConvergesToSetpoint) {
    PIDController pid(2.0f, 0.1f, 0.5f);

    float value    = 0.0f;
    float setpoint = 1.0f;
    float dt       = 0.01f;

    for (int i = 0; i < 1000; i++) {
        float output = pid.Update(setpoint - value, dt);
        value += output * dt;
    }

    EXPECT_NEAR(value, setpoint, 0.1f);
}

TEST(PIDController, Reset) {
    PIDController pid(1.0f, 0.5f, 0.0f);

    pid.Update(1.0f, 0.01f);
    pid.Update(0.5f, 0.01f);
    pid.Reset();

    // After reset, integral term should be zero
    float output = pid.Update(0.0f, 0.01f);
    EXPECT_NEAR(output, 0.0f, EPSILON);
}
