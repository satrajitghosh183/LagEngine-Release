#include "WickedStyleTemplateApp.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Scene/SceneHelpers.hpp"
#include "../../Engine/Graphics/RenderPath.hpp"

namespace GameEngine {

    WickedStyleTemplateApp::WickedStyleTemplateApp()
        : Application(RuntimeConfig(1280, 720, "Wicked Style Template")) {
    }

    void WickedStyleTemplateApp::OnInit() {
        GE_CORE_INFO("WickedStyleTemplateApp initialized");
        
        // Create render paths
        m_RenderPath3D = CreateRef<RenderPath3D>();
        m_RenderPath2D = CreateRef<RenderPath2D>();
        
        // Configure RenderPath3D post-process
        m_RenderPath3D->SetFXAAEnabled(true);
        m_RenderPath3D->SetBloomEnabled(false);
        m_RenderPath3D->SetSSAOEnabled(true);
        
        // Get global scene
        auto scene = wi::scene::GetScene();
        
        // Set scene on render path
        m_RenderPath3D->SetScene(scene);
        
        // Activate 3D render path
        ActivatePath(m_RenderPath3D);
        
        // Example: Load a model using Wicked-style API
        // Entity modelEntity = wi::scene::LoadModel("Assets/Models/cube.obj");
    }

    void WickedStyleTemplateApp::OnUpdate(float deltaTime) {
        // Example: Input handling using Wicked-style API
        if (wi::input::Press(wi::input::Keys::KEYBOARD_BUTTON_SPACE)) {
            GE_CORE_INFO("Space pressed!");
        }
        
        if (wi::input::Down(wi::input::Keys::KEYBOARD_BUTTON_TAB)) {
            // Switch render paths
            m_Use3D = !m_Use3D;
            if (m_Use3D) {
                ActivatePath(m_RenderPath3D);
            } else {
                ActivatePath(m_RenderPath2D);
            }
        }
    }

    void WickedStyleTemplateApp::OnRender() {
        // Rendering handled by active render path
    }

}
