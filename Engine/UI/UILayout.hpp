#pragma once

#include "UICanvas.hpp"

namespace GameEngine {
namespace UI {

    // Base class for layout containers. Arranges children in a specific pattern.
    class UILayoutContainer : public UIWidget {
    public:
        float Spacing = 4.0f;
        float PaddingLeft = 4.0f, PaddingRight = 4.0f;
        float PaddingTop  = 4.0f, PaddingBottom = 4.0f;

        // Called by the canvas after parent layout changes
        virtual void PerformLayout() = 0;

        void render() override {} // Containers are usually invisible
    };

    // Horizontal box: lays out children left-to-right
    class UIHBox : public UILayoutContainer {
    public:
        void PerformLayout() override;
        void render() override;
    };

    // Vertical box: lays out children top-to-bottom
    class UIVBox : public UILayoutContainer {
    public:
        void PerformLayout() override;
        void render() override;
    };

    // Grid: N columns, auto-rows
    class UIGridContainer : public UILayoutContainer {
    public:
        int Columns = 2;
        void PerformLayout() override;
        void render() override;
    };

    // Scrollable container: clips children and offsets by scroll position
    class UIScrollContainer : public UIWidget {
    public:
        float ScrollX = 0.0f;
        float ScrollY = 0.0f;
        glm::vec2 ContentSize{0.0f};

        void update(float dt) override;
        void render() override;
    };

}}
