#include "ThemeEditorWindow.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Platform/FileSystem.hpp"
#include <nlohmann/json.hpp>
#include <fstream>

namespace GameEngine {

    ThemeEditorWindow::ThemeEditorWindow() {
        // Initialize with current ImGui style
        m_Style = ImGui::GetStyle();
    }

    void ThemeEditorWindow::OnImGuiRender() {
        if (!m_IsOpen) return;

        ImGui::Begin("Theme Editor", &m_IsOpen);

        if (ImGui::Button("Reset to Default")) {
            ResetToDefault();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save Theme...")) {
            // TODO: Show file dialog
            SaveTheme("themes/custom.json");
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Theme...")) {
            // TODO: Show file dialog
            LoadTheme("themes/custom.json");
        }

        ImGui::Separator();

        if (ImGui::BeginTabBar("ThemeTabs")) {
            if (ImGui::BeginTabItem("Colors")) {
                RenderColorEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Sizing")) {
                RenderSizingEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Rounding")) {
                RenderRoundingEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Alpha")) {
                RenderAlphaEditor();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Presets")) {
                RenderPresets();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Apply style changes
        ApplyStyle();

        ImGui::End();
    }

    void ThemeEditorWindow::RenderColorEditor() {
        ImGui::Text("Color Settings");
        ImGui::Separator();

        // Common colors
        struct ColorEdit {
            const char* Name;
            ImGuiCol Color;
        };

        ColorEdit colors[] = {
            { "Text", ImGuiCol_Text },
            { "Text Disabled", ImGuiCol_TextDisabled },
            { "Window Background", ImGuiCol_WindowBg },
            { "Child Background", ImGuiCol_ChildBg },
            { "Popup Background", ImGuiCol_PopupBg },
            { "Border", ImGuiCol_Border },
            { "Border Shadow", ImGuiCol_BorderShadow },
            { "Frame Background", ImGuiCol_FrameBg },
            { "Frame Background Hovered", ImGuiCol_FrameBgHovered },
            { "Frame Background Active", ImGuiCol_FrameBgActive },
            { "Title Background", ImGuiCol_TitleBg },
            { "Title Background Active", ImGuiCol_TitleBgActive },
            { "Title Background Collapsed", ImGuiCol_TitleBgCollapsed },
            { "Menu Bar Background", ImGuiCol_MenuBarBg },
            { "Scrollbar Background", ImGuiCol_ScrollbarBg },
            { "Scrollbar Grab", ImGuiCol_ScrollbarGrab },
            { "Scrollbar Grab Hovered", ImGuiCol_ScrollbarGrabHovered },
            { "Scrollbar Grab Active", ImGuiCol_ScrollbarGrabActive },
            { "Check Mark", ImGuiCol_CheckMark },
            { "Slider Grab", ImGuiCol_SliderGrab },
            { "Slider Grab Active", ImGuiCol_SliderGrabActive },
            { "Button", ImGuiCol_Button },
            { "Button Hovered", ImGuiCol_ButtonHovered },
            { "Button Active", ImGuiCol_ButtonActive },
            { "Header", ImGuiCol_Header },
            { "Header Hovered", ImGuiCol_HeaderHovered },
            { "Header Active", ImGuiCol_HeaderActive },
            { "Separator", ImGuiCol_Separator },
            { "Separator Hovered", ImGuiCol_SeparatorHovered },
            { "Separator Active", ImGuiCol_SeparatorActive },
            { "Resize Grip", ImGuiCol_ResizeGrip },
            { "Resize Grip Hovered", ImGuiCol_ResizeGripHovered },
            { "Resize Grip Active", ImGuiCol_ResizeGripActive },
            { "Tab", ImGuiCol_Tab },
            { "Tab Hovered", ImGuiCol_TabHovered },
            { "Tab Active", ImGuiCol_TabActive },
            { "Tab Unfocused", ImGuiCol_TabUnfocused },
            { "Tab Unfocused Active", ImGuiCol_TabUnfocusedActive },
            { "Plot Lines", ImGuiCol_PlotLines },
            { "Plot Lines Hovered", ImGuiCol_PlotLinesHovered },
            { "Plot Histogram", ImGuiCol_PlotHistogram },
            { "Plot Histogram Hovered", ImGuiCol_PlotHistogramHovered },
            { "Table Header Background", ImGuiCol_TableHeaderBg },
            { "Table Border Strong", ImGuiCol_TableBorderStrong },
            { "Table Border Light", ImGuiCol_TableBorderLight },
            { "Table Row Background", ImGuiCol_TableRowBg },
            { "Table Row Background Alt", ImGuiCol_TableRowBgAlt },
            { "Text Selected Background", ImGuiCol_TextSelectedBg },
            { "Drag Drop Target", ImGuiCol_DragDropTarget },
            { "Nav Highlight", ImGuiCol_NavHighlight },
            { "Nav Windowing Highlight", ImGuiCol_NavWindowingHighlight },
            { "Nav Windowing Dim Background", ImGuiCol_NavWindowingDimBg },
            { "Modal Window Dim Background", ImGuiCol_ModalWindowDimBg },
        };

        for (const auto& colorEdit : colors) {
            ImVec4& color = m_Style.Colors[colorEdit.Color];
            if (ImGui::ColorEdit4(colorEdit.Name, (float*)&color)) {
                // Color changed
            }
        }
    }

    void ThemeEditorWindow::RenderSizingEditor() {
        ImGui::Text("Sizing Settings");
        ImGui::Separator();

        ImGui::DragFloat("Alpha", &m_Style.Alpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Disabled Alpha", &m_Style.DisabledAlpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat2("Window Padding", (float*)&m_Style.WindowPadding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat2("Frame Padding", (float*)&m_Style.FramePadding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat2("Item Spacing", (float*)&m_Style.ItemSpacing, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat2("Item Inner Spacing", (float*)&m_Style.ItemInnerSpacing, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat2("Cell Padding", (float*)&m_Style.CellPadding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat2("Touch Extra Padding", (float*)&m_Style.TouchExtraPadding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Indent Spacing", &m_Style.IndentSpacing, 1.0f, 0.0f, 100.0f);
        ImGui::DragFloat("Columns Min Spacing", &m_Style.ColumnsMinSpacing, 1.0f, 0.0f, 100.0f);
        ImGui::DragFloat("Scrollbar Size", &m_Style.ScrollbarSize, 1.0f, 0.0f, 50.0f);
        ImGui::DragFloat("Grab Min Size", &m_Style.GrabMinSize, 1.0f, 0.0f, 50.0f);
    }

    void ThemeEditorWindow::RenderRoundingEditor() {
        ImGui::Text("Rounding Settings");
        ImGui::Separator();

        ImGui::DragFloat("Window Rounding", &m_Style.WindowRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Child Rounding", &m_Style.ChildRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Frame Rounding", &m_Style.FrameRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Popup Rounding", &m_Style.PopupRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Scrollbar Rounding", &m_Style.ScrollbarRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Grab Rounding", &m_Style.GrabRounding, 0.5f, 0.0f, 20.0f);
        ImGui::DragFloat("Tab Rounding", &m_Style.TabRounding, 0.5f, 0.0f, 20.0f);
    }

    void ThemeEditorWindow::RenderAlphaEditor() {
        ImGui::Text("Alpha Settings");
        ImGui::Separator();

        ImGui::DragFloat("Global Alpha", &m_Style.Alpha, 0.01f, 0.0f, 1.0f);
        ImGui::DragFloat("Disabled Alpha", &m_Style.DisabledAlpha, 0.01f, 0.0f, 1.0f);
    }

    void ThemeEditorWindow::RenderPresets() {
        ImGui::Text("Theme Presets");
        ImGui::Separator();

        for (const auto& preset : m_PresetNames) {
            if (ImGui::Button(preset.c_str(), ImVec2(200, 0))) {
                ApplyPreset(preset);
            }
        }
    }

    void ThemeEditorWindow::ApplyStyle() {
        ImGui::GetStyle() = m_Style;
    }

    void ThemeEditorWindow::ResetToDefault() {
        m_Style = ImGuiStyle();
        ImGui::StyleColorsDark();  // Apply default dark theme
        m_Style = ImGui::GetStyle();
    }

    bool ThemeEditorWindow::SaveTheme(const std::string& filepath) {
        nlohmann::json theme;
        
        // Save colors
        theme["colors"] = nlohmann::json::object();
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            ImVec4& color = m_Style.Colors[i];
            theme["colors"][std::to_string(i)] = {
                color.x, color.y, color.z, color.w
            };
        }
        
        // Save sizing
        theme["alpha"] = m_Style.Alpha;
        theme["disabledAlpha"] = m_Style.DisabledAlpha;
        theme["windowPadding"] = { m_Style.WindowPadding.x, m_Style.WindowPadding.y };
        theme["framePadding"] = { m_Style.FramePadding.x, m_Style.FramePadding.y };
        theme["itemSpacing"] = { m_Style.ItemSpacing.x, m_Style.ItemSpacing.y };
        theme["itemInnerSpacing"] = { m_Style.ItemInnerSpacing.x, m_Style.ItemInnerSpacing.y };
        theme["indentSpacing"] = m_Style.IndentSpacing;
        theme["scrollbarSize"] = m_Style.ScrollbarSize;
        theme["grabMinSize"] = m_Style.GrabMinSize;
        
        // Save rounding
        theme["windowRounding"] = m_Style.WindowRounding;
        theme["childRounding"] = m_Style.ChildRounding;
        theme["frameRounding"] = m_Style.FrameRounding;
        theme["popupRounding"] = m_Style.PopupRounding;
        theme["scrollbarRounding"] = m_Style.ScrollbarRounding;
        theme["grabRounding"] = m_Style.GrabRounding;
        theme["tabRounding"] = m_Style.TabRounding;
        
        try {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                GE_CORE_ERROR("Failed to save theme to: {}", filepath);
                return false;
            }
            file << theme.dump(4);
            file.close();
            GE_CORE_INFO("Theme saved to: {}", filepath);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Error saving theme: {}", e.what());
            return false;
        }
    }

    bool ThemeEditorWindow::LoadTheme(const std::string& filepath) {
        if (!FileSystem::Exists(filepath)) {
            GE_CORE_ERROR("Theme file not found: {}", filepath);
            return false;
        }
        
        try {
            std::ifstream file(filepath);
            nlohmann::json theme;
            file >> theme;
            file.close();
            
            // Load colors
            if (theme.contains("colors")) {
                for (int i = 0; i < ImGuiCol_COUNT; ++i) {
                    std::string key = std::to_string(i);
                    if (theme["colors"].contains(key)) {
                        auto color = theme["colors"][key];
                        m_Style.Colors[i] = ImVec4(
                            color[0], color[1], color[2], color[3]
                        );
                    }
                }
            }
            
            // Load sizing
            if (theme.contains("alpha")) m_Style.Alpha = theme["alpha"];
            if (theme.contains("disabledAlpha")) m_Style.DisabledAlpha = theme["disabledAlpha"];
            if (theme.contains("windowPadding")) {
                m_Style.WindowPadding = ImVec2(theme["windowPadding"][0], theme["windowPadding"][1]);
            }
            if (theme.contains("framePadding")) {
                m_Style.FramePadding = ImVec2(theme["framePadding"][0], theme["framePadding"][1]);
            }
            if (theme.contains("itemSpacing")) {
                m_Style.ItemSpacing = ImVec2(theme["itemSpacing"][0], theme["itemSpacing"][1]);
            }
            if (theme.contains("itemInnerSpacing")) {
                m_Style.ItemInnerSpacing = ImVec2(theme["itemInnerSpacing"][0], theme["itemInnerSpacing"][1]);
            }
            if (theme.contains("indentSpacing")) m_Style.IndentSpacing = theme["indentSpacing"];
            if (theme.contains("scrollbarSize")) m_Style.ScrollbarSize = theme["scrollbarSize"];
            if (theme.contains("grabMinSize")) m_Style.GrabMinSize = theme["grabMinSize"];
            
            // Load rounding
            if (theme.contains("windowRounding")) m_Style.WindowRounding = theme["windowRounding"];
            if (theme.contains("childRounding")) m_Style.ChildRounding = theme["childRounding"];
            if (theme.contains("frameRounding")) m_Style.FrameRounding = theme["frameRounding"];
            if (theme.contains("popupRounding")) m_Style.PopupRounding = theme["popupRounding"];
            if (theme.contains("scrollbarRounding")) m_Style.ScrollbarRounding = theme["scrollbarRounding"];
            if (theme.contains("grabRounding")) m_Style.GrabRounding = theme["grabRounding"];
            if (theme.contains("tabRounding")) m_Style.TabRounding = theme["tabRounding"];
            
            ApplyStyle();
            GE_CORE_INFO("Theme loaded from: {}", filepath);
            return true;
        } catch (const std::exception& e) {
            GE_CORE_ERROR("Error loading theme: {}", e.what());
            return false;
        }
    }

    void ThemeEditorWindow::ApplyPreset(const std::string& presetName) {
        if (presetName == "Default" || presetName == "Dark") {
            ImGui::StyleColorsDark();
            m_Style = ImGui::GetStyle();
        } else if (presetName == "Light") {
            ImGui::StyleColorsLight();
            m_Style = ImGui::GetStyle();
        } else if (presetName == "Classic") {
            ImGui::StyleColorsClassic();
            m_Style = ImGui::GetStyle();
        }
        ApplyStyle();
    }

}
