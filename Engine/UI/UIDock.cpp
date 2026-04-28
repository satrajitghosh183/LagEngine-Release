#include "UIDock.hpp"
#include <algorithm>

namespace GameEngine {
namespace UI {

    UIDockPanel::UIDockPanel(const std::string& title) : Title(title) {
        m_Size = { 320.0f, 240.0f };
    }

    UIDockRoot::UIDockRoot() {
        m_Slots = {
            { Region::Left,   200.0f, {}, 0 },
            { Region::Right,  280.0f, {}, 0 },
            { Region::Top,     40.0f, {}, 0 },
            { Region::Bottom, 180.0f, {}, 0 },
            { Region::Center,   0.0f, {}, 0 },
        };
    }

    UIDockRoot::Slot& UIDockRoot::GetSlot(Region r) {
        for (auto& s : m_Slots) if (s.R == r) return s;
        return m_Slots.back();
    }

    void UIDockRoot::SetRegionSize(Region r, float size) { GetSlot(r).Size = size; }
    float UIDockRoot::GetRegionSize(Region r) const {
        for (const auto& s : m_Slots) if (s.R == r) return s.Size;
        return 0.0f;
    }

    void UIDockRoot::DockPanel(Ref<UIDockPanel> panel, Region region) {
        if (!panel) return;
        // Remove from any existing slot first
        for (auto& s : m_Slots) {
            auto it = std::find_if(s.Panels.begin(), s.Panels.end(),
                                   [&](const Ref<UIDockPanel>& p) { return p.get() == panel.get(); });
            if (it != s.Panels.end()) s.Panels.erase(it);
        }
        GetSlot(region).Panels.push_back(panel);
    }

    void UIDockRoot::UndockPanel(UIDockPanel* panel) {
        for (auto& s : m_Slots) {
            auto it = std::find_if(s.Panels.begin(), s.Panels.end(),
                                   [&](const Ref<UIDockPanel>& p) { return p.get() == panel; });
            if (it != s.Panels.end()) { s.Panels.erase(it); return; }
        }
    }

    void UIDockRoot::Layout(const glm::vec2& screenSize) {
        float left = GetRegionSize(Region::Left);
        float right = GetRegionSize(Region::Right);
        float top = GetRegionSize(Region::Top);
        float bottom = GetRegionSize(Region::Bottom);

        float centerW = std::max(0.0f, screenSize.x - left - right);
        float centerH = std::max(0.0f, screenSize.y - top - bottom);

        // Layout each slot's panels (first tab visible, others hidden)
        for (auto& s : m_Slots) {
            glm::vec2 pos, size;
            switch (s.R) {
                case Region::Left:   pos = { 0.0f, top }; size = { s.Size, centerH }; break;
                case Region::Right:  pos = { screenSize.x - s.Size, top }; size = { s.Size, centerH }; break;
                case Region::Top:    pos = { 0.0f, 0.0f }; size = { screenSize.x, s.Size }; break;
                case Region::Bottom: pos = { 0.0f, screenSize.y - s.Size }; size = { screenSize.x, s.Size }; break;
                case Region::Center: pos = { left, top }; size = { centerW, centerH }; break;
            }
            for (size_t i = 0; i < s.Panels.size(); i++) {
                auto& p = s.Panels[i];
                p->setPosition(pos);
                p->setSize(size);
                p->setVisible(static_cast<int>(i) == s.ActiveTab);
            }
        }
    }

    void UIDockRoot::render() {
        // Actual visual rendering happens through UISystem which walks children.
    }

}}
