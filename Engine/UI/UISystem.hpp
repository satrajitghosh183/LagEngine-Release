#pragma once

#include "UICanvas.hpp"
#include "UITheme.hpp"
#include "UIDrawList.hpp"
#include "UIWidgetsExt.hpp"
#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>

namespace GameEngine {

    class BatchRenderer2D;

    namespace UI {

        /**
         * UISystem — top-level UI manager.
         *
         * Owns the active canvas + theme, consumes the widget tree each frame,
         * emits a UIDrawList, and submits it through the engine's
         * BatchRenderer2D within an existing Vulkan command buffer.
         *
         * Usage:
         *   UISystem::Init();
         *   UISystem::SetCanvas(canvas);
         *   // per frame:
         *   UISystem::Update(dt, screenSize);
         *   UISystem::Render(cmd);
         *   UISystem::Shutdown();
         */
        class UISystem {
        public:
            static void Init();
            static void Shutdown();

            static void SetCanvas(Ref<UICanvas> canvas);
            static Ref<UICanvas> GetCanvas() { return s_Canvas; }

            static void SetTheme(Ref<UITheme> theme);
            static Ref<UITheme> GetTheme() { return s_Theme; }

            // Per-frame
            static void Update(float deltaTime, const glm::vec2& screenSize);
            static void Render(VkCommandBuffer cmd);

            // Input — forward from the Application's input callbacks
            static void OnMouseMove(float x, float y);
            static void OnMouseButton(int button, bool pressed);
            static void OnKey(int key, bool pressed);
            static void OnCharTyped(uint32_t codepoint);
            static void OnScroll(float dx, float dy);

        private:
            static void BuildDrawList();
            static void RecordWidget(UIWidget* w);

            static Ref<UICanvas> s_Canvas;
            static Ref<UITheme> s_Theme;
            static UIDrawList s_DrawList;
            static UIWidget* s_FocusedWidget;
            static UIWidget* s_HoveredWidget;
            static glm::vec2 s_MousePos;
        };

    }
}
