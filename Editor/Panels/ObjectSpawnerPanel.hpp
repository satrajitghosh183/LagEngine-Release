#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include <string>
#include <vector>
#include <functional>

namespace GameEngine {

    enum class SpawnCategory {
        Primitives,
        Lights,
        Physics,
        Effects,
        Audio,
        UI,
        Custom
    };

    struct SpawnableObject {
        std::string Name;
        std::string Description;
        std::string Icon;
        SpawnCategory Category;
        std::function<Entity(Scene*)> SpawnFunc;
    };

    class ObjectSpawnerPanel {
    public:
        ObjectSpawnerPanel();
        ~ObjectSpawnerPanel() = default;

        void OnImGuiRender();

        // Visibility control
        void Toggle() { m_IsOpen = !m_IsOpen; }
        void SetOpen(bool open) { m_IsOpen = open; }
        bool IsOpen() const { return m_IsOpen; }

        void SetScene(Scene* scene) { m_Scene = scene; }
        void SetOnEntitySpawnedCallback(std::function<void(Entity)> callback) {
            m_OnEntitySpawnedCallback = callback;
        }

        // Register custom spawnable objects
        void RegisterSpawnable(const SpawnableObject& spawnable);

    private:
        void RenderCategoryTab(SpawnCategory category);
        void RenderSpawnableItem(const SpawnableObject& item);
        void SpawnObject(const SpawnableObject& item);

        void InitializeSpawnables();
        std::string GetCategoryName(SpawnCategory category);
        std::vector<SpawnableObject*> GetObjectsInCategory(SpawnCategory category);

        // Primitive creation helpers
        Entity CreateCube(Scene* scene);
        Entity CreateSphere(Scene* scene);
        Entity CreatePlane(Scene* scene);
        Entity CreateCylinder(Scene* scene);
        Entity CreateCapsule(Scene* scene);
        Entity CreateCone(Scene* scene);
        Entity CreateTorus(Scene* scene);
        Entity CreateQuad(Scene* scene);

        // Light creation helpers
        Entity CreateDirectionalLight(Scene* scene);
        Entity CreatePointLight(Scene* scene);
        Entity CreateSpotLight(Scene* scene);

        // Physics creation helpers
        Entity CreateRigidBody(Scene* scene);
        Entity CreateStaticCollider(Scene* scene);
        Entity CreateTrigger(Scene* scene);

        // Effect creation helpers
        Entity CreateParticleSystem(Scene* scene);
        Entity CreateTrailRenderer(Scene* scene);

        // Audio creation helpers
        Entity CreateAudioSource(Scene* scene);
        Entity CreateAudioListener(Scene* scene);

    private:
        Scene* m_Scene = nullptr;
        std::vector<SpawnableObject> m_Spawnables;
        
        // UI state
        SpawnCategory m_SelectedCategory = SpawnCategory::Primitives;
        char m_SearchBuffer[128] = "";
        bool m_ShowGrid = true;
        int m_IconSize = 64;

        // Callbacks
        std::function<void(Entity)> m_OnEntitySpawnedCallback;

        // Visibility
        bool m_IsOpen = true;
    };

}
