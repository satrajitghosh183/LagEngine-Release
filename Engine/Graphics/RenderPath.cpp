#include "RenderPath.hpp"
#include "Renderer3D.hpp"
#include "../Scene/Scene.hpp"
#include "../Core/Logger.hpp"
#include "../Scene/Scene.hpp"

namespace GameEngine {

    RenderPath::RenderPath(const std::string& name)
        : m_Name(name)
        , m_RenderGraph(CreateRef<RenderGraph>()) {
    }

    RenderPath3D::RenderPath3D()
        : RenderPath("RenderPath3D") {
    }

    void RenderPath3D::Start() {
        m_IsActive = true;
        GE_CORE_INFO("RenderPath3D started");
    }

    void RenderPath3D::Update(float deltaTime) {
        if (!m_IsActive || !m_Scene) return;
        m_Scene->Update(deltaTime);
    }

    void RenderPath3D::Render() {
        if (!m_IsActive || !m_Scene) return;
        
        // Get main camera
        auto* camera = m_Scene->GetMainCamera();
        if (!camera) return;

        // Begin scene rendering
        Renderer3D::BeginScene(*camera);
        
        // Render scene
        m_Scene->Render();
        
        // End scene (applies post-processing if enabled)
        Renderer3D::EndScene();
    }

    void RenderPath3D::Compose() {
        if (!m_IsActive) return;
        
        // Execute render graph
        m_RenderGraph->Execute();
        
        // Apply post-processing effects
        // TODO: Implement FXAA, Bloom, SSAO
    }

    RenderPath2D::RenderPath2D()
        : RenderPath("RenderPath2D") {
    }

    void RenderPath2D::Start() {
        m_IsActive = true;
        GE_CORE_INFO("RenderPath2D started");
    }

    void RenderPath2D::Update(float deltaTime) {
        if (!m_IsActive) return;
        // Update 2D elements
    }

    void RenderPath2D::Render() {
        if (!m_IsActive) return;
        // Render sprites and text
        // TODO: Implement 2D rendering
    }

    void RenderPath2D::Compose() {
        if (!m_IsActive) return;
        // Compose 2D overlay
    }

    void RenderPath2D::AddSprite(/* Sprite* sprite */) {
        // TODO: Implement sprite rendering
    }

    void RenderPath2D::AddText(const std::string& text, const glm::vec2& position, float size) {
        // TODO: Implement text rendering
    }

}
