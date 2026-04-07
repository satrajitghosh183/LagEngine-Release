#pragma once

#include "../Core/Base.hpp"
#include "RenderGraph.hpp"
#include <string>
#include <glm/glm.hpp>

namespace GameEngine {

    // Forward declaration
    class Scene;

    /**
     * @brief Base render path class
     * 
     * Render paths define how a scene is rendered (deferred, forward, 2D overlay, etc.)
     * Similar to Wicked Engine's RenderPath system
     */
    class RenderPath {
    public:
        RenderPath(const std::string& name);
        virtual ~RenderPath() = default;

        /**
         * @brief Lifecycle methods
         */
        virtual void Start() {}
        virtual void Update(float deltaTime) {}
        virtual void Render() {}
        virtual void Compose() {}

        /**
         * @brief Get render graph
         */
        Ref<RenderGraph> GetRenderGraph() const { return m_RenderGraph; }

        /**
         * @brief Check if path is active
         */
        bool IsActive() const { return m_IsActive; }
        void SetActive(bool active) { m_IsActive = active; }

        /**
         * @brief Get name
         */
        const std::string& GetName() const { return m_Name; }

    protected:
        std::string m_Name;
        Ref<RenderGraph> m_RenderGraph;
        bool m_IsActive = false;
    };

    /**
     * @brief 3D render path - renders 3D scenes
     */
    class RenderPath3D : public RenderPath {
    public:
        RenderPath3D();
        ~RenderPath3D() override = default;

        void Start() override;
        void Update(float deltaTime) override;
        void Render() override;
        void Compose() override;

        /**
         * @brief Post-process settings
         */
        void SetFXAAEnabled(bool enabled) { m_FXAAEnabled = enabled; }
        bool IsFXAAEnabled() const { return m_FXAAEnabled; }

        void SetBloomEnabled(bool enabled) { m_BloomEnabled = enabled; }
        bool IsBloomEnabled() const { return m_BloomEnabled; }

        void SetSSAOEnabled(bool enabled) { m_SSAOEnabled = enabled; }
        bool IsSSAOEnabled() const { return m_SSAOEnabled; }

        /**
         * @brief Set scene to render
         */
        void SetScene(const Ref<Scene>& scene) { m_Scene = scene; }
        Ref<Scene> GetScene() const { return m_Scene; }

    private:
        Ref<Scene> m_Scene;
        bool m_FXAAEnabled = false;
        bool m_BloomEnabled = false;
        bool m_SSAOEnabled = false;
    };

    /**
     * @brief 2D render path - renders 2D overlays (UI, sprites, fonts)
     */
    class RenderPath2D : public RenderPath {
    public:
        RenderPath2D();
        ~RenderPath2D() override = default;

        void Start() override;
        void Update(float deltaTime) override;
        void Render() override;
        void Compose() override;

        /**
         * @brief Add sprite to render
         */
        void AddSprite(/* Sprite* sprite */); // TODO: Implement Sprite class

        /**
         * @brief Add text to render
         */
        void AddText(const std::string& text, const glm::vec2& position, float size = 16.0f);

    private:
        // Sprite and text rendering data
        // TODO: Implement sprite and font rendering
    };

}
