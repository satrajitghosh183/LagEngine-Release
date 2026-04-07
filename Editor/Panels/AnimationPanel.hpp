#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include <imgui.h>
#include <string>

namespace GameEngine {

    /**
     * @brief Animation panel
     *
     * Shows the animation state of the selected entity and allows
     * playback control + visual timeline scrubbing.
     *
     * Currently supports:
     *   - SpriteAnimatorComponent  (2D sprite-sheet animation)
     *
     * Transform keyframe animation is prepared for a future implementation.
     */
    class AnimationPanel {
    public:
        void SetContext(const Ref<Scene>& scene) { m_Context = scene; }
        void SetSelectedEntity(Entity entity)    { m_SelectedEntity = entity; }

        void OnImGuiRender();

    private:
        void RenderSpriteAnimator();
        void RenderNoAnimationMsg();
        void DrawTimeline(int currentFrame, int totalFrames, float& scrubValue);

    private:
        Ref<Scene> m_Context;
        Entity     m_SelectedEntity;
    };

}
