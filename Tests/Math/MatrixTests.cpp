/**
 * @file MatrixTests.cpp
 * @brief Unit tests for GLM matrix operations used in the engine
 */

#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

constexpr float EPSILON = 1e-5f;

// ==================== Mat3 Tests ====================

TEST(Mat3Test, Identity) {
    glm::mat3 m(1.0f);
    
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (i == j) {
                EXPECT_FLOAT_EQ(m[i][j], 1.0f);
            } else {
                EXPECT_FLOAT_EQ(m[i][j], 0.0f);
            }
        }
    }
}

TEST(Mat3Test, Determinant) {
    glm::mat3 m(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    );
    
    float det = glm::determinant(m);
    EXPECT_NEAR(det, 0.0f, EPSILON); // This matrix is singular
}

TEST(Mat3Test, Transpose) {
    glm::mat3 m(
        1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f,
        7.0f, 8.0f, 9.0f
    );
    
    glm::mat3 t = glm::transpose(m);
    
    EXPECT_FLOAT_EQ(t[0][1], m[1][0]);
    EXPECT_FLOAT_EQ(t[1][0], m[0][1]);
}

// ==================== Mat4 Tests ====================

TEST(Mat4Test, Identity) {
    glm::mat4 m(1.0f);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (i == j) {
                EXPECT_FLOAT_EQ(m[i][j], 1.0f);
            } else {
                EXPECT_FLOAT_EQ(m[i][j], 0.0f);
            }
        }
    }
}

TEST(Mat4Test, IdentityMultiplication) {
    glm::mat4 identity(1.0f);
    glm::mat4 m(
        1.0f, 2.0f, 3.0f, 4.0f,
        5.0f, 6.0f, 7.0f, 8.0f,
        9.0f, 10.0f, 11.0f, 12.0f,
        13.0f, 14.0f, 15.0f, 16.0f
    );
    
    glm::mat4 result = identity * m;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            EXPECT_FLOAT_EQ(result[i][j], m[i][j]);
        }
    }
}

TEST(Mat4Test, Translation) {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::vec4 point(0.0f, 0.0f, 0.0f, 1.0f);
    
    glm::vec4 result = t * point;
    
    EXPECT_FLOAT_EQ(result.x, 1.0f);
    EXPECT_FLOAT_EQ(result.y, 2.0f);
    EXPECT_FLOAT_EQ(result.z, 3.0f);
    EXPECT_FLOAT_EQ(result.w, 1.0f);
}

TEST(Mat4Test, Scale) {
    glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 4.0f));
    glm::vec4 point(1.0f, 1.0f, 1.0f, 1.0f);
    
    glm::vec4 result = s * point;
    
    EXPECT_FLOAT_EQ(result.x, 2.0f);
    EXPECT_FLOAT_EQ(result.y, 3.0f);
    EXPECT_FLOAT_EQ(result.z, 4.0f);
}

TEST(Mat4Test, RotationX) {
    float angle = glm::radians(90.0f);
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec4 point(0.0f, 1.0f, 0.0f, 1.0f);
    
    glm::vec4 result = r * point;
    
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, 1.0f, EPSILON);
}

TEST(Mat4Test, RotationY) {
    float angle = glm::radians(90.0f);
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec4 point(1.0f, 0.0f, 0.0f, 1.0f);
    
    glm::vec4 result = r * point;
    
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 0.0f, EPSILON);
    EXPECT_NEAR(result.z, -1.0f, EPSILON);
}

TEST(Mat4Test, RotationZ) {
    float angle = glm::radians(90.0f);
    glm::mat4 r = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::vec4 point(1.0f, 0.0f, 0.0f, 1.0f);
    
    glm::vec4 result = r * point;
    
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 1.0f, EPSILON);
    EXPECT_NEAR(result.z, 0.0f, EPSILON);
}

TEST(Mat4Test, Inverse) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::mat4 inv = glm::inverse(m);
    glm::mat4 identity = m * inv;
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            float expected = (i == j) ? 1.0f : 0.0f;
            EXPECT_NEAR(identity[i][j], expected, EPSILON);
        }
    }
}

TEST(Mat4Test, LookAt) {
    glm::vec3 eye(0.0f, 0.0f, 5.0f);
    glm::vec3 center(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    
    glm::mat4 view = glm::lookAt(eye, center, up);
    
    // Transform eye position - should be at origin in view space
    glm::vec4 eyeInView = view * glm::vec4(eye, 1.0f);
    EXPECT_NEAR(eyeInView.x, 0.0f, EPSILON);
    EXPECT_NEAR(eyeInView.y, 0.0f, EPSILON);
    EXPECT_NEAR(eyeInView.z, 0.0f, EPSILON);
}

TEST(Mat4Test, Perspective) {
    float fov = glm::radians(45.0f);
    float aspect = 16.0f / 9.0f;
    float near = 0.1f;
    float far = 100.0f;
    
    glm::mat4 proj = glm::perspective(fov, aspect, near, far);
    
    // Test that near plane maps correctly
    glm::vec4 nearPoint(0.0f, 0.0f, -near, 1.0f);
    glm::vec4 projNear = proj * nearPoint;
    projNear /= projNear.w;
    EXPECT_NEAR(projNear.z, -1.0f, EPSILON);
    
    // Test that far plane maps correctly
    glm::vec4 farPoint(0.0f, 0.0f, -far, 1.0f);
    glm::vec4 projFar = proj * farPoint;
    projFar /= projFar.w;
    EXPECT_NEAR(projFar.z, 1.0f, EPSILON);
}

TEST(Mat4Test, Orthographic) {
    float left = -10.0f, right = 10.0f;
    float bottom = -10.0f, top = 10.0f;
    float near = -1.0f, far = 1.0f;
    
    glm::mat4 ortho = glm::ortho(left, right, bottom, top, near, far);
    
    // Center should map to origin
    glm::vec4 center(0.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 projCenter = ortho * center;
    EXPECT_NEAR(projCenter.x, 0.0f, EPSILON);
    EXPECT_NEAR(projCenter.y, 0.0f, EPSILON);
    EXPECT_NEAR(projCenter.z, 0.0f, EPSILON);
}

// ==================== Transform Composition ====================

TEST(Mat4Test, TRS_Order) {
    glm::vec3 translation(1.0f, 2.0f, 3.0f);
    glm::vec3 scale(2.0f, 2.0f, 2.0f);
    float angle = glm::radians(45.0f);
    
    // Correct order: T * R * S
    glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
    glm::mat4 R = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    
    glm::mat4 TRS = T * R * S;
    
    // Test point transformation
    glm::vec4 point(1.0f, 0.0f, 0.0f, 1.0f);
    glm::vec4 result = TRS * point;
    
    // Expected: scale first (2, 0, 0), then rotate 45 deg around Y, then translate
    float cos45 = std::cos(angle);
    float sin45 = std::sin(angle);
    float expectedX = 2.0f * cos45 + translation.x;
    float expectedY = translation.y;
    float expectedZ = -2.0f * sin45 + translation.z;
    
    EXPECT_NEAR(result.x, expectedX, EPSILON);
    EXPECT_NEAR(result.y, expectedY, EPSILON);
    EXPECT_NEAR(result.z, expectedZ, EPSILON);
}
