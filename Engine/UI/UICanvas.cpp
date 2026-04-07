#include "UICanvas.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {
namespace UI {

    // =====================================================================
    // UIWidget
    // =====================================================================

    void UIWidget::update(float dt) {
        // Update tweens
        for (auto& tween : m_Tweens) {
            if (!tween.Active) continue;
            tween.Timer += dt;
            float t = glm::clamp(tween.Timer / tween.Duration, 0.0f, 1.0f);
            t = t * t * (3.0f - 2.0f * t); // smoothstep

            if (tween.Prop == Tween::Property::Position) {
                m_Position = glm::mix(tween.StartPos, tween.EndPos, t);
            } else if (tween.Prop == Tween::Property::Alpha) {
                m_Alpha = glm::mix(tween.StartAlpha, tween.EndAlpha, t);
            }

            if (tween.Timer >= tween.Duration) tween.Active = false;
        }

        // Remove finished tweens
        m_Tweens.erase(
            std::remove_if(m_Tweens.begin(), m_Tweens.end(),
                [](const Tween& t) { return !t.Active; }),
            m_Tweens.end());

        // Update children
        for (auto& child : m_Children) {
            child->update(dt);
        }
    }

    bool UIWidget::hitTest(const glm::vec2& point) const {
        glm::vec4 rect = getWorldRect();
        return point.x >= rect.x && point.x <= rect.x + rect.z &&
               point.y >= rect.y && point.y <= rect.y + rect.w;
    }

    void UIWidget::addChild(Ref<UIWidget> child) {
        child->m_Parent = this;
        m_Children.push_back(std::move(child));
    }

    void UIWidget::removeChild(UIWidget* child) {
        m_Children.erase(
            std::remove_if(m_Children.begin(), m_Children.end(),
                [child](const Ref<UIWidget>& w) { return w.get() == child; }),
            m_Children.end());
    }

    glm::vec2 UIWidget::getWorldPosition() const {
        glm::vec2 pos = m_Position;
        if (m_Parent) {
            pos += m_Parent->getWorldPosition();
        }
        return pos;
    }

    glm::vec4 UIWidget::getWorldRect() const {
        glm::vec2 pos = getWorldPosition();
        return glm::vec4(pos.x, pos.y, m_Size.x, m_Size.y);
    }

    void UIWidget::tweenPosition(const glm::vec2& target, float duration) {
        Tween tween;
        tween.Prop = Tween::Property::Position;
        tween.StartPos = m_Position;
        tween.EndPos = target;
        tween.Duration = duration;
        tween.Active = true;
        m_Tweens.push_back(tween);
    }

    void UIWidget::tweenAlpha(float target, float duration) {
        Tween tween;
        tween.Prop = Tween::Property::Alpha;
        tween.StartAlpha = m_Alpha;
        tween.EndAlpha = target;
        tween.Duration = duration;
        tween.Active = true;
        m_Tweens.push_back(tween);
    }

    // =====================================================================
    // Concrete Widgets
    // =====================================================================

    UILabel::UILabel(const std::string& text) : Text(text) {
        m_Size = glm::vec2(200.0f, 30.0f);
        m_Interactable = false;
    }

    void UILabel::render() {
        if (!m_Visible) return;
        // In a full implementation, this would use the engine's text rendering system
        // For now, widgets are rendered via ImGui in the editor or a custom renderer
        for (auto& child : m_Children) child->render();
    }

    UIButton::UIButton(const std::string& label) : Label(label) {
        m_Size = glm::vec2(120.0f, 40.0f);
    }

    void UIButton::render() {
        if (!m_Visible) return;
        for (auto& child : m_Children) child->render();
    }

    UIImage::UIImage(uint32_t textureID) : TextureID(textureID) {
        m_Size = glm::vec2(64.0f, 64.0f);
    }

    void UIImage::render() {
        if (!m_Visible) return;
        for (auto& child : m_Children) child->render();
    }

    UISlider::UISlider(float minVal, float maxVal, float value)
        : MinValue(minVal), MaxValue(maxVal), Value(value) {
        m_Size = glm::vec2(200.0f, 20.0f);
    }

    void UISlider::render() {
        if (!m_Visible) return;
        for (auto& child : m_Children) child->render();
    }

    UIProgressBar::UIProgressBar(float value) : Value(value) {
        m_Size = glm::vec2(200.0f, 20.0f);
        m_Interactable = false;
    }

    void UIProgressBar::render() {
        if (!m_Visible) return;
        for (auto& child : m_Children) child->render();
    }

    UIPanel::UIPanel() {
        m_Size = glm::vec2(300.0f, 200.0f);
    }

    void UIPanel::render() {
        if (!m_Visible) return;
        for (auto& child : m_Children) child->render();
    }

    // =====================================================================
    // UICanvas
    // =====================================================================

    UICanvas::UICanvas(float referenceWidth, float referenceHeight)
        : m_RefWidth(referenceWidth), m_RefHeight(referenceHeight) {}

    void UICanvas::update(float dt, const glm::vec2& screenSize) {
        m_ScreenWidth = screenSize.x;
        m_ScreenHeight = screenSize.y;
        m_ScaleX = screenSize.x / m_RefWidth;
        m_ScaleY = screenSize.y / m_RefHeight;

        for (auto& widget : m_RootWidgets) {
            widget->update(dt);
        }
    }

    void UICanvas::render() {
        for (auto& widget : m_RootWidgets) {
            if (widget->isVisible()) {
                widget->render();
            }
        }
    }

    void UICanvas::onMouseMove(const glm::vec2& screenPos) {
        glm::vec2 canvasPos = screenToCanvas(screenPos);

        UIWidget* hovered = nullptr;
        for (auto it = m_RootWidgets.rbegin(); it != m_RootWidgets.rend(); ++it) {
            hovered = hitTestRecursive(it->get(), canvasPos);
            if (hovered) break;
        }

        if (hovered != m_HoveredWidget) {
            if (m_HoveredWidget) {
                // Could fire hover exit event
            }
            m_HoveredWidget = hovered;
            if (m_HoveredWidget) {
                if (auto* btn = dynamic_cast<UIButton*>(m_HoveredWidget)) {
                    btn->IsHovered = true;
                }
            }
        }

        // Handle slider drag
        if (m_PressedWidget) {
            if (auto* slider = dynamic_cast<UISlider*>(m_PressedWidget)) {
                glm::vec4 rect = slider->getWorldRect();
                float t = glm::clamp((canvasPos.x - rect.x) / rect.z, 0.0f, 1.0f);
                slider->Value = glm::mix(slider->MinValue, slider->MaxValue, t);
                if (slider->OnValueChanged) slider->OnValueChanged(slider->Value);
            }
        }
    }

    void UICanvas::onMouseClick(const glm::vec2& screenPos) {
        glm::vec2 canvasPos = screenToCanvas(screenPos);

        UIWidget* clicked = nullptr;
        for (auto it = m_RootWidgets.rbegin(); it != m_RootWidgets.rend(); ++it) {
            clicked = hitTestRecursive(it->get(), canvasPos);
            if (clicked) break;
        }

        m_PressedWidget = clicked;

        if (clicked) {
            if (auto* btn = dynamic_cast<UIButton*>(clicked)) {
                btn->IsPressed = true;
                if (btn->OnClick) btn->OnClick();
            }
        }
    }

    void UICanvas::onMouseRelease(const glm::vec2& /*screenPos*/) {
        if (m_PressedWidget) {
            if (auto* btn = dynamic_cast<UIButton*>(m_PressedWidget)) {
                btn->IsPressed = false;
            }
            m_PressedWidget = nullptr;
        }
    }

    void UICanvas::addWidget(Ref<UIWidget> widget) {
        m_RootWidgets.push_back(std::move(widget));
    }

    void UICanvas::removeWidget(UIWidget* widget) {
        m_RootWidgets.erase(
            std::remove_if(m_RootWidgets.begin(), m_RootWidgets.end(),
                [widget](const Ref<UIWidget>& w) { return w.get() == widget; }),
            m_RootWidgets.end());
    }

    UIWidget* UICanvas::findByName(const std::string& name) const {
        std::function<UIWidget*(UIWidget*)> search = [&](UIWidget* w) -> UIWidget* {
            if (w->Name == name) return w;
            for (auto& child : w->getChildren()) {
                UIWidget* found = search(child.get());
                if (found) return found;
            }
            return nullptr;
        };

        for (auto& widget : m_RootWidgets) {
            UIWidget* found = search(widget.get());
            if (found) return found;
        }
        return nullptr;
    }

    glm::vec2 UICanvas::screenToCanvas(const glm::vec2& screenPos) const {
        return glm::vec2(screenPos.x / m_ScaleX, screenPos.y / m_ScaleY);
    }

    UIWidget* UICanvas::hitTestRecursive(UIWidget* widget, const glm::vec2& point) {
        if (!widget->isVisible() || !widget->isInteractable()) return nullptr;

        // Check children first (front-to-back, reversed)
        auto& children = widget->getChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            UIWidget* hit = hitTestRecursive(it->get(), point);
            if (hit) return hit;
        }

        if (widget->hitTest(point)) return widget;
        return nullptr;
    }

}}
