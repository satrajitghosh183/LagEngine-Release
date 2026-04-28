#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace GameEngine {
namespace UI {

    // Per-widget-type style block.
    struct UIStyle {
        glm::vec4 BackgroundColor = glm::vec4(0.18f, 0.18f, 0.20f, 1.0f);
        glm::vec4 BorderColor     = glm::vec4(0.35f, 0.35f, 0.40f, 1.0f);
        glm::vec4 TextColor       = glm::vec4(0.95f, 0.95f, 0.95f, 1.0f);
        glm::vec4 HoverColor      = glm::vec4(0.25f, 0.30f, 0.50f, 1.0f);
        glm::vec4 PressedColor    = glm::vec4(0.15f, 0.20f, 0.40f, 1.0f);
        glm::vec4 DisabledColor   = glm::vec4(0.30f, 0.30f, 0.30f, 1.0f);
        glm::vec4 AccentColor     = glm::vec4(0.30f, 0.60f, 1.00f, 1.0f);
        float CornerRadius = 4.0f;
        float BorderWidth = 1.0f;
        float PaddingX = 8.0f;
        float PaddingY = 4.0f;
        float FontSize = 14.0f;
        std::string FontName = "default";
    };

    // A theme is a collection of named styles keyed by widget type ("Button",
    // "Label", "Panel", "CheckBox", "Slider", ...). Widgets query the theme
    // for their type, falling back to a default style.
    class UITheme {
    public:
        static Ref<UITheme> CreateDefault();

        // Style lookup; returns default style if not set.
        const UIStyle& GetStyle(const std::string& widgetType) const;
        void SetStyle(const std::string& widgetType, const UIStyle& style);

        // Load a theme from a JSON file. Returns null on failure.
        static Ref<UITheme> LoadFromJSON(const std::string& path);
        bool SaveToJSON(const std::string& path) const;

    private:
        UIStyle m_Default;
        std::unordered_map<std::string, UIStyle> m_Styles;
    };

}}
