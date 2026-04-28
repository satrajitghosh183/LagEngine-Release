#include "UIWidgetsExt.hpp"

namespace GameEngine {
namespace UI {

    // =====================================================================
    // CheckBox
    // =====================================================================

    UICheckBox::UICheckBox(const std::string& label, bool checked)
        : IsChecked(checked), Label(label) {
        m_Size = { 160.0f, 22.0f };
    }

    bool UICheckBox::hitTest(const glm::vec2& point) const {
        glm::vec4 r = getWorldRect();
        return point.x >= r.x && point.x <= r.x + r.z &&
               point.y >= r.y && point.y <= r.y + r.w;
    }

    void UICheckBox::render() {
        // Rendering is deferred through the canvas → BatchRenderer2D.
        // Widgets don't directly draw — UISystem consumes the widget tree
        // and emits draw commands via UIDrawList.
    }

    void UICheckBox::HandleClick() {
        IsChecked = !IsChecked;
        OnToggled.Emit(IsChecked);
    }

    // =====================================================================
    // TextEdit
    // =====================================================================

    UITextEdit::UITextEdit() {
        m_Size = { 180.0f, 24.0f };
    }

    bool UITextEdit::hitTest(const glm::vec2& point) const {
        glm::vec4 r = getWorldRect();
        return point.x >= r.x && point.x <= r.x + r.z &&
               point.y >= r.y && point.y <= r.y + r.w;
    }

    void UITextEdit::update(float dt) {
        UIWidget::update(dt);
        m_CursorBlink += dt;
        if (m_CursorBlink > 1.0f) m_CursorBlink -= 1.0f;
    }

    void UITextEdit::render() {
        // See CheckBox::render — deferred to UISystem.
    }

    void UITextEdit::OnCharTyped(uint32_t codepoint) {
        if (!HasFocus) return;
        if (static_cast<int>(Text.size()) >= MaxLength) return;
        if (codepoint < 32 || codepoint == 127) return; // skip control chars
        Text.insert(m_Cursor, 1, static_cast<char>(codepoint));
        m_Cursor++;
        OnTextChanged.Emit(Text);
    }

    void UITextEdit::OnKey(int keycode, bool pressed) {
        if (!pressed || !HasFocus) return;
        // GLFW key constants: 259 = BACKSPACE, 257 = ENTER, 262 = RIGHT, 263 = LEFT, 261 = DELETE
        if (keycode == 259) { // Backspace
            if (m_Cursor > 0) {
                Text.erase(m_Cursor - 1, 1);
                m_Cursor--;
                OnTextChanged.Emit(Text);
            }
        } else if (keycode == 261) { // Delete
            if (m_Cursor < static_cast<int>(Text.size())) {
                Text.erase(m_Cursor, 1);
                OnTextChanged.Emit(Text);
            }
        } else if (keycode == 263) { // Left
            m_Cursor = std::max(0, m_Cursor - 1);
        } else if (keycode == 262) { // Right
            m_Cursor = std::min(static_cast<int>(Text.size()), m_Cursor + 1);
        } else if (keycode == 257 || keycode == 335) { // Enter, KP_Enter
            OnSubmit.Emit(Text);
        }
    }

    // =====================================================================
    // Dropdown
    // =====================================================================

    bool UIDropdown::hitTest(const glm::vec2& point) const {
        glm::vec4 r = getWorldRect();
        float h = r.w;
        if (IsOpen) {
            h += static_cast<float>(Options.size()) * m_Size.y;
        }
        return point.x >= r.x && point.x <= r.x + r.z &&
               point.y >= r.y && point.y <= r.y + h;
    }

    void UIDropdown::render() {}

    void UIDropdown::HandleClick() {
        IsOpen = !IsOpen;
    }

    void UIDropdown::SelectOption(int index) {
        if (index < 0 || index >= static_cast<int>(Options.size())) return;
        SelectedIndex = index;
        IsOpen = false;
        OnSelectionChanged.Emit(index, Options[index]);
    }

    // =====================================================================
    // TabContainer
    // =====================================================================

    bool UITabContainer::hitTest(const glm::vec2& point) const {
        glm::vec4 r = getWorldRect();
        return point.x >= r.x && point.x <= r.x + r.z &&
               point.y >= r.y && point.y <= r.y + r.w;
    }

    void UITabContainer::render() {}

    void UITabContainer::AddTab(const std::string& name, Ref<UIWidget> content) {
        TabNames.push_back(name);
        addChild(content);
        // Only the current tab's content is visible
        for (size_t i = 0; i < m_Children.size(); i++) {
            m_Children[i]->setVisible(static_cast<int>(i) == CurrentTab);
        }
    }

    // =====================================================================
    // TreeView
    // =====================================================================

    bool UITreeView::hitTest(const glm::vec2& point) const {
        glm::vec4 r = getWorldRect();
        return point.x >= r.x && point.x <= r.x + r.z &&
               point.y >= r.y && point.y <= r.y + r.w;
    }

    void UITreeView::render() {}

}}
