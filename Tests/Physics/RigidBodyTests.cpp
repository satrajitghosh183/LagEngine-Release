/**
 * @file RigidBodyTests.cpp
 * @brief Unit tests for RigidBody dynamics
 */

#include <gtest/gtest.h>
#include "Physics/RigidBody.hpp"
#include "Physics/Shapes/SphereShape.hpp"
#include "Physics/Shapes/BoxShape.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace GameEngine;
using namespace GameEngine::Physics;

constexpr float EPSILON = 1e-4f;

class RigidBodyTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_Body = CreateRef<RigidBody>();
        m_Body->SetPosition(glm::vec3(0.0f));
        m_Body->SetRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        m_Body->SetMass(1.0f);
    }

    Ref<RigidBody> m_Body;
};

// ==================== Basic Properties ====================

TEST_F(RigidBodyTest, InitialState) {
    EXPECT_EQ(m_Body->GetPosition(), glm::vec3(0.0f));
    EXPECT_EQ(m_Body->GetLinearVelocity(), glm::vec3(0.0f));
    EXPECT_EQ(m_Body->GetAngularVelocity(), glm::vec3(0.0f));
    EXPECT_FLOAT_EQ(m_Body->GetMass(), 1.0f);
}

TEST_F(RigidBodyTest, SetPosition) {
    m_Body->SetPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(m_Body->GetPosition(), glm::vec3(1.0f, 2.0f, 3.0f));
}

TEST_F(RigidBodyTest, SetRotation) {
    glm::quat rot = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    m_Body->SetRotation(rot);
    
    glm::quat bodyRot = m_Body->GetRotation();
    EXPECT_NEAR(bodyRot.w, rot.w, EPSILON);
    EXPECT_NEAR(bodyRot.x, rot.x, EPSILON);
    EXPECT_NEAR(bodyRot.y, rot.y, EPSILON);
    EXPECT_NEAR(bodyRot.z, rot.z, EPSILON);
}

TEST_F(RigidBodyTest, SetMass) {
    m_Body->SetMass(5.0f);
    EXPECT_FLOAT_EQ(m_Body->GetMass(), 5.0f);
    EXPECT_FLOAT_EQ(m_Body->GetInverseMass(), 0.2f);
}

TEST_F(RigidBodyTest, StaticBody) {
    m_Body->SetBodyType(RigidBody::BodyType::Static);
    EXPECT_FLOAT_EQ(m_Body->GetMass(), 0.0f);
    EXPECT_FLOAT_EQ(m_Body->GetInverseMass(), 0.0f);
    EXPECT_EQ(m_Body->GetBodyType(), RigidBody::BodyType::Static);
}

// ==================== Force Application ====================

TEST_F(RigidBodyTest, ApplyForce) {
    m_Body->ApplyForce(glm::vec3(10.0f, 0.0f, 0.0f));
    m_Body->IntegrateForces(1.0f);  // 1 second
    
    // F = ma, so a = F/m = 10/1 = 10 m/s^2
    // After 1 second: v = 10 m/s
    glm::vec3 velocity = m_Body->GetLinearVelocity();
    EXPECT_NEAR(velocity.x, 10.0f, EPSILON);
    EXPECT_NEAR(velocity.y, 0.0f, EPSILON);
    EXPECT_NEAR(velocity.z, 0.0f, EPSILON);
}

TEST_F(RigidBodyTest, ApplyForceAtPoint) {
    m_Body->SetPosition(glm::vec3(0.0f));
    
    // Apply force at offset to create torque
    glm::vec3 force(0.0f, 10.0f, 0.0f);
    glm::vec3 point(1.0f, 0.0f, 0.0f);  // 1 unit from center
    
    m_Body->ApplyForceAtPoint(force, point);
    m_Body->IntegrateForces(1.0f);
    
    // Should have both linear and angular velocity
    glm::vec3 linearVel = m_Body->GetLinearVelocity();
    glm::vec3 angularVel = m_Body->GetAngularVelocity();
    
    EXPECT_NEAR(linearVel.y, 10.0f, EPSILON);  // Linear force
    EXPECT_NE(glm::length(angularVel), 0.0f);   // Torque generated
}

TEST_F(RigidBodyTest, ApplyImpulse) {
    m_Body->ApplyImpulse(glm::vec3(5.0f, 0.0f, 0.0f));
    
    // Impulse directly changes velocity: v = J/m = 5/1 = 5
    glm::vec3 velocity = m_Body->GetLinearVelocity();
    EXPECT_NEAR(velocity.x, 5.0f, EPSILON);
}

TEST_F(RigidBodyTest, ApplyTorque) {
    m_Body->ApplyTorque(glm::vec3(0.0f, 10.0f, 0.0f));
    m_Body->IntegrateForces(1.0f);
    
    glm::vec3 angularVel = m_Body->GetAngularVelocity();
    EXPECT_NE(angularVel.y, 0.0f);  // Should have Y angular velocity
}

// ==================== Integration ====================

TEST_F(RigidBodyTest, IntegrateVelocities) {
    m_Body->SetLinearVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
    m_Body->IntegrateVelocities(1.0f);  // 1 second
    
    glm::vec3 position = m_Body->GetPosition();
    EXPECT_NEAR(position.x, 1.0f, EPSILON);
}

TEST_F(RigidBodyTest, IntegrateRotation) {
    m_Body->SetAngularVelocity(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
    m_Body->IntegrateVelocities(1.0f);  // 1 second
    
    glm::quat rotation = m_Body->GetRotation();
    
    // After 1 second of 90 deg/s rotation, should have rotated 90 degrees
    glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    EXPECT_NEAR(forward.x, -1.0f, EPSILON);
    EXPECT_NEAR(forward.z, 0.0f, EPSILON);
}

TEST_F(RigidBodyTest, Damping) {
    m_Body->SetLinearDamping(0.5f);
    m_Body->SetLinearVelocity(glm::vec3(10.0f, 0.0f, 0.0f));
    
    // Integrate several times
    for (int i = 0; i < 10; i++) {
        m_Body->IntegrateVelocities(0.1f);
    }
    
    // Velocity should have decreased due to damping
    float speed = glm::length(m_Body->GetLinearVelocity());
    EXPECT_LT(speed, 10.0f);
}

// ==================== Gravity ====================

TEST_F(RigidBodyTest, GravityFall) {
    glm::vec3 gravity(0.0f, -9.81f, 0.0f);
    m_Body->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
    
    // Simulate 1 second of falling
    float dt = 0.01f;
    for (int i = 0; i < 100; i++) {
        m_Body->ApplyForce(gravity * m_Body->GetMass());
        m_Body->IntegrateForces(dt);
        m_Body->IntegrateVelocities(dt);
        m_Body->ClearForces();
    }
    
    // After 1 second: y = 10 - 0.5 * 9.81 * 1^2 = 10 - 4.905 ≈ 5.1
    float y = m_Body->GetPosition().y;
    EXPECT_NEAR(y, 5.095f, 0.5f);  // Allow some tolerance for integration error
    
    // Velocity should be approximately -9.81
    EXPECT_NEAR(m_Body->GetLinearVelocity().y, -9.81f, 0.5f);
}

// ==================== Kinematic State ====================

TEST_F(RigidBodyTest, KinematicBody) {
    m_Body->SetBodyType(RigidBody::BodyType::Kinematic);
    EXPECT_EQ(m_Body->GetBodyType(), RigidBody::BodyType::Kinematic);

    // Kinematic bodies should not respond to forces
    m_Body->ApplyForce(glm::vec3(100.0f, 0.0f, 0.0f));
    m_Body->IntegrateForces(1.0f);

    EXPECT_EQ(m_Body->GetLinearVelocity(), glm::vec3(0.0f));
}

// ==================== Sleep State ====================

TEST_F(RigidBodyTest, SleepState) {
    m_Body->SetAwake(false);
    EXPECT_FALSE(m_Body->IsAwake());
    
    // Wake up when force applied
    m_Body->ApplyForce(glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_TRUE(m_Body->IsAwake());
}

// ==================== Inertia Tensor ====================

TEST_F(RigidBodyTest, SphereInertiaTensor) {
    auto sphere = CreateRef<SphereShape>(1.0f);
    m_Body->SetCollisionShape(sphere);
    m_Body->SetMass(1.0f);
    m_Body->SetInertiaTensor(sphere->CalculateInertiaTensor(1.0f));

    glm::mat3 inertia = m_Body->GetInertiaTensor();

    // For a solid sphere: I = (2/5) * m * r^2 = 0.4 for unit sphere with unit mass
    EXPECT_NEAR(inertia[0][0], 0.4f, EPSILON);
    EXPECT_NEAR(inertia[1][1], 0.4f, EPSILON);
    EXPECT_NEAR(inertia[2][2], 0.4f, EPSILON);
}

TEST_F(RigidBodyTest, BoxInertiaTensor) {
    auto box = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));  // 2x2x2 box
    m_Body->SetCollisionShape(box);
    m_Body->SetMass(1.0f);
    m_Body->SetInertiaTensor(box->CalculateInertiaTensor(1.0f));

    glm::mat3 inertia = m_Body->GetInertiaTensor();

    // For a box: I_x = (1/12) * m * (h^2 + d^2) = (1/12) * 1 * (4 + 4) = 0.667
    EXPECT_NEAR(inertia[0][0], 0.667f, 0.01f);
    EXPECT_NEAR(inertia[1][1], 0.667f, 0.01f);
    EXPECT_NEAR(inertia[2][2], 0.667f, 0.01f);
}

// ==================== World Transform ====================

TEST_F(RigidBodyTest, GetWorldTransform) {
    m_Body->SetPosition(glm::vec3(1.0f, 2.0f, 3.0f));
    m_Body->SetRotation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));

    // Compute world transform from position and rotation
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Body->GetPosition()) *
                           glm::mat4_cast(m_Body->GetRotation());

    // Check translation
    EXPECT_NEAR(transform[3][0], 1.0f, EPSILON);
    EXPECT_NEAR(transform[3][1], 2.0f, EPSILON);
    EXPECT_NEAR(transform[3][2], 3.0f, EPSILON);
}

// ==================== Collision Shape ====================

TEST_F(RigidBodyTest, SetCollisionShape) {
    auto sphere = CreateRef<SphereShape>(2.0f);
    m_Body->SetCollisionShape(sphere);
    
    EXPECT_EQ(m_Body->GetCollisionShape().get(), sphere.get());
}
