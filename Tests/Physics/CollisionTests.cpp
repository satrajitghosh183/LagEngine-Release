/**
 * @file CollisionTests.cpp
 * @brief Unit tests for collision detection systems
 */

#include <gtest/gtest.h>
#include "Physics/Collision/CollisionDetector.hpp"
#include "Physics/Shapes/SphereShape.hpp"
#include "Physics/Shapes/BoxShape.hpp"
#include "Physics/Shapes/CapsuleShape.hpp"
#include "Physics/Shapes/PlaneShape.hpp"
#include "Physics/Collision/ContactManifold.hpp"
#include "Physics/RigidBody.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace GameEngine;
using namespace GameEngine::Physics;

constexpr float EPSILON = 1e-5f;

// Helper to create a ContactManifold for tests (needs two bodies)
static RigidBody s_DummyBodyA;
static RigidBody s_DummyBodyB;

// ==================== Sphere-Sphere Collision ====================

TEST(CollisionTest, SphereSphere_NoCollision) {
    auto sphere1 = CreateRef<SphereShape>(1.0f);
    auto sphere2 = CreateRef<SphereShape>(1.0f);

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(5.0f, 0.0f, 0.0f);  // 5 units apart, radii sum = 2
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere1, pos1, rot,
        sphere2, pos2, rot,
        manifold
    );

    EXPECT_FALSE(colliding);
    EXPECT_EQ(manifold.GetContactCount(), 0u);
}

TEST(CollisionTest, SphereSphere_Collision) {
    auto sphere1 = CreateRef<SphereShape>(1.0f);
    auto sphere2 = CreateRef<SphereShape>(1.0f);

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(1.5f, 0.0f, 0.0f);  // 1.5 units apart, radii sum = 2
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere1, pos1, rot,
        sphere2, pos2, rot,
        manifold
    );

    EXPECT_TRUE(colliding);
    EXPECT_GT(manifold.GetContactCount(), 0u);

    // Check contact details
    const auto& contacts = manifold.GetContacts();
    ASSERT_GT(contacts.size(), 0u);

    // Penetration should be approximately 0.5 (2.0 - 1.5)
    EXPECT_NEAR(contacts[0].Penetration, 0.5f, EPSILON);

    // Normal should point from sphere1 to sphere2
    EXPECT_NEAR(contacts[0].Normal.x, 1.0f, EPSILON);
    EXPECT_NEAR(contacts[0].Normal.y, 0.0f, EPSILON);
    EXPECT_NEAR(contacts[0].Normal.z, 0.0f, EPSILON);
}

TEST(CollisionTest, SphereSphere_Touching) {
    auto sphere1 = CreateRef<SphereShape>(1.0f);
    auto sphere2 = CreateRef<SphereShape>(1.0f);

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(2.0f, 0.0f, 0.0f);  // Exactly touching
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere1, pos1, rot,
        sphere2, pos2, rot,
        manifold
    );

    // Touching is considered a collision with zero penetration
    EXPECT_TRUE(colliding);
    if (manifold.GetContactCount() > 0) {
        EXPECT_NEAR(manifold.GetContacts()[0].Penetration, 0.0f, EPSILON);
    }
}

// ==================== Sphere-Box Collision ====================

TEST(CollisionTest, SphereBox_NoCollision) {
    auto sphere = CreateRef<SphereShape>(1.0f);
    auto box = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));  // 2x2x2 box

    glm::vec3 spherePos(5.0f, 0.0f, 0.0f);
    glm::vec3 boxPos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere, spherePos, rot,
        box, boxPos, rot,
        manifold
    );

    EXPECT_FALSE(colliding);
}

TEST(CollisionTest, SphereBox_Collision_Face) {
    auto sphere = CreateRef<SphereShape>(1.0f);
    auto box = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));  // 2x2x2 box

    glm::vec3 spherePos(1.5f, 0.0f, 0.0f);  // Sphere center at 1.5, box extends to 1.0
    glm::vec3 boxPos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere, spherePos, rot,
        box, boxPos, rot,
        manifold
    );

    EXPECT_TRUE(colliding);
    EXPECT_GT(manifold.GetContactCount(), 0u);

    // Normal should point along +X
    if (manifold.GetContactCount() > 0) {
        EXPECT_NEAR(std::abs(manifold.GetContacts()[0].Normal.x), 1.0f, EPSILON);
    }
}

// ==================== Box-Box Collision ====================

TEST(CollisionTest, BoxBox_NoCollision) {
    auto box1 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));
    auto box2 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(5.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        box1, pos1, rot,
        box2, pos2, rot,
        manifold
    );

    EXPECT_FALSE(colliding);
}

TEST(CollisionTest, BoxBox_Collision_FaceToFace) {
    auto box1 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));
    auto box2 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(1.5f, 0.0f, 0.0f);  // Overlapping by 0.5
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        box1, pos1, rot,
        box2, pos2, rot,
        manifold
    );

    EXPECT_TRUE(colliding);
    EXPECT_GT(manifold.GetContactCount(), 0u);
    if (manifold.GetContactCount() > 0) {
        EXPECT_NEAR(manifold.GetContacts()[0].Penetration, 0.5f, EPSILON);
    }
}

TEST(CollisionTest, BoxBox_RotatedCollision) {
    auto box1 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));
    auto box2 = CreateRef<BoxShape>(glm::vec3(1.0f, 1.0f, 1.0f));

    glm::vec3 pos1(0.0f, 0.0f, 0.0f);
    glm::vec3 pos2(2.0f, 0.0f, 0.0f);
    glm::quat rot1 = glm::quat(1, 0, 0, 0);
    glm::quat rot2 = glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        box1, pos1, rot1,
        box2, pos2, rot2,
        manifold
    );

    // 45-degree rotated box corner should reach further
    // At 45 degrees, the box extends sqrt(2) ~ 1.414 instead of 1.0
    // So total reach is 2 + 1.414 ~ 3.414, and boxes are 2 apart
    // This should collide
    EXPECT_TRUE(colliding);
}

// ==================== Sphere-Plane Collision ====================

TEST(CollisionTest, SpherePlane_NoCollision) {
    auto sphere = CreateRef<SphereShape>(1.0f);
    auto plane = CreateRef<PlaneShape>(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);  // Y = 0 plane

    glm::vec3 spherePos(0.0f, 5.0f, 0.0f);  // Well above plane
    glm::vec3 planePos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere, spherePos, rot,
        plane, planePos, rot,
        manifold
    );

    EXPECT_FALSE(colliding);
}

TEST(CollisionTest, SpherePlane_Collision) {
    auto sphere = CreateRef<SphereShape>(1.0f);
    auto plane = CreateRef<PlaneShape>(glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);

    glm::vec3 spherePos(0.0f, 0.5f, 0.0f);  // Center at 0.5, radius 1.0
    glm::vec3 planePos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        sphere, spherePos, rot,
        plane, planePos, rot,
        manifold
    );

    EXPECT_TRUE(colliding);
    if (manifold.GetContactCount() > 0) {
        EXPECT_NEAR(manifold.GetContacts()[0].Penetration, 0.5f, EPSILON);
        // Normal should point up (plane normal)
        EXPECT_NEAR(manifold.GetContacts()[0].Normal.y, 1.0f, EPSILON);
    }
}

// ==================== Capsule Collision ====================

TEST(CollisionTest, CapsuleSphere_NoCollision) {
    auto capsule = CreateRef<CapsuleShape>(1.0f, 2.0f);  // Radius 1, height 2
    auto sphere = CreateRef<SphereShape>(1.0f);

    glm::vec3 capsulePos(0.0f, 0.0f, 0.0f);
    glm::vec3 spherePos(5.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        capsule, capsulePos, rot,
        sphere, spherePos, rot,
        manifold
    );

    EXPECT_FALSE(colliding);
}

TEST(CollisionTest, CapsuleSphere_Collision) {
    auto capsule = CreateRef<CapsuleShape>(1.0f, 2.0f);
    auto sphere = CreateRef<SphereShape>(1.0f);

    glm::vec3 capsulePos(0.0f, 0.0f, 0.0f);
    glm::vec3 spherePos(1.5f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    ContactManifold manifold(&s_DummyBodyA, &s_DummyBodyB);
    bool colliding = CollisionDetector::DetectCollision(
        capsule, capsulePos, rot,
        sphere, spherePos, rot,
        manifold
    );

    EXPECT_TRUE(colliding);
}

// ==================== AABB Tests ====================

TEST(CollisionTest, AABBOverlap) {
    glm::vec3 min1(-1.0f, -1.0f, -1.0f);
    glm::vec3 max1(1.0f, 1.0f, 1.0f);

    glm::vec3 min2(0.5f, 0.5f, 0.5f);
    glm::vec3 max2(2.0f, 2.0f, 2.0f);

    // AABB overlap: check all axes
    bool overlaps = (min1.x <= max2.x && max1.x >= min2.x) &&
                    (min1.y <= max2.y && max1.y >= min2.y) &&
                    (min1.z <= max2.z && max1.z >= min2.z);
    EXPECT_TRUE(overlaps);
}

TEST(CollisionTest, AABBNoOverlap) {
    glm::vec3 min1(-1.0f, -1.0f, -1.0f);
    glm::vec3 max1(1.0f, 1.0f, 1.0f);

    glm::vec3 min2(2.0f, 2.0f, 2.0f);
    glm::vec3 max2(3.0f, 3.0f, 3.0f);

    bool overlaps = (min1.x <= max2.x && max1.x >= min2.x) &&
                    (min1.y <= max2.y && max1.y >= min2.y) &&
                    (min1.z <= max2.z && max1.z >= min2.z);
    EXPECT_FALSE(overlaps);
}

// ==================== Ray Casting ====================

TEST(CollisionTest, RaySphere_Hit) {
    auto sphere = CreateRef<SphereShape>(1.0f);

    glm::vec3 rayOrigin(-5.0f, 0.0f, 0.0f);
    glm::vec3 rayDir(1.0f, 0.0f, 0.0f);
    glm::vec3 spherePos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    // Use shape's Raycast method directly
    auto hit = sphere->Raycast(rayOrigin, rayDir, spherePos, rot, 100.0f);

    EXPECT_TRUE(hit.Hit);
    EXPECT_NEAR(hit.Distance, 4.0f, EPSILON);  // Hit at distance 4 (5 - 1)
    EXPECT_NEAR(hit.Point.x, -1.0f, EPSILON);
    EXPECT_NEAR(hit.Normal.x, -1.0f, EPSILON);  // Normal points towards ray
}

TEST(CollisionTest, RaySphere_Miss) {
    auto sphere = CreateRef<SphereShape>(1.0f);

    glm::vec3 rayOrigin(-5.0f, 5.0f, 0.0f);  // Above sphere
    glm::vec3 rayDir(1.0f, 0.0f, 0.0f);
    glm::vec3 spherePos(0.0f, 0.0f, 0.0f);
    glm::quat rot = glm::quat(1, 0, 0, 0);

    auto hit = sphere->Raycast(rayOrigin, rayDir, spherePos, rot, 100.0f);

    EXPECT_FALSE(hit.Hit);
}
