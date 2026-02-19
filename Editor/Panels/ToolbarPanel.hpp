#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../Core/EditorContext.hpp"
#include <imgui.h>

namespace GameEngine {

    /**
     * @brief Toolbar panel - Play/Pause/Stop controls
     * 
     * Features:
     * - Play button to enter play mode
     * - Pause button to pause simulation
     * - Stop button to exit play mode and restore scene
     * - Visual indication of current state
     */
    class ToolbarPanel {
    public:
        ToolbarPanel() = default;
        ~ToolbarPanel() = default;
        
        void SetContext(EditorContext* context) { m_Context = context; }
        
        void OnImGuiRender();
        
    private:
        EditorContext* m_Context = nullptr;
    };

}
