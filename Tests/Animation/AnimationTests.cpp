/**
 * @file AnimationTests.cpp
 * @brief Unit tests for Animation system
 */

#include <gtest/gtest.h>
#include "Animation/AnimationClip.hpp"
#include "Animation/Animator.hpp"
#include "Animation/Skeleton.hpp"
#include "Animation/Bone.hpp"
#include "Core/Base.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace GameEngine;

constexpr float EPSILON = 1e-5f;

// ==================== Bone Tests ====================

TEST(BoneTest, DefaultConstruction) {
    Bone bone;

    EXPECT_EQ(bone.Name, "");
    EXPECT_EQ(bone.ParentID, -1);
}

TEST(BoneTest, NamedConstruction) {
    Bone bone;
    bone.Name = "TestBone";
    bone.ParentID = 0;

    EXPECT_EQ(bone.Name, "TestBone");
    EXPECT_EQ(bone.ParentID, 0);
}

TEST(BoneTest, LocalTransform) {
    Bone bone;
    bone.Name = "TestBone";
    bone.ParentID = -1;

    glm::vec3 position(1.0f, 2.0f, 3.0f);
    glm::quat rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 1, 0));
    glm::vec3 scale(1.0f);

    bone.LocalTransform = glm::translate(glm::mat4(1.0f), position) *
                          glm::mat4_cast(rotation) *
                          glm::scale(glm::mat4(1.0f), scale);

    glm::vec3 extractedPos = glm::vec3(bone.LocalTransform[3]);
    EXPECT_NEAR(extractedPos.x, position.x, EPSILON);
    EXPECT_NEAR(extractedPos.y, position.y, EPSILON);
    EXPECT_NEAR(extractedPos.z, position.z, EPSILON);
}

// ==================== Skeleton Tests ====================

TEST(SkeletonTest, CreateSkeleton) {
    auto skeleton = CreateRef<Skeleton>();

    Bone root;
    root.Name = "Root";
    root.ParentID = -1;
    skeleton->AddBone(root);

    Bone spine;
    spine.Name = "Spine";
    spine.ParentID = 0;
    skeleton->AddBone(spine);

    Bone head;
    head.Name = "Head";
    head.ParentID = 1;
    skeleton->AddBone(head);

    EXPECT_EQ(skeleton->GetBoneCount(), 3u);
}

TEST(SkeletonTest, FindBone) {
    auto skeleton = CreateRef<Skeleton>();

    Bone root;
    root.Name = "Root";
    root.ParentID = -1;
    skeleton->AddBone(root);

    Bone spine;
    spine.Name = "Spine";
    spine.ParentID = 0;
    skeleton->AddBone(spine);

    Bone* rootBone = skeleton->GetBone("Root");
    Bone* spineBone = skeleton->GetBone("Spine");
    Bone* notFound = skeleton->GetBone("NonExistent");

    EXPECT_NE(rootBone, nullptr);
    EXPECT_NE(spineBone, nullptr);
    EXPECT_EQ(notFound, nullptr);
    if (rootBone) EXPECT_EQ(rootBone->Name, "Root");
    if (spineBone) EXPECT_EQ(spineBone->Name, "Spine");
}

TEST(SkeletonTest, BoneHierarchy) {
    auto skeleton = CreateRef<Skeleton>();

    Bone root;
    root.Name = "Root";
    root.ParentID = -1;
    skeleton->AddBone(root);

    Bone leftArm;
    leftArm.Name = "LeftArm";
    leftArm.ParentID = 0;
    skeleton->AddBone(leftArm);

    Bone rightArm;
    rightArm.Name = "RightArm";
    rightArm.ParentID = 0;
    skeleton->AddBone(rightArm);

    Bone leftHand;
    leftHand.Name = "LeftHand";
    leftHand.ParentID = 1;
    skeleton->AddBone(leftHand);

    Bone& leftArmRef = skeleton->GetBone(1);
    EXPECT_EQ(leftArmRef.ParentID, 0);

    Bone& leftHandRef = skeleton->GetBone(3);
    EXPECT_EQ(leftHandRef.ParentID, 1);
}

// ==================== Animation Clip Tests ====================

TEST(AnimationClipTest, Construction) {
    auto clip = CreateRef<AnimationClip>("TestClip", 2.5f, 30.0f);

    EXPECT_EQ(clip->GetName(), "TestClip");
    EXPECT_FLOAT_EQ(clip->GetDuration(), 2.5f);
    EXPECT_FLOAT_EQ(clip->GetTicksPerSecond(), 30.0f);
}

// ==================== Keyframe Tests ====================

TEST(KeyframeTest, PositionKeyframe) {
    struct PositionKey {
        float Time;
        glm::vec3 Value;
    };

    PositionKey key1{0.0f, glm::vec3(0.0f)};
    PositionKey key2{1.0f, glm::vec3(1.0f, 0.0f, 0.0f)};

    // Interpolation at t=0.5
    float t = 0.5f;
    glm::vec3 interpolated = glm::mix(key1.Value, key2.Value, t);

    EXPECT_NEAR(interpolated.x, 0.5f, EPSILON);
}

TEST(KeyframeTest, RotationKeyframe) {
    struct RotationKey {
        float Time;
        glm::quat Value;
    };

    RotationKey key1{0.0f, glm::quat(1, 0, 0, 0)};
    RotationKey key2{1.0f, glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0))};

    // Slerp at t=0.5
    float t = 0.5f;
    glm::quat interpolated = glm::slerp(key1.Value, key2.Value, t);

    // At t=0.5, should be 45 degree rotation
    EXPECT_NEAR(glm::angle(interpolated), glm::radians(45.0f), 0.01f);
}

// ==================== Animator Tests ====================

TEST(AnimatorTest, ConstructionAndMembers) {
    auto skeleton = CreateRef<Skeleton>();
    Bone root;
    root.Name = "Root";
    root.ParentID = -1;
    skeleton->AddBone(root);

    Animator animator(skeleton);

    EXPECT_FALSE(animator.IsPlaying);
    EXPECT_FLOAT_EQ(animator.PlaybackSpeed, 1.0f);
}

TEST(AnimatorTest, PlaybackSpeed) {
    auto skeleton = CreateRef<Skeleton>();
    Bone root;
    root.Name = "Root";
    root.ParentID = -1;
    skeleton->AddBone(root);

    Animator animator(skeleton);

    animator.PlaybackSpeed = 2.0f;
    EXPECT_FLOAT_EQ(animator.PlaybackSpeed, 2.0f);

    animator.PlaybackSpeed = 0.5f;
    EXPECT_FLOAT_EQ(animator.PlaybackSpeed, 0.5f);
}

// ==================== Animation Blending ====================

TEST(AnimationBlendTest, LinearBlend) {
    // Simple linear blend between two poses
    glm::vec3 poseA(0.0f, 0.0f, 0.0f);
    glm::vec3 poseB(10.0f, 0.0f, 0.0f);

    float blendFactor = 0.3f;
    glm::vec3 blended = glm::mix(poseA, poseB, blendFactor);

    EXPECT_NEAR(blended.x, 3.0f, EPSILON);
}

TEST(AnimationBlendTest, QuaternionBlend) {
    glm::quat quatA = glm::quat(1, 0, 0, 0);  // Identity
    glm::quat quatB = glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0));

    glm::quat blended = glm::slerp(quatA, quatB, 0.5f);

    // Result should be normalized
    float length = glm::length(blended);
    EXPECT_NEAR(length, 1.0f, EPSILON);
}

// ==================== Bone Matrix Calculation ====================

TEST(BoneMatrixTest, LocalToWorld) {
    // Create simple hierarchy: Root -> Child
    glm::mat4 rootLocal = glm::translate(glm::mat4(1.0f), glm::vec3(0, 1, 0));
    glm::mat4 childLocal = glm::translate(glm::mat4(1.0f), glm::vec3(1, 0, 0));

    // Child world = Root world * Child local
    glm::mat4 childWorld = rootLocal * childLocal;

    // Child should be at (1, 1, 0) in world space
    glm::vec4 childPos = childWorld * glm::vec4(0, 0, 0, 1);
    EXPECT_NEAR(childPos.x, 1.0f, EPSILON);
    EXPECT_NEAR(childPos.y, 1.0f, EPSILON);
}
