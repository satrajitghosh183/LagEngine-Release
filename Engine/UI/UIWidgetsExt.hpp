#pragma once

#include "UICanvas.hpp"
#include "UISignal.hpp"
#include "UIDrawList.hpp"
#include "UITheme.hpp"
#include <string>
#include <vector>

namespace GameEngine {
namespace UI {

    // CheckBox — on/off toggle with a label.
    class UICheckBox : public UIWidget {
    public:
        explicit UICheckBox(const std::string& label = "CheckBox", bool checked = false);

        void render() override;
        bool hitTest(const glm::vec2& point) const override;

        bool IsChecked = false;
        std::string Label;
        Ref<UITheme> Theme;

        UISignal<bool> OnToggled;

        void HandleClick();
    };

    // TextEdit — single-line editable text field.
    class UITextEdit : public UIWidget {
    public:
        UITextEdit();

        void render() override;
        bool hitTest(const glm::vec2& point) const override;
        void update(float dt) override;

        std::string Text;
        std::string Placeholder = "Enter text...";
        int MaxLength = 256;
        bool HasFocus = false;
        Ref<UITheme> Theme;

        UISignal<const std::string&> OnTextChanged;
        UISignal<const std::string&> OnSubmit; // Enter pressed

        // Input forwarding — call from input system
        void OnCharTyped(uint32_t codepoint);
        void OnKey(int keycode, bool pressed);

    private:
        int m_Cursor = 0;
        float m_CursorBlink = 0.0f;
    };

    // Dropdown / ComboBox — select one from a list.
    class UIDropdown : public UIWidget {
    public:
        UIDropdown() = default;

        void render() override;
        bool hitTest(const glm::vec2& point) const override;

        std::vector<std::string> Options;
        int SelectedIndex = -1;
        bool IsOpen = false;
        Ref<UITheme> Theme;

        UISignal<int, const std::string&> OnSelectionChanged;

        void HandleClick();
        void SelectOption(int index);
    };

    // TabContainer — tabs at top, one child shown per tab.
    class UITabContainer : public UIWidget {
    public:
        UITabContainer() = default;

        void render() override;
        bool hitTest(const glm::vec2& point) const override;

        std::vector<std::string> TabNames;
        int CurrentTab = 0;
        Ref<UITheme> Theme;

        UISignal<int> OnTabChanged;

        void AddTab(const std::string& name, Ref<UIWidget> content);
    };

    // TreeView — hierarchical collapsible list.
    class UITreeNode {
    public:
        std::string Label;
        bool Expanded = true;
        bool Selected = false;
        std::vector<Ref<UITreeNode>> Children;
        void* UserData = nullptr;
    };

    class UITreeView : public UIWidget {
    public:
        UITreeView() = default;

        void render() override;
        bool hitTest(const glm::vec2& point) const override;

        Ref<UITreeNode> Root;
        Ref<UITheme> Theme;

        UISignal<UITreeNode*> OnNodeSelected;
        UISignal<UITreeNode*> OnNodeExpanded;
    };

}}
