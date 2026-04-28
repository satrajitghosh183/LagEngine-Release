#include "UILayout.hpp"

namespace GameEngine {
namespace UI {

    void UIHBox::PerformLayout() {
        float x = PaddingLeft;
        float maxH = 0.0f;
        for (auto& child : m_Children) {
            if (!child->isVisible()) continue;
            glm::vec2 sz = child->getSize();
            child->setPosition({ x, PaddingTop });
            x += sz.x + Spacing;
            maxH = std::max(maxH, sz.y);
        }
        m_Size.x = x - Spacing + PaddingRight;
        m_Size.y = maxH + PaddingTop + PaddingBottom;

        for (auto& child : m_Children) {
            if (auto lc = dynamic_cast<UILayoutContainer*>(child.get())) lc->PerformLayout();
        }
    }

    void UIHBox::render() {
        for (auto& child : m_Children) {
            if (child->isVisible()) child->render();
        }
    }

    void UIVBox::PerformLayout() {
        float y = PaddingTop;
        float maxW = 0.0f;
        for (auto& child : m_Children) {
            if (!child->isVisible()) continue;
            glm::vec2 sz = child->getSize();
            child->setPosition({ PaddingLeft, y });
            y += sz.y + Spacing;
            maxW = std::max(maxW, sz.x);
        }
        m_Size.y = y - Spacing + PaddingBottom;
        m_Size.x = maxW + PaddingLeft + PaddingRight;

        for (auto& child : m_Children) {
            if (auto lc = dynamic_cast<UILayoutContainer*>(child.get())) lc->PerformLayout();
        }
    }

    void UIVBox::render() {
        for (auto& child : m_Children) {
            if (child->isVisible()) child->render();
        }
    }

    void UIGridContainer::PerformLayout() {
        if (Columns < 1) Columns = 1;
        float maxW = 0.0f, maxH = 0.0f;
        for (auto& child : m_Children) {
            if (!child->isVisible()) continue;
            maxW = std::max(maxW, child->getSize().x);
            maxH = std::max(maxH, child->getSize().y);
        }

        int col = 0, row = 0;
        for (auto& child : m_Children) {
            if (!child->isVisible()) continue;
            float x = PaddingLeft + col * (maxW + Spacing);
            float y = PaddingTop  + row * (maxH + Spacing);
            child->setPosition({ x, y });
            col++;
            if (col >= Columns) { col = 0; row++; }
        }

        int totalChildren = 0;
        for (auto& child : m_Children) if (child->isVisible()) totalChildren++;
        int rows = (totalChildren + Columns - 1) / Columns;
        m_Size.x = PaddingLeft + Columns * maxW + (Columns - 1) * Spacing + PaddingRight;
        m_Size.y = PaddingTop  + rows * maxH + (rows > 0 ? (rows - 1) : 0) * Spacing + PaddingBottom;

        for (auto& child : m_Children) {
            if (auto lc = dynamic_cast<UILayoutContainer*>(child.get())) lc->PerformLayout();
        }
    }

    void UIGridContainer::render() {
        for (auto& child : m_Children) {
            if (child->isVisible()) child->render();
        }
    }

    void UIScrollContainer::update(float dt) {
        UIWidget::update(dt);
        // Clamp scroll
        float maxX = std::max(0.0f, ContentSize.x - m_Size.x);
        float maxY = std::max(0.0f, ContentSize.y - m_Size.y);
        ScrollX = std::max(0.0f, std::min(ScrollX, maxX));
        ScrollY = std::max(0.0f, std::min(ScrollY, maxY));
    }

    void UIScrollContainer::render() {
        // Children are offset by scroll; actual clipping happens in UISystem via PushScissor
        for (auto& child : m_Children) {
            if (child->isVisible()) child->render();
        }
    }

}}
