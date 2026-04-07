/**
 * @file SerializationTests.cpp
 * @brief Unit tests for Scene serialization and deserialization
 */

#include <gtest/gtest.h>
#include "Scene/Scene.hpp"
#include "Scene/Entity.hpp"
#include "Scene/SceneSerializer.hpp"
#include "Scene/Components/TransformComponent.hpp"
#include "Scene/Components/LightComponent.hpp"
#include "Scene/Components/CameraComponent.hpp"
#include <filesystem>
#include <fstream>

using namespace GameEngine;

class SerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_Scene = CreateRef<Scene>("TestScene");
        m_TestFilePath = "test_scene.json";
    }

    void TearDown() override {
        m_Scene.reset();
        // Clean up test file
        if (std::filesystem::exists(m_TestFilePath)) {
            std::filesystem::remove(m_TestFilePath);
        }
    }

    Ref<Scene> m_Scene;
    std::string m_TestFilePath;
};

// ==================== Basic Serialization ====================

TEST_F(SerializationTest, SerializeEmptyScene) {
    SceneSerializer serializer(m_Scene);
    EXPECT_NO_THROW(serializer.Serialize(m_TestFilePath));
    EXPECT_TRUE(std::filesystem::exists(m_TestFilePath));
}

TEST_F(SerializationTest, DeserializeEmptyScene) {
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    
    EXPECT_TRUE(loadSerializer.Deserialize(m_TestFilePath));
}

// ==================== Entity Serialization ====================

TEST_F(SerializationTest, SerializeSingleEntity) {
    Entity entity = m_Scene->CreateEntity("TestEntity");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec3(1.0f, 2.0f, 3.0f);
    transform.Scale = glm::vec3(2.0f, 2.0f, 2.0f);
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    Entity loadedEntity = loadedScene->GetEntityByName("TestEntity");
    EXPECT_TRUE(loadedEntity.IsValid());
    
    auto& loadedTransform = loadedEntity.GetComponent<TransformComponent>();
    EXPECT_EQ(loadedTransform.Position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(loadedTransform.Scale, glm::vec3(2.0f, 2.0f, 2.0f));
}

TEST_F(SerializationTest, SerializeMultipleEntities) {
    m_Scene->CreateEntity("Entity1");
    m_Scene->CreateEntity("Entity2");
    m_Scene->CreateEntity("Entity3");
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    EXPECT_TRUE(loadedScene->GetEntityByName("Entity1").IsValid());
    EXPECT_TRUE(loadedScene->GetEntityByName("Entity2").IsValid());
    EXPECT_TRUE(loadedScene->GetEntityByName("Entity3").IsValid());
}

// ==================== Component Serialization ====================

TEST_F(SerializationTest, SerializeTransformComponent) {
    Entity entity = m_Scene->CreateEntity("TransformTest");
    auto& transform = entity.GetComponent<TransformComponent>();
    
    transform.Position = glm::vec3(10.0f, 20.0f, 30.0f);
    transform.Rotation = glm::quat(glm::vec3(0.1f, 0.2f, 0.3f));
    transform.Scale = glm::vec3(1.5f, 2.5f, 3.5f);
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    Entity loadedEntity = loadedScene->GetEntityByName("TransformTest");
    auto& loadedTransform = loadedEntity.GetComponent<TransformComponent>();
    
    EXPECT_NEAR(loadedTransform.Position.x, 10.0f, 1e-5f);
    EXPECT_NEAR(loadedTransform.Position.y, 20.0f, 1e-5f);
    EXPECT_NEAR(loadedTransform.Position.z, 30.0f, 1e-5f);
    
    EXPECT_NEAR(loadedTransform.Scale.x, 1.5f, 1e-5f);
    EXPECT_NEAR(loadedTransform.Scale.y, 2.5f, 1e-5f);
    EXPECT_NEAR(loadedTransform.Scale.z, 3.5f, 1e-5f);
}

TEST_F(SerializationTest, SerializeLightComponent) {
    Entity entity = m_Scene->CreateEntity("LightTest");
    auto& light = entity.AddComponent<LightComponent>();
    
    light.Type = LightType::Point;
    light.Color = glm::vec3(1.0f, 0.5f, 0.25f);
    light.Intensity = 10.0f;
    light.Range = 50.0f;
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    Entity loadedEntity = loadedScene->GetEntityByName("LightTest");
    EXPECT_TRUE(loadedEntity.HasComponent<LightComponent>());
    
    auto& loadedLight = loadedEntity.GetComponent<LightComponent>();
    EXPECT_EQ(loadedLight.Type, LightType::Point);
    EXPECT_NEAR(loadedLight.Color.x, 1.0f, 1e-5f);
    EXPECT_NEAR(loadedLight.Color.y, 0.5f, 1e-5f);
    EXPECT_NEAR(loadedLight.Color.z, 0.25f, 1e-5f);
    EXPECT_FLOAT_EQ(loadedLight.Intensity, 10.0f);
    EXPECT_FLOAT_EQ(loadedLight.Range, 50.0f);
}

TEST_F(SerializationTest, SerializeCameraComponent) {
    Entity entity = m_Scene->CreateEntity("CameraTest");
    auto& camera = entity.AddComponent<CameraComponent>();
    
    camera.FOV = 60.0f;
    camera.NearClip = 0.01f;
    camera.FarClip = 2000.0f;
    camera.IsMainCamera = true;
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    Entity loadedEntity = loadedScene->GetEntityByName("CameraTest");
    EXPECT_TRUE(loadedEntity.HasComponent<CameraComponent>());
    
    auto& loadedCamera = loadedEntity.GetComponent<CameraComponent>();
    EXPECT_FLOAT_EQ(loadedCamera.FOV, 60.0f);
    EXPECT_FLOAT_EQ(loadedCamera.NearClip, 0.01f);
    EXPECT_FLOAT_EQ(loadedCamera.FarClip, 2000.0f);
    EXPECT_TRUE(loadedCamera.IsMainCamera);
}

// ==================== UUID Preservation ====================

TEST_F(SerializationTest, PreserveEntityUUID) {
    Entity entity = m_Scene->CreateEntity("UUIDTest");
    UUID originalUUID = entity.GetUUID();
    
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    loadSerializer.Deserialize(m_TestFilePath);
    
    Entity loadedEntity = loadedScene->GetEntityByName("UUIDTest");
    EXPECT_EQ(static_cast<uint64_t>(loadedEntity.GetUUID()), static_cast<uint64_t>(originalUUID));
}

// ==================== Error Handling ====================

TEST_F(SerializationTest, DeserializeNonexistentFile) {
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    
    EXPECT_FALSE(loadSerializer.Deserialize("nonexistent_file.json"));
}

TEST_F(SerializationTest, DeserializeInvalidJSON) {
    // Create an invalid JSON file
    std::ofstream file(m_TestFilePath);
    file << "{ invalid json content ";
    file.close();
    
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    
    EXPECT_FALSE(loadSerializer.Deserialize(m_TestFilePath));
}

// ==================== Complex Scene ====================

TEST_F(SerializationTest, SerializeComplexScene) {
    // Create a scene with multiple entities and various components
    Entity camera = m_Scene->CreateEntity("MainCamera");
    camera.AddComponent<CameraComponent>().IsMainCamera = true;
    camera.GetComponent<TransformComponent>().Position = glm::vec3(0, 5, 10);
    
    Entity light = m_Scene->CreateEntity("DirectionalLight");
    auto& lightComp = light.AddComponent<LightComponent>();
    lightComp.Type = LightType::Directional;
    lightComp.Intensity = 1.0f;
    
    Entity cube = m_Scene->CreateEntity("Cube");
    cube.GetComponent<TransformComponent>().Position = glm::vec3(0, 0, 0);
    cube.GetComponent<TransformComponent>().Scale = glm::vec3(2, 2, 2);
    
    Entity sphere = m_Scene->CreateEntity("Sphere");
    sphere.GetComponent<TransformComponent>().Position = glm::vec3(5, 0, 0);
    
    // Serialize
    SceneSerializer serializer(m_Scene);
    serializer.Serialize(m_TestFilePath);
    
    // Deserialize to new scene
    auto loadedScene = CreateRef<Scene>("LoadedScene");
    SceneSerializer loadSerializer(loadedScene);
    EXPECT_TRUE(loadSerializer.Deserialize(m_TestFilePath));
    
    // Verify all entities exist
    EXPECT_TRUE(loadedScene->GetEntityByName("MainCamera").IsValid());
    EXPECT_TRUE(loadedScene->GetEntityByName("DirectionalLight").IsValid());
    EXPECT_TRUE(loadedScene->GetEntityByName("Cube").IsValid());
    EXPECT_TRUE(loadedScene->GetEntityByName("Sphere").IsValid());
    
    // Verify components
    Entity loadedCamera = loadedScene->GetEntityByName("MainCamera");
    EXPECT_TRUE(loadedCamera.HasComponent<CameraComponent>());
    EXPECT_TRUE(loadedCamera.GetComponent<CameraComponent>().IsMainCamera);
    
    Entity loadedLight = loadedScene->GetEntityByName("DirectionalLight");
    EXPECT_TRUE(loadedLight.HasComponent<LightComponent>());
    EXPECT_EQ(loadedLight.GetComponent<LightComponent>().Type, LightType::Directional);
}
