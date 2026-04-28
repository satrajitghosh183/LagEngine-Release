#pragma once

#include "UICanvas.hpp"
#include "UISignal.hpp"
#include "UITheme.hpp"
#include <string>
#include <vector>

namespace GameEngine {
namespace UI {

    /**
     * UIDockPanel — a movable/resizable panel that can be docked in a UIDockRoot.
     *
     * Panels have a title bar and a content area. They can be dragged to other
     * dock regions, resized by their borders, and tab-stacked with other panels
     * occupying the same dock slot.
     */
    class UIDockPanel : public UIWidget {
    public:
        explicit UIDockPanel(const std::string& title = "Panel");

        std::string Title;
        bool Closable = true;
        bool Movable = true;
        bool Resizable = true;
        Ref<UITheme> Theme;

        UISignal<> OnClose;
        UISignal<> OnFocus;

        void render() override {}
    };

    /**
     * UIDockRoot — the container managing the whole editor layout.
     *
     * Splits the viewport into named dock regions (Left, Right, Top, Bottom, Center).
     * Panels are placed into regions by name; regions can be resized by dragging
     * the split handles.
     */
    class UIDockRoot : public UIWidget {
    public:
        enum class Region { Center, Left, Right, Top, Bottom };

        UIDockRoot();

        void SetRegionSize(Region r, float size); // pixels for L/R/T/B; Center takes remainder
        float GetRegionSize(Region r) const;

        void DockPanel(Ref<UIDockPanel> panel, Region region);
        void UndockPanel(UIDockPanel* panel);

        void render() override;
        void Layout(const glm::vec2& screenSize);

    private:
        struct Slot {
            Region R;
            float Size;
            std::vector<Ref<UIDockPanel>> Panels;
            int ActiveTab = 0;
        };
        std::vector<Slot> m_Slots;

        Slot& GetSlot(Region r);
    };

}}
