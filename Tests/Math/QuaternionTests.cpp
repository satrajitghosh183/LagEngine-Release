/**
 * @file QuaternionTests.cpp
 * @brief Unit tests for GLM quaternion operations used in the engine
 */

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

constexpr float EPSILON = 1e-5f;

// ==================== Quaternion Basics ====================

TEST(QuaternionTest, Identity) {
    glm::quat q = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    EXPECT_FLOAT_EQ(q.w, 1.0f);
    EXPECT_FLOAT_EQ(q.x, 0.0f);
    EXPECT_FLOAT_EQ(q.y, 0.0f);
    EXPECT_FLOAT_EQ(q.z, 0.0f);
}

TEST(QuaternionTest, Normalization) {
    glm::quat q(2.0f, 1.0f, 1.0f, 1.0f);
    glm::quat n = glm::normalize(q);
    
    float length = std::sqrt(n.w*n.w + n.x*n.x + n.y*n.y + n.z*n.z);
    EXPECT_NEAR(length, 1.0f, EPSILON);
}

TEST(QuaternionTest, FromAxisAngle) {
    glm::vec3 axis(0.0f, 1.0f, 0.0f); // Y axis
    float angle = glm::radians(90.0f);
    
    glm::quat q = glm::angleAxis(angle, axis);
    
    // Verify by rotating a vector
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    glm::vec3 rotated = q * v;
    
    EXPECT_NEAR(rotated.x, 0.0f, EPSILON);
    EXPECT_NEAR(rotated.y, 0.0f, EPSILON);
    EXPECT_NEAR(rotated.z, -1.0f, EPSILON);
}

TEST(QuaternionTest, ToAxisAngle) {
    glm::vec3 axis(0.0f, 1.0f, 0.0f);
    float angle = glm::radians(90.0f);
    
    glm::quat q = glm::angleAxis(angle, axis);
    
    float extractedAngle = glm::angle(q);
    glm::vec3 extractedAxis = glm::axis(q);
    
    EXPECT_NEAR(extractedAngle, angle, EPSILON);
    EXPECT_NEAR(extractedAxis.x, axis.x, EPSILON);
    EXPECT_NEAR(extractedAxis.y, axis.y, EPSILON);
    EXPECT_NEAR(extractedAxis.z, axis.z, EPSILON);
}

// ==================== Quaternion Operations ====================

TEST(QuaternionTest, Multiplication) {
    // 90 degrees around Y + 90 degrees around Y = 180 degrees around Y
    glm::quat q1 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat q2 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    glm::quat combined = q2 * q1;
    
    // Rotate (1, 0, 0) by 180 degrees around Y should give (-1, 0, 0)
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    glm::vec3 result = combined * v;
    
    EXPECT_NEAR(result.x, -1.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, 0.0f, EPSILON);
}

TEST(QuaternionTest, Inverse) {
    glm::quat q = glm::angleAxis(glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::quat qInv = glm::inverse(q);
    
    glm::quat identity = q * qInv;
    
    EXPECT_NEAR(identity.w, 1.0f, EPSILON);
    EXPECT_NEAR(identity.x, 0.0f, EPSILON);
    EXPECT_NEAR(identity.y, 0.0f, EPSILON);
    EXPECT_NEAR(identity.z, 0.0f, EPSILON);
}

TEST(QuaternionTest, Conjugate) {
    glm::quat q(0.7071f, 0.7071f, 0.0f, 0.0f);
    glm::quat conj = glm::conjugate(q);
    
    EXPECT_FLOAT_EQ(conj.w, q.w);
    EXPECT_FLOAT_EQ(conj.x, -q.x);
    EXPECT_FLOAT_EQ(conj.y, -q.y);
    EXPECT_FLOAT_EQ(conj.z, -q.z);
}

// ==================== Quaternion Interpolation ====================

TEST(QuaternionTest, Slerp_Endpoints) {
    glm::quat q1 = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::quat q2 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // t = 0 should return q1
    glm::quat r0 = glm::slerp(q1, q2, 0.0f);
    EXPECT_NEAR(r0.w, q1.w, EPSILON);
    EXPECT_NEAR(r0.x, q1.x, EPSILON);
    EXPECT_NEAR(r0.y, q1.y, EPSILON);
    EXPECT_NEAR(r0.z, q1.z, EPSILON);
    
    // t = 1 should return q2
    glm::quat r1 = glm::slerp(q1, q2, 1.0f);
    EXPECT_NEAR(r1.w, q2.w, EPSILON);
    EXPECT_NEAR(r1.x, q2.x, EPSILON);
    EXPECT_NEAR(r1.y, q2.y, EPSILON);
    EXPECT_NEAR(r1.z, q2.z, EPSILON);
}

TEST(QuaternionTest, Slerp_Midpoint) {
    glm::quat q1 = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::quat q2 = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    glm::quat mid = glm::slerp(q1, q2, 0.5f);
    
    // Mid should represent 45 degree rotation
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    glm::vec3 rotated = mid * v;
    
    float expected = std::cos(glm::radians(45.0f));
    EXPECT_NEAR(rotated.x, expected, EPSILON);
    EXPECT_NEAR(rotated.z, -std::sin(glm::radians(45.0f)), EPSILON);
}

// ==================== Euler Angles ====================

TEST(QuaternionTest, FromEulerAngles) {
    glm::vec3 euler(glm::radians(30.0f), glm::radians(45.0f), glm::radians(60.0f));
    glm::quat q = glm::quat(euler);
    
    // Convert back to euler
    glm::vec3 backToEuler = glm::eulerAngles(q);
    
    EXPECT_NEAR(backToEuler.x, euler.x, EPSILON);
    EXPECT_NEAR(backToEuler.y, euler.y, EPSILON);
    EXPECT_NEAR(backToEuler.z, euler.z, EPSILON);
}

TEST(QuaternionTest, PitchYawRoll) {
    float pitch = glm::radians(30.0f);
    float yaw = glm::radians(45.0f);
    float roll = glm::radians(60.0f);
    
    glm::quat q = glm::quat(glm::vec3(pitch, yaw, roll));
    
    EXPECT_NEAR(glm::pitch(q), pitch, EPSILON);
    EXPECT_NEAR(glm::yaw(q), yaw, EPSILON);
    EXPECT_NEAR(glm::roll(q), roll, EPSILON);
}

// ==================== Matrix Conversion ====================

TEST(QuaternionTest, ToMatrix) {
    glm::quat q = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 m = glm::mat4_cast(q);
    
    // Transform using both
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    
    glm::vec3 fromQuat = q * v;
    glm::vec3 fromMat = glm::vec3(m * glm::vec4(v, 1.0f));
    
    EXPECT_NEAR(fromQuat.x, fromMat.x, EPSILON);
    EXPECT_NEAR(fromQuat.y, fromMat.y, EPSILON);
    EXPECT_NEAR(fromQuat.z, fromMat.z, EPSILON);
}

TEST(QuaternionTest, FromMatrix) {
    glm::mat4 m = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::quat q = glm::quat_cast(m);
    
    // Transform a vector
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    glm::vec3 result = q * v;
    
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 1.0f, EPSILON);
    EXPECT_NEAR(result.z, 0.0f, EPSILON);
}

// ==================== Edge Cases ====================

TEST(QuaternionTest, SmallAngle) {
    float smallAngle = 0.001f;
    glm::quat q = glm::angleAxis(smallAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    float extracted = glm::angle(q);
    EXPECT_NEAR(extracted, smallAngle, 1e-4f);
}

TEST(QuaternionTest, LargeAngle) {
    float largeAngle = glm::radians(359.0f);
    glm::quat q = glm::angleAxis(largeAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Should still rotate correctly
    glm::vec3 v(1.0f, 0.0f, 0.0f);
    glm::vec3 result = q * v;
    
    float expected = std::cos(largeAngle);
    EXPECT_NEAR(result.x, expected, 1e-4f);
}

TEST(QuaternionTest, DoubleRotation) {
    // q * q should equal rotation by 2*angle
    float angle = glm::radians(45.0f);
    glm::quat q = glm::angleAxis(angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat q2 = q * q;
    
    glm::quat expected = glm::angleAxis(2.0f * angle, glm::vec3(0.0f, 1.0f, 0.0f));
    
    EXPECT_NEAR(q2.w, expected.w, EPSILON);
    EXPECT_NEAR(q2.x, expected.x, EPSILON);
    EXPECT_NEAR(q2.y, expected.y, EPSILON);
    EXPECT_NEAR(q2.z, expected.z, EPSILON);
}
