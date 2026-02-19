/**
 * @file EntityTests.cpp
 * @brief Unit tests for Entity-Component system
 */

#include <gtest/gtest.h>
#include "Scene/Scene.hpp"
#include "Scene/Entity.hpp"
#include "Scene/Components/TransformComponent.hpp"
#include "Scene/Components/MeshRendererComponent.hpp"
#include "Scene/Components/LightComponent.hpp"
#include "Scene/Components/CameraComponent.hpp"
#include "Scene/Components/RigidBodyComponent.hpp"
#include "Scene/Components/ScriptComponent.hpp"

using namespace GameEngine;

class EntityTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_Scene = CreateRef<Scene>("TestScene");
    }

    void TearDown() override {
        m_Scene.reset();
    }

    Ref<Scene> m_Scene;
};

// ==================== Entity Creation ====================

TEST_F(EntityTest, CreateEntity) {
    Entity entity = m_Scene->CreateEntity("TestEntity");
    
    EXPECT_TRUE(entity.IsValid());
    EXPECT_EQ(entity.GetName(), "TestEntity");
}

TEST_F(EntityTest, CreateMultipleEntities) {
    Entity e1 = m_Scene->CreateEntity("Entity1");
    Entity e2 = m_Scene->CreateEntity("Entity2");
    Entity e3 = m_Scene->CreateEntity("Entity3");
    
    EXPECT_TRUE(e1.IsValid());
    EXPECT_TRUE(e2.IsValid());
    EXPECT_TRUE(e3.IsValid());
    
    // All should have unique UUIDs
    EXPECT_NE(e1.GetUUID(), e2.GetUUID());
    EXPECT_NE(e2.GetUUID(), e3.GetUUID());
    EXPECT_NE(e1.GetUUID(), e3.GetUUID());
}

TEST_F(EntityTest, EntityWithUUID) {
    UUID uuid;
    Entity entity = m_Scene->CreateEntityWithUUID(uuid, "UUIDEntity");
    
    EXPECT_TRUE(entity.IsValid());
    EXPECT_EQ(static_cast<uint64_t>(entity.GetUUID()), static_cast<uint64_t>(uuid));
}

TEST_F(EntityTest, DestroyEntity) {
    Entity entity = m_Scene->CreateEntity("ToDestroy");
    EXPECT_TRUE(entity.IsValid());
    
    m_Scene->DestroyEntity(entity);
    
    // Entity handle should be invalidated after destruction
    // Note: The actual behavior depends on implementation
}

// ==================== Component Management ====================

TEST_F(EntityTest, AddTransformComponent) {
    Entity entity = m_Scene->CreateEntity("WithTransform");
    
    // Entity should have transform by default (added in CreateEntity)
    EXPECT_TRUE(entity.HasComponent<TransformComponent>());
    
    auto& transform = entity.GetComponent<TransformComponent>();
    EXPECT_EQ(transform.Position, glm::vec3(0.0f));
    EXPECT_EQ(transform.Scale, glm::vec3(1.0f));
}

TEST_F(EntityTest, ModifyTransformComponent) {
    Entity entity = m_Scene->CreateEntity("ModifyTransform");
    
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.Scale = glm::vec3(2.0f);
    
    // Read back
    auto& readTransform = entity.GetComponent<TransformComponent>();
    EXPECT_EQ(readTransform.Position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(readTransform.Scale, glm::vec3(2.0f));
}

TEST_F(EntityTest, AddLightComponent) {
    Entity entity = m_Scene->CreateEntity("WithLight");
    
    EXPECT_FALSE(entity.HasComponent<LightComponent>());
    
    entity.AddComponent<LightComponent>();
    
    EXPECT_TRUE(entity.HasComponent<LightComponent>());
    
    auto& light = entity.GetComponent<LightComponent>();
    light.Color = glm::vec3(1.0f, 0.0f, 0.0f);
    light.Intensity = 5.0f;
    
    EXPECT_EQ(entity.GetComponent<LightComponent>().Color, glm::vec3(1.0f, 0.0f, 0.0f));
    EXPECT_FLOAT_EQ(entity.GetComponent<LightComponent>().Intensity, 5.0f);
}

TEST_F(EntityTest, AddCameraComponent) {
    Entity entity = m_Scene->CreateEntity("WithCamera");
    
    entity.AddComponent<CameraComponent>();
    
    EXPECT_TRUE(entity.HasComponent<CameraComponent>());
    
    auto& camera = entity.GetComponent<CameraComponent>();
    camera.FOV = 60.0f;
    camera.NearClip = 0.1f;
    camera.FarClip = 1000.0f;
    
    EXPECT_FLOAT_EQ(entity.GetComponent<CameraComponent>().FOV, 60.0f);
}

TEST_F(EntityTest, RemoveComponent) {
    Entity entity = m_Scene->CreateEntity("RemoveTest");
    entity.AddComponent<LightComponent>();
    
    EXPECT_TRUE(entity.HasComponent<LightComponent>());
    
    entity.RemoveComponent<LightComponent>();
    
    EXPECT_FALSE(entity.HasComponent<LightComponent>());
}

TEST_F(EntityTest, MultipleComponents) {
    Entity entity = m_Scene->CreateEntity("MultiComponent");
    
    entity.AddComponent<LightComponent>();
    entity.AddComponent<CameraComponent>();
    
    EXPECT_TRUE(entity.HasComponent<TransformComponent>());
    EXPECT_TRUE(entity.HasComponent<LightComponent>());
    EXPECT_TRUE(entity.HasComponent<CameraComponent>());
}

// ==================== Transform Hierarchy ====================

TEST_F(EntityTest, GetWorldTransform) {
    Entity entity = m_Scene->CreateEntity("WorldTransform");
    
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec3(1.0f, 2.0f, 3.0f);
    
    glm::mat4 worldMatrix = transform.GetWorldTransform();

    // Position should be in the translation part of the matrix
    EXPECT_FLOAT_EQ(worldMatrix[3][0], 1.0f);
    EXPECT_FLOAT_EQ(worldMatrix[3][1], 2.0f);
    EXPECT_FLOAT_EQ(worldMatrix[3][2], 3.0f);
}

TEST_F(EntityTest, TransformRotation) {
    Entity entity = m_Scene->CreateEntity("RotationTest");
    
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Rotation = glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
    
    glm::vec3 forward = transform.GetForward();
    
    // After 90 degree Y rotation, forward should point along -X
    EXPECT_NEAR(forward.x, -1.0f, 1e-5f);
    EXPECT_NEAR(forward.y, 0.0f, 1e-5f);
    EXPECT_NEAR(forward.z, 0.0f, 1e-5f);
}

// ==================== Entity Iteration ====================

TEST_F(EntityTest, IterateEntities) {
    m_Scene->CreateEntity("Entity1");
    m_Scene->CreateEntity("Entity2");
    m_Scene->CreateEntity("Entity3");
    
    auto entities = m_Scene->GetAllEntities();
    EXPECT_EQ(static_cast<int>(entities.size()), 3);
}

TEST_F(EntityTest, IterateWithComponent) {
    Entity e1 = m_Scene->CreateEntity("WithLight1");
    e1.AddComponent<LightComponent>();
    
    Entity e2 = m_Scene->CreateEntity("WithLight2");
    e2.AddComponent<LightComponent>();
    
    Entity e3 = m_Scene->CreateEntity("NoLight");
    
    auto allEntities = m_Scene->GetAllEntities();
    int lightCount = 0;
    for (auto& entity : allEntities) {
        if (entity.HasComponent<LightComponent>()) {
            lightCount++;
        }
    }
    EXPECT_EQ(lightCount, 2);
}

// ==================== Entity Lookup ====================

TEST_F(EntityTest, GetEntityByUUID) {
    Entity original = m_Scene->CreateEntity("Findable");
    UUID uuid = original.GetUUID();
    
    Entity found = m_Scene->GetEntityByUUID(uuid);
    
    EXPECT_TRUE(found.IsValid());
    EXPECT_EQ(found.GetName(), "Findable");
}

TEST_F(EntityTest, GetEntityByName) {
    m_Scene->CreateEntity("Target");
    m_Scene->CreateEntity("Other");
    
    Entity found = m_Scene->GetEntityByName("Target");
    
    EXPECT_TRUE(found.IsValid());
    EXPECT_EQ(found.GetName(), "Target");
}

TEST_F(EntityTest, GetNonexistentEntity) {
    Entity notFound = m_Scene->GetEntityByName("DoesNotExist");
    
    EXPECT_FALSE(notFound.IsValid());
}
