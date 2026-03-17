#include "EditorTheme.hpp"

namespace editor {

EditorTheme::EditorTheme() {
    setupStyle();
}

void EditorTheme::applyTheme(Theme theme) {
    m_currentTheme = theme;
    
    switch (theme) {
        case Theme::Dark:      applyDarkTheme(); break;
        case Theme::DarkPro:   applyDarkProTheme(); break;
        case Theme::Light:     applyLightTheme(); break;
        case Theme::Nord:      applyNordTheme(); break;
        case Theme::Dracula:   applyDraculaTheme(); break;
        case Theme::CyberPunk: applyCyberPunkTheme(); break;
        case Theme::Forest:    applyForestTheme(); break;
    }
}

void EditorTheme::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    colors[ImGuiCol_WindowBg]           = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg]            = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.12f, 0.14f, 0.95f);
    colors[ImGuiCol_Border]             = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.25f, 0.25f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.35f, 0.35f, 0.40f, 1.00f);
    colors[ImGuiCol_CheckMark]          = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.55f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]             = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.28f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.35f, 0.35f, 0.45f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.28f, 0.28f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.35f, 0.35f, 0.45f, 1.00f);
    colors[ImGuiCol_Separator]          = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]   = ImVec4(0.35f, 0.50f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive]    = ImVec4(0.45f, 0.60f, 0.80f, 1.00f);
    colors[ImGuiCol_ResizeGrip]         = ImVec4(0.35f, 0.50f, 0.70f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(0.45f, 0.60f, 0.80f, 0.75f);
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(0.55f, 0.70f, 0.90f, 1.00f);
    colors[ImGuiCol_Tab]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(0.35f, 0.50f, 0.70f, 0.80f);
    colors[ImGuiCol_TabActive]          = ImVec4(0.25f, 0.40f, 0.60f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_PlotLines]          = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]   = ImVec4(0.55f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]      = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.55f, 0.85f, 1.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(0.35f, 0.50f, 0.70f, 0.50f);
    colors[ImGuiCol_DragDropTarget]     = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
    colors[ImGuiCol_NavHighlight]       = ImVec4(0.45f, 0.75f, 0.95f, 1.00f);
}

void EditorTheme::applyDarkProTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Professional dark theme with cyan accents
    ImVec4 bg         = ImVec4(0.067f, 0.071f, 0.082f, 1.00f);
    ImVec4 bgChild    = ImVec4(0.078f, 0.082f, 0.094f, 1.00f);
    ImVec4 bgPopup    = ImVec4(0.090f, 0.094f, 0.110f, 0.98f);
    ImVec4 border     = ImVec4(0.145f, 0.153f, 0.176f, 1.00f);
    ImVec4 accent     = ImVec4(0.235f, 0.745f, 0.859f, 1.00f);  // Cyan accent
    ImVec4 accentDim  = ImVec4(0.176f, 0.559f, 0.643f, 1.00f);
    ImVec4 accentBright = ImVec4(0.329f, 0.827f, 0.949f, 1.00f);
    ImVec4 text       = ImVec4(0.918f, 0.925f, 0.933f, 1.00f);
    ImVec4 textDim    = ImVec4(0.600f, 0.620f, 0.660f, 1.00f);
    
    colors[ImGuiCol_Text]               = text;
    colors[ImGuiCol_TextDisabled]       = textDim;
    colors[ImGuiCol_WindowBg]           = bg;
    colors[ImGuiCol_ChildBg]            = bgChild;
    colors[ImGuiCol_PopupBg]            = bgPopup;
    colors[ImGuiCol_Border]             = border;
    colors[ImGuiCol_BorderShadow]       = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg]            = ImVec4(0.110f, 0.118f, 0.137f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.145f, 0.157f, 0.184f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.180f, 0.196f, 0.231f, 1.00f);
    colors[ImGuiCol_TitleBg]            = ImVec4(0.055f, 0.059f, 0.071f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.067f, 0.071f, 0.086f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.045f, 0.047f, 0.059f, 1.00f);
    colors[ImGuiCol_MenuBarBg]          = ImVec4(0.078f, 0.082f, 0.098f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]        = ImVec4(0.055f, 0.059f, 0.071f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]      = ImVec4(0.180f, 0.192f, 0.224f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.220f, 0.235f, 0.275f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.260f, 0.278f, 0.325f, 1.00f);
    colors[ImGuiCol_CheckMark]          = accent;
    colors[ImGuiCol_SliderGrab]         = accentDim;
    colors[ImGuiCol_SliderGrabActive]   = accent;
    colors[ImGuiCol_Button]             = ImVec4(0.137f, 0.145f, 0.169f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.180f, 0.192f, 0.227f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.220f, 0.235f, 0.282f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.137f, 0.145f, 0.169f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.180f, 0.192f, 0.227f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(accentDim.x * 0.5f, accentDim.y * 0.5f, accentDim.z * 0.5f, 1.00f);
    colors[ImGuiCol_Separator]          = border;
    colors[ImGuiCol_SeparatorHovered]   = accentDim;
    colors[ImGuiCol_SeparatorActive]    = accent;
    colors[ImGuiCol_ResizeGrip]         = ImVec4(accentDim.x, accentDim.y, accentDim.z, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]  = ImVec4(accentDim.x, accentDim.y, accentDim.z, 0.67f);
    colors[ImGuiCol_ResizeGripActive]   = ImVec4(accent.x, accent.y, accent.z, 0.95f);
    colors[ImGuiCol_Tab]                = ImVec4(0.098f, 0.102f, 0.122f, 1.00f);
    colors[ImGuiCol_TabHovered]         = ImVec4(accentDim.x * 0.8f, accentDim.y * 0.8f, accentDim.z * 0.8f, 0.80f);
    colors[ImGuiCol_TabActive]          = ImVec4(accentDim.x * 0.6f, accentDim.y * 0.6f, accentDim.z * 0.6f, 1.00f);
    colors[ImGuiCol_TabUnfocused]       = ImVec4(0.078f, 0.082f, 0.098f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.118f, 0.125f, 0.149f, 1.00f);
    colors[ImGuiCol_PlotLines]          = accent;
    colors[ImGuiCol_PlotLinesHovered]   = accentBright;
    colors[ImGuiCol_PlotHistogram]      = accent;
    colors[ImGuiCol_PlotHistogramHovered] = accentBright;
    colors[ImGuiCol_TableHeaderBg]      = ImVec4(0.110f, 0.118f, 0.141f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]  = border;
    colors[ImGuiCol_TableBorderLight]   = ImVec4(border.x * 0.7f, border.y * 0.7f, border.z * 0.7f, 1.0f);
    colors[ImGuiCol_TableRowBg]         = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_TableRowBgAlt]      = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    colors[ImGuiCol_TextSelectedBg]     = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_DragDropTarget]     = accent;
    colors[ImGuiCol_NavHighlight]       = accent;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]  = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]   = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void EditorTheme::applyLightTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsLight();
    
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]        = ImVec4(0.96f, 0.96f, 0.97f, 1.00f);
    colors[ImGuiCol_ChildBg]         = ImVec4(0.94f, 0.94f, 0.95f, 1.00f);
    colors[ImGuiCol_Border]          = ImVec4(0.80f, 0.80f, 0.82f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_Header]          = ImVec4(0.45f, 0.65f, 0.85f, 0.45f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.45f, 0.65f, 0.85f, 0.60f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.45f, 0.65f, 0.85f, 0.80f);
    colors[ImGuiCol_Tab]             = ImVec4(0.88f, 0.88f, 0.90f, 1.00f);
    colors[ImGuiCol_TabHovered]      = ImVec4(0.45f, 0.65f, 0.85f, 0.80f);
    colors[ImGuiCol_TabActive]       = ImVec4(0.55f, 0.75f, 0.95f, 1.00f);
}

void EditorTheme::applyNordTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Nord color palette
    ImVec4 nord0  = ImVec4(0.180f, 0.204f, 0.251f, 1.00f);  // Polar Night
    ImVec4 nord1  = ImVec4(0.231f, 0.259f, 0.322f, 1.00f);
    ImVec4 nord2  = ImVec4(0.263f, 0.298f, 0.369f, 1.00f);
    ImVec4 nord3  = ImVec4(0.298f, 0.337f, 0.416f, 1.00f);
    ImVec4 nord4  = ImVec4(0.847f, 0.871f, 0.914f, 1.00f);  // Snow Storm
    ImVec4 nord8  = ImVec4(0.533f, 0.753f, 0.816f, 1.00f);  // Frost (cyan)
    ImVec4 nord9  = ImVec4(0.506f, 0.631f, 0.757f, 1.00f);  // Frost (blue)
    ImVec4 nord10 = ImVec4(0.369f, 0.506f, 0.675f, 1.00f);  // Frost (darker blue)
    
    colors[ImGuiCol_Text]            = nord4;
    colors[ImGuiCol_WindowBg]        = nord0;
    colors[ImGuiCol_ChildBg]         = ImVec4(nord0.x * 0.95f, nord0.y * 0.95f, nord0.z * 0.95f, 1.0f);
    colors[ImGuiCol_PopupBg]         = nord1;
    colors[ImGuiCol_Border]          = nord3;
    colors[ImGuiCol_FrameBg]         = nord1;
    colors[ImGuiCol_FrameBgHovered]  = nord2;
    colors[ImGuiCol_FrameBgActive]   = nord3;
    colors[ImGuiCol_TitleBg]         = nord0;
    colors[ImGuiCol_TitleBgActive]   = nord1;
    colors[ImGuiCol_Header]          = nord2;
    colors[ImGuiCol_HeaderHovered]   = nord3;
    colors[ImGuiCol_HeaderActive]    = nord10;
    colors[ImGuiCol_Button]          = nord2;
    colors[ImGuiCol_ButtonHovered]   = nord3;
    colors[ImGuiCol_ButtonActive]    = nord10;
    colors[ImGuiCol_CheckMark]       = nord8;
    colors[ImGuiCol_SliderGrab]      = nord9;
    colors[ImGuiCol_SliderGrabActive] = nord8;
    colors[ImGuiCol_Tab]             = nord1;
    colors[ImGuiCol_TabHovered]      = nord9;
    colors[ImGuiCol_TabActive]       = nord10;
}

void EditorTheme::applyDraculaTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Dracula color palette
    ImVec4 bg        = ImVec4(0.157f, 0.165f, 0.212f, 1.00f);
    ImVec4 currentLine = ImVec4(0.275f, 0.278f, 0.353f, 1.00f);
    ImVec4 selection = ImVec4(0.275f, 0.278f, 0.353f, 1.00f);
    ImVec4 foreground = ImVec4(0.973f, 0.973f, 0.949f, 1.00f);
    ImVec4 comment   = ImVec4(0.384f, 0.447f, 0.643f, 1.00f);
    ImVec4 cyan      = ImVec4(0.545f, 0.914f, 0.992f, 1.00f);
    ImVec4 green     = ImVec4(0.314f, 0.980f, 0.482f, 1.00f);
    ImVec4 purple    = ImVec4(0.741f, 0.576f, 0.976f, 1.00f);
    
    colors[ImGuiCol_Text]            = foreground;
    colors[ImGuiCol_TextDisabled]    = comment;
    colors[ImGuiCol_WindowBg]        = bg;
    colors[ImGuiCol_ChildBg]         = ImVec4(bg.x * 0.9f, bg.y * 0.9f, bg.z * 0.9f, 1.0f);
    colors[ImGuiCol_PopupBg]         = currentLine;
    colors[ImGuiCol_Border]          = selection;
    colors[ImGuiCol_FrameBg]         = currentLine;
    colors[ImGuiCol_FrameBgHovered]  = selection;
    colors[ImGuiCol_FrameBgActive]   = ImVec4(purple.x * 0.4f, purple.y * 0.4f, purple.z * 0.4f, 1.0f);
    colors[ImGuiCol_TitleBg]         = bg;
    colors[ImGuiCol_TitleBgActive]   = currentLine;
    colors[ImGuiCol_Header]          = currentLine;
    colors[ImGuiCol_HeaderHovered]   = selection;
    colors[ImGuiCol_HeaderActive]    = ImVec4(purple.x * 0.5f, purple.y * 0.5f, purple.z * 0.5f, 1.0f);
    colors[ImGuiCol_Button]          = currentLine;
    colors[ImGuiCol_ButtonHovered]   = selection;
    colors[ImGuiCol_ButtonActive]    = ImVec4(purple.x * 0.5f, purple.y * 0.5f, purple.z * 0.5f, 1.0f);
    colors[ImGuiCol_CheckMark]       = cyan;
    colors[ImGuiCol_SliderGrab]      = purple;
    colors[ImGuiCol_SliderGrabActive] = cyan;
    colors[ImGuiCol_Tab]             = currentLine;
    colors[ImGuiCol_TabHovered]      = purple;
    colors[ImGuiCol_TabActive]       = ImVec4(purple.x * 0.7f, purple.y * 0.7f, purple.z * 0.7f, 1.0f);
    colors[ImGuiCol_PlotLines]       = cyan;
    colors[ImGuiCol_PlotHistogram]   = green;
    
    (void)green; // Suppress unused warning
}

void EditorTheme::applyCyberPunkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Cyberpunk neon colors
    ImVec4 bg      = ImVec4(0.031f, 0.020f, 0.047f, 1.00f);
    ImVec4 bgLight = ImVec4(0.059f, 0.039f, 0.090f, 1.00f);
    ImVec4 neonPink = ImVec4(1.000f, 0.078f, 0.576f, 1.00f);
    ImVec4 neonCyan = ImVec4(0.000f, 1.000f, 0.957f, 1.00f);
    ImVec4 darkPurple = ImVec4(0.176f, 0.039f, 0.278f, 1.00f);
    
    colors[ImGuiCol_Text]            = ImVec4(0.95f, 0.95f, 0.98f, 1.00f);
    colors[ImGuiCol_WindowBg]        = bg;
    colors[ImGuiCol_ChildBg]         = ImVec4(bg.x * 1.2f, bg.y * 1.2f, bg.z * 1.2f, 1.0f);
    colors[ImGuiCol_PopupBg]         = bgLight;
    colors[ImGuiCol_Border]          = ImVec4(neonPink.x * 0.5f, neonPink.y * 0.5f, neonPink.z * 0.5f, 0.5f);
    colors[ImGuiCol_FrameBg]         = darkPurple;
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(darkPurple.x * 1.5f, darkPurple.y * 1.5f, darkPurple.z * 1.5f, 1.0f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(neonPink.x * 0.3f, neonPink.y * 0.3f, neonPink.z * 0.3f, 1.0f);
    colors[ImGuiCol_TitleBg]         = bg;
    colors[ImGuiCol_TitleBgActive]   = bgLight;
    colors[ImGuiCol_Header]          = darkPurple;
    colors[ImGuiCol_HeaderHovered]   = ImVec4(neonPink.x * 0.4f, neonPink.y * 0.4f, neonPink.z * 0.4f, 1.0f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(neonPink.x * 0.5f, neonPink.y * 0.5f, neonPink.z * 0.5f, 1.0f);
    colors[ImGuiCol_Button]          = darkPurple;
    colors[ImGuiCol_ButtonHovered]   = ImVec4(neonCyan.x * 0.3f, neonCyan.y * 0.3f, neonCyan.z * 0.3f, 1.0f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(neonCyan.x * 0.5f, neonCyan.y * 0.5f, neonCyan.z * 0.5f, 1.0f);
    colors[ImGuiCol_CheckMark]       = neonCyan;
    colors[ImGuiCol_SliderGrab]      = neonPink;
    colors[ImGuiCol_SliderGrabActive] = neonCyan;
    colors[ImGuiCol_Tab]             = darkPurple;
    colors[ImGuiCol_TabHovered]      = neonPink;
    colors[ImGuiCol_TabActive]       = ImVec4(neonPink.x * 0.6f, neonPink.y * 0.6f, neonPink.z * 0.6f, 1.0f);
    colors[ImGuiCol_PlotLines]       = neonCyan;
    colors[ImGuiCol_PlotHistogram]   = neonPink;
}

void EditorTheme::applyForestTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;
    
    // Forest/nature inspired colors
    ImVec4 darkGreen   = ImVec4(0.059f, 0.094f, 0.071f, 1.00f);
    ImVec4 forestGreen = ImVec4(0.133f, 0.208f, 0.145f, 1.00f);
    ImVec4 leafGreen   = ImVec4(0.298f, 0.545f, 0.302f, 1.00f);
    ImVec4 moss        = ImVec4(0.482f, 0.635f, 0.306f, 1.00f);
    ImVec4 cream       = ImVec4(0.961f, 0.957f, 0.918f, 1.00f);
    
    colors[ImGuiCol_Text]            = cream;
    colors[ImGuiCol_TextDisabled]    = ImVec4(cream.x * 0.5f, cream.y * 0.5f, cream.z * 0.5f, 1.0f);
    colors[ImGuiCol_WindowBg]        = darkGreen;
    colors[ImGuiCol_ChildBg]         = ImVec4(darkGreen.x * 1.1f, darkGreen.y * 1.1f, darkGreen.z * 1.1f, 1.0f);
    colors[ImGuiCol_PopupBg]         = forestGreen;
    colors[ImGuiCol_Border]          = ImVec4(leafGreen.x * 0.5f, leafGreen.y * 0.5f, leafGreen.z * 0.5f, 1.0f);
    colors[ImGuiCol_FrameBg]         = forestGreen;
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(forestGreen.x * 1.3f, forestGreen.y * 1.3f, forestGreen.z * 1.3f, 1.0f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(leafGreen.x * 0.5f, leafGreen.y * 0.5f, leafGreen.z * 0.5f, 1.0f);
    colors[ImGuiCol_TitleBg]         = darkGreen;
    colors[ImGuiCol_TitleBgActive]   = forestGreen;
    colors[ImGuiCol_Header]          = forestGreen;
    colors[ImGuiCol_HeaderHovered]   = ImVec4(leafGreen.x * 0.5f, leafGreen.y * 0.5f, leafGreen.z * 0.5f, 1.0f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(leafGreen.x * 0.7f, leafGreen.y * 0.7f, leafGreen.z * 0.7f, 1.0f);
    colors[ImGuiCol_Button]          = forestGreen;
    colors[ImGuiCol_ButtonHovered]   = leafGreen;
    colors[ImGuiCol_ButtonActive]    = moss;
    colors[ImGuiCol_CheckMark]       = moss;
    colors[ImGuiCol_SliderGrab]      = leafGreen;
    colors[ImGuiCol_SliderGrabActive] = moss;
    colors[ImGuiCol_Tab]             = forestGreen;
    colors[ImGuiCol_TabHovered]      = leafGreen;
    colors[ImGuiCol_TabActive]       = ImVec4(leafGreen.x * 0.8f, leafGreen.y * 0.8f, leafGreen.z * 0.8f, 1.0f);
    colors[ImGuiCol_PlotLines]       = moss;
    colors[ImGuiCol_PlotHistogram]   = leafGreen;
}

const std::string& EditorTheme::getThemeName() const {
    static const std::string names[] = {
        "Dark", "Dark Pro", "Light", "Nord", "Dracula", "CyberPunk", "Forest"
    };
    return names[static_cast<int>(m_currentTheme)];
}

void EditorTheme::setupStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Rounding
    style.WindowRounding = 4.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 3.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    
    // Sizing
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 20.0f;
    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 12.0f;
    
    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;
    
    // Alignment
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
}

ImVec4 EditorTheme::getAccentColor() {
    return ImVec4(0.235f, 0.745f, 0.859f, 1.00f);
}

ImVec4 EditorTheme::getWarningColor() {
    return ImVec4(0.95f, 0.75f, 0.25f, 1.00f);
}

ImVec4 EditorTheme::getErrorColor() {
    return ImVec4(0.95f, 0.30f, 0.30f, 1.00f);
}

ImVec4 EditorTheme::getSuccessColor() {
    return ImVec4(0.30f, 0.85f, 0.45f, 1.00f);
}

ImVec4 EditorTheme::getHierarchyItemColor(bool selected, bool hovered) {
    if (selected) return ImVec4(0.235f, 0.745f, 0.859f, 0.65f);
    if (hovered) return ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
}

ImVec4 EditorTheme::getInspectorHeaderColor() {
    return ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
}

ImVec4 EditorTheme::getViewportBorderColor() {
    return ImVec4(0.235f, 0.745f, 0.859f, 0.50f);
}

} // namespace editor
