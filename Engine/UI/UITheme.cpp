#include "UITheme.hpp"
#include "../Core/Logger.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace GameEngine {
namespace UI {

    Ref<UITheme> UITheme::CreateDefault() {
        auto theme = CreateRef<UITheme>();

        // Button
        UIStyle btn;
        btn.BackgroundColor = glm::vec4(0.25f, 0.25f, 0.28f, 1.0f);
        btn.HoverColor      = glm::vec4(0.32f, 0.40f, 0.55f, 1.0f);
        btn.PressedColor    = glm::vec4(0.20f, 0.30f, 0.45f, 1.0f);
        btn.CornerRadius = 4.0f;
        btn.PaddingX = 12.0f;
        theme->SetStyle("Button", btn);

        // Panel
        UIStyle panel;
        panel.BackgroundColor = glm::vec4(0.12f, 0.12f, 0.14f, 0.95f);
        panel.BorderColor     = glm::vec4(0.30f, 0.30f, 0.35f, 1.0f);
        panel.CornerRadius = 6.0f;
        panel.BorderWidth = 1.0f;
        theme->SetStyle("Panel", panel);

        // Label — transparent background
        UIStyle lbl;
        lbl.BackgroundColor = glm::vec4(0.0f);
        lbl.BorderColor = glm::vec4(0.0f);
        theme->SetStyle("Label", lbl);

        // Slider, Checkbox, etc. use defaults with accent
        UIStyle slider;
        slider.AccentColor = glm::vec4(0.35f, 0.65f, 1.00f, 1.0f);
        slider.BackgroundColor = glm::vec4(0.10f, 0.10f, 0.12f, 1.0f);
        theme->SetStyle("Slider", slider);

        UIStyle check;
        check.AccentColor = glm::vec4(0.35f, 0.65f, 1.00f, 1.0f);
        check.BackgroundColor = glm::vec4(0.15f, 0.15f, 0.18f, 1.0f);
        theme->SetStyle("CheckBox", check);

        UIStyle tedit;
        tedit.BackgroundColor = glm::vec4(0.08f, 0.08f, 0.10f, 1.0f);
        tedit.BorderColor = glm::vec4(0.30f, 0.35f, 0.45f, 1.0f);
        tedit.BorderWidth = 1.0f;
        theme->SetStyle("TextEdit", tedit);

        return theme;
    }

    const UIStyle& UITheme::GetStyle(const std::string& widgetType) const {
        auto it = m_Styles.find(widgetType);
        return it == m_Styles.end() ? m_Default : it->second;
    }

    void UITheme::SetStyle(const std::string& widgetType, const UIStyle& style) {
        m_Styles[widgetType] = style;
    }

    static glm::vec4 ParseColor(const nlohmann::json& j, glm::vec4 fallback) {
        if (j.is_array() && j.size() >= 3) {
            return { j[0].get<float>(), j[1].get<float>(), j[2].get<float>(),
                     j.size() >= 4 ? j[3].get<float>() : 1.0f };
        }
        return fallback;
    }

    static void StyleFromJSON(UIStyle& s, const nlohmann::json& j) {
        if (j.contains("background")) s.BackgroundColor = ParseColor(j["background"], s.BackgroundColor);
        if (j.contains("border"))     s.BorderColor     = ParseColor(j["border"], s.BorderColor);
        if (j.contains("text"))       s.TextColor       = ParseColor(j["text"], s.TextColor);
        if (j.contains("hover"))      s.HoverColor      = ParseColor(j["hover"], s.HoverColor);
        if (j.contains("pressed"))    s.PressedColor    = ParseColor(j["pressed"], s.PressedColor);
        if (j.contains("accent"))     s.AccentColor     = ParseColor(j["accent"], s.AccentColor);
        if (j.contains("cornerRadius")) s.CornerRadius = j["cornerRadius"].get<float>();
        if (j.contains("borderWidth"))  s.BorderWidth  = j["borderWidth"].get<float>();
        if (j.contains("paddingX"))     s.PaddingX     = j["paddingX"].get<float>();
        if (j.contains("paddingY"))     s.PaddingY     = j["paddingY"].get<float>();
        if (j.contains("fontSize"))     s.FontSize     = j["fontSize"].get<float>();
        if (j.contains("fontName"))     s.FontName     = j["fontName"].get<std::string>();
    }

    Ref<UITheme> UITheme::LoadFromJSON(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            GE_CORE_ERROR("UITheme: failed to open {}", path);
            return nullptr;
        }
        try {
            nlohmann::json j;
            file >> j;
            auto theme = CreateRef<UITheme>();
            if (j.contains("default")) StyleFromJSON(theme->m_Default, j["default"]);
            if (j.contains("styles")) {
                for (auto& [name, val] : j["styles"].items()) {
                    UIStyle s = theme->m_Default;
                    StyleFromJSON(s, val);
                    theme->m_Styles[name] = s;
                }
            }
            return theme;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("UITheme: JSON parse error: {}", e.what());
            return nullptr;
        }
    }

    static nlohmann::json ColorToJSON(const glm::vec4& c) {
        return { c.r, c.g, c.b, c.a };
    }

    static nlohmann::json StyleToJSON(const UIStyle& s) {
        nlohmann::json j;
        j["background"] = ColorToJSON(s.BackgroundColor);
        j["border"]     = ColorToJSON(s.BorderColor);
        j["text"]       = ColorToJSON(s.TextColor);
        j["hover"]      = ColorToJSON(s.HoverColor);
        j["pressed"]    = ColorToJSON(s.PressedColor);
        j["accent"]     = ColorToJSON(s.AccentColor);
        j["cornerRadius"] = s.CornerRadius;
        j["borderWidth"]  = s.BorderWidth;
        j["paddingX"]     = s.PaddingX;
        j["paddingY"]     = s.PaddingY;
        j["fontSize"]     = s.FontSize;
        j["fontName"]     = s.FontName;
        return j;
    }

    bool UITheme::SaveToJSON(const std::string& path) const {
        nlohmann::json j;
        j["default"] = StyleToJSON(m_Default);
        nlohmann::json styles = nlohmann::json::object();
        for (const auto& [name, style] : m_Styles) {
            styles[name] = StyleToJSON(style);
        }
        j["styles"] = styles;
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << j.dump(2);
        return true;
    }

}}
