/**
 * @file VectorTests.cpp
 * @brief Unit tests for GLM vector operations used in the engine
 */

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

constexpr float EPSILON = 1e-5f;

// ==================== Vec2 Tests ====================

TEST(Vec2Test, DefaultConstructor) {
    glm::vec2 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
}

TEST(Vec2Test, ParameterizedConstructor) {
    glm::vec2 v(1.0f, 2.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
}

TEST(Vec2Test, Addition) {
    glm::vec2 a(1.0f, 2.0f);
    glm::vec2 b(3.0f, 4.0f);
    glm::vec2 c = a + b;
    EXPECT_FLOAT_EQ(c.x, 4.0f);
    EXPECT_FLOAT_EQ(c.y, 6.0f);
}

TEST(Vec2Test, Subtraction) {
    glm::vec2 a(5.0f, 7.0f);
    glm::vec2 b(2.0f, 3.0f);
    glm::vec2 c = a - b;
    EXPECT_FLOAT_EQ(c.x, 3.0f);
    EXPECT_FLOAT_EQ(c.y, 4.0f);
}

TEST(Vec2Test, ScalarMultiplication) {
    glm::vec2 v(2.0f, 3.0f);
    glm::vec2 r = v * 2.0f;
    EXPECT_FLOAT_EQ(r.x, 4.0f);
    EXPECT_FLOAT_EQ(r.y, 6.0f);
}

TEST(Vec2Test, DotProduct) {
    glm::vec2 a(1.0f, 0.0f);
    glm::vec2 b(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(glm::dot(a, b), 0.0f); // Perpendicular
    
    glm::vec2 c(1.0f, 2.0f);
    glm::vec2 d(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(glm::dot(c, d), 11.0f); // 1*3 + 2*4
}

TEST(Vec2Test, Length) {
    glm::vec2 v(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(glm::length(v), 5.0f);
}

TEST(Vec2Test, Normalize) {
    glm::vec2 v(3.0f, 4.0f);
    glm::vec2 n = glm::normalize(v);
    EXPECT_NEAR(glm::length(n), 1.0f, EPSILON);
    EXPECT_FLOAT_EQ(n.x, 0.6f);
    EXPECT_FLOAT_EQ(n.y, 0.8f);
}

// ==================== Vec3 Tests ====================

TEST(Vec3Test, DefaultConstructor) {
    glm::vec3 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
}

TEST(Vec3Test, ParameterizedConstructor) {
    glm::vec3 v(1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(v.x, 1.0f);
    EXPECT_FLOAT_EQ(v.y, 2.0f);
    EXPECT_FLOAT_EQ(v.z, 3.0f);
}

TEST(Vec3Test, CrossProduct) {
    glm::vec3 x(1.0f, 0.0f, 0.0f);
    glm::vec3 y(0.0f, 1.0f, 0.0f);
    glm::vec3 z = glm::cross(x, y);
    
    EXPECT_FLOAT_EQ(z.x, 0.0f);
    EXPECT_FLOAT_EQ(z.y, 0.0f);
    EXPECT_FLOAT_EQ(z.z, 1.0f);
}

TEST(Vec3Test, CrossProductAnticommutative) {
    glm::vec3 a(1.0f, 2.0f, 3.0f);
    glm::vec3 b(4.0f, 5.0f, 6.0f);
    
    glm::vec3 ab = glm::cross(a, b);
    glm::vec3 ba = glm::cross(b, a);
    
    EXPECT_NEAR(ab.x, -ba.x, EPSILON);
    EXPECT_NEAR(ab.y, -ba.y, EPSILON);
    EXPECT_NEAR(ab.z, -ba.z, EPSILON);
}

TEST(Vec3Test, Length) {
    glm::vec3 v(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(glm::length(v), 3.0f);
}

TEST(Vec3Test, Normalize) {
    glm::vec3 v(1.0f, 2.0f, 2.0f);
    glm::vec3 n = glm::normalize(v);
    EXPECT_NEAR(glm::length(n), 1.0f, EPSILON);
}

TEST(Vec3Test, Distance) {
    glm::vec3 a(0.0f, 0.0f, 0.0f);
    glm::vec3 b(1.0f, 2.0f, 2.0f);
    EXPECT_FLOAT_EQ(glm::distance(a, b), 3.0f);
}

TEST(Vec3Test, Reflect) {
    glm::vec3 incident(1.0f, -1.0f, 0.0f);
    glm::vec3 normal(0.0f, 1.0f, 0.0f);
    glm::vec3 reflected = glm::reflect(glm::normalize(incident), normal);
    
    EXPECT_NEAR(reflected.y, -incident.y / glm::length(incident), EPSILON);
}

// ==================== Vec4 Tests ====================

TEST(Vec4Test, DefaultConstructor) {
    glm::vec4 v;
    EXPECT_FLOAT_EQ(v.x, 0.0f);
    EXPECT_FLOAT_EQ(v.y, 0.0f);
    EXPECT_FLOAT_EQ(v.z, 0.0f);
    EXPECT_FLOAT_EQ(v.w, 0.0f);
}

TEST(Vec4Test, FromVec3) {
    glm::vec3 v3(1.0f, 2.0f, 3.0f);
    glm::vec4 v4(v3, 1.0f);
    
    EXPECT_FLOAT_EQ(v4.x, 1.0f);
    EXPECT_FLOAT_EQ(v4.y, 2.0f);
    EXPECT_FLOAT_EQ(v4.z, 3.0f);
    EXPECT_FLOAT_EQ(v4.w, 1.0f);
}

TEST(Vec4Test, Swizzle) {
    glm::vec4 v(1.0f, 2.0f, 3.0f, 4.0f);
    glm::vec3 xyz = glm::vec3(v);
    
    EXPECT_FLOAT_EQ(xyz.x, 1.0f);
    EXPECT_FLOAT_EQ(xyz.y, 2.0f);
    EXPECT_FLOAT_EQ(xyz.z, 3.0f);
}

// ==================== Common Operations ====================

TEST(VectorTest, Lerp) {
    glm::vec3 a(0.0f, 0.0f, 0.0f);
    glm::vec3 b(10.0f, 10.0f, 10.0f);
    
    glm::vec3 mid = glm::mix(a, b, 0.5f);
    EXPECT_FLOAT_EQ(mid.x, 5.0f);
    EXPECT_FLOAT_EQ(mid.y, 5.0f);
    EXPECT_FLOAT_EQ(mid.z, 5.0f);
    
    glm::vec3 quarter = glm::mix(a, b, 0.25f);
    EXPECT_FLOAT_EQ(quarter.x, 2.5f);
}

TEST(VectorTest, Clamp) {
    glm::vec3 v(15.0f, -5.0f, 5.0f);
    glm::vec3 clamped = glm::clamp(v, glm::vec3(0.0f), glm::vec3(10.0f));
    
    EXPECT_FLOAT_EQ(clamped.x, 10.0f);
    EXPECT_FLOAT_EQ(clamped.y, 0.0f);
    EXPECT_FLOAT_EQ(clamped.z, 5.0f);
}

TEST(VectorTest, Min) {
    glm::vec3 a(1.0f, 5.0f, 3.0f);
    glm::vec3 b(2.0f, 3.0f, 4.0f);
    glm::vec3 m = glm::min(a, b);
    
    EXPECT_FLOAT_EQ(m.x, 1.0f);
    EXPECT_FLOAT_EQ(m.y, 3.0f);
    EXPECT_FLOAT_EQ(m.z, 3.0f);
}

TEST(VectorTest, Max) {
    glm::vec3 a(1.0f, 5.0f, 3.0f);
    glm::vec3 b(2.0f, 3.0f, 4.0f);
    glm::vec3 m = glm::max(a, b);
    
    EXPECT_FLOAT_EQ(m.x, 2.0f);
    EXPECT_FLOAT_EQ(m.y, 5.0f);
    EXPECT_FLOAT_EQ(m.z, 4.0f);
}
