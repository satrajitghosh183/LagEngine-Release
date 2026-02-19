#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include "../Core/Command.hpp"
#include <imgui.h>
#include <memory>
#include <string>

namespace GameEngine {

    // Forward declarations
    class EditorContext;
    
    // Base class for component windows
    class ComponentWindow {
    public:
        virtual ~ComponentWindow() = default;
        virtual void Update(float dt) {}
        virtual void Render(Entity entity) = 0;
        virtual const char* GetName() const = 0;
        /** @return true if this window handles removal for the given entity (and removed it) */
        virtual bool TryRemoveComponent(Entity entity) { return false; }
    };

    // Individual component windows
    class TransformWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Transform"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class MaterialWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Material"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class MeshWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Mesh"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class LightWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Light"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class CameraComponentWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Camera"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class RigidBodyWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Rigid Body"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class ColliderWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Collider"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class ScriptWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Script"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class GPUParticleWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "GPU Particles"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class RobotArmWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Robot Arm"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class SoftBodyWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Soft Body (XPBD)"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class SpriteWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Sprite"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    class SpriteAnimatorWindow : public ComponentWindow {
    public:
        void Render(Entity entity) override;
        const char* GetName() const override { return "Sprite Animator"; }
        bool TryRemoveComponent(Entity entity) override;
    };

    /**
     * @brief Components window - displays and edits components of selected entity
     */
    class ComponentsWindow {
    public:
        ComponentsWindow();
        ~ComponentsWindow() = default;

        void SetContext(EditorContext* context) { m_EditorContext = context; }
        void SetCommandHistory(CommandHistory* history) { m_CommandHistory = history; }
        void SetSelectedEntity(Entity entity) { m_SelectedEntity = entity; }

        void OnImGuiRender();

    private:
        void RenderComponentWindow(ComponentWindow* window, Entity entity);
        void RenderAddComponentMenu(Entity entity);

    private:
        EditorContext* m_EditorContext = nullptr;
        CommandHistory* m_CommandHistory = nullptr;
        Entity m_SelectedEntity;

        // Make CommandHistory accessible to component windows
        friend class ComponentWindow;

        // Component windows
        TransformWindow m_TransformWindow;
        MaterialWindow m_MaterialWindow;
        MeshWindow m_MeshWindow;
        LightWindow m_LightWindow;
        CameraComponentWindow m_CameraWindow;
        RigidBodyWindow m_RigidBodyWindow;
        ColliderWindow m_ColliderWindow;
        ScriptWindow m_ScriptWindow;
        GPUParticleWindow m_GPUParticleWindow;
        RobotArmWindow m_RobotArmWindow;
        SoftBodyWindow m_SoftBodyWindow;
        SpriteWindow m_SpriteWindow;
        SpriteAnimatorWindow m_SpriteAnimatorWindow;
    };

}
