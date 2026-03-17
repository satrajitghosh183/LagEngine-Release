#pragma once

#include <imgui.h>
#include <string>
#include <filesystem>

namespace editor::utils {

/**
 * @brief Utility functions for the editor
 */

// String utilities
inline std::string truncateString(const std::string& str, size_t maxLength) {
    if (str.length() <= maxLength) return str;
    return str.substr(0, maxLength - 3) + "...";
}

// File utilities
inline std::string getFileExtension(const std::filesystem::path& path) {
    return path.extension().string();
}

inline std::string getFileName(const std::filesystem::path& path) {
    return path.filename().string();
}

inline std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
    }
    
    char buffer[32];
    if (unit == 0) {
        snprintf(buffer, sizeof(buffer), "%zu %s", bytes, units[unit]);
    } else {
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    }
    
    return buffer;
}

// ImGui utilities
inline void helpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

inline void centeredText(const char* text) {
    float windowWidth = ImGui::GetWindowSize().x;
    float textWidth = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text("%s", text);
}

inline bool centeredButton(const char* label, const ImVec2& size = ImVec2(0, 0)) {
    float windowWidth = ImGui::GetWindowSize().x;
    float buttonWidth = size.x > 0 ? size.x : ImGui::CalcTextSize(label).x + ImGui::GetStyle().FramePadding.x * 2.0f;
    ImGui::SetCursorPosX((windowWidth - buttonWidth) * 0.5f);
    return ImGui::Button(label, size);
}

// Scoped style helpers
class ScopedID {
public:
    ScopedID(int id) { ImGui::PushID(id); }
    ScopedID(const char* id) { ImGui::PushID(id); }
    ScopedID(void* id) { ImGui::PushID(id); }
    ~ScopedID() { ImGui::PopID(); }
};

class ScopedColor {
public:
    ScopedColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
    ScopedColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
    ~ScopedColor() { ImGui::PopStyleColor(); }
};

class ScopedStyleVar {
public:
    ScopedStyleVar(ImGuiStyleVar idx, float val) { ImGui::PushStyleVar(idx, val); }
    ScopedStyleVar(ImGuiStyleVar idx, const ImVec2& val) { ImGui::PushStyleVar(idx, val); }
    ~ScopedStyleVar() { ImGui::PopStyleVar(); }
};

class ScopedDisabled {
public:
    ScopedDisabled(bool disabled = true) : m_disabled(disabled) { 
        if (m_disabled) ImGui::BeginDisabled(); 
    }
    ~ScopedDisabled() { 
        if (m_disabled) ImGui::EndDisabled(); 
    }
private:
    bool m_disabled;
};

class ScopedGroup {
public:
    ScopedGroup() { ImGui::BeginGroup(); }
    ~ScopedGroup() { ImGui::EndGroup(); }
};

// Math utilities
inline float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

inline float clamp(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Color utilities
inline ImVec4 lerpColor(const ImVec4& a, const ImVec4& b, float t) {
    return ImVec4(
        lerp(a.x, b.x, t),
        lerp(a.y, b.y, t),
        lerp(a.z, b.z, t),
        lerp(a.w, b.w, t)
    );
}

inline ImU32 colorToU32(const ImVec4& col) {
    return ImGui::ColorConvertFloat4ToU32(col);
}

inline ImVec4 u32ToColor(ImU32 col) {
    ImVec4 result;
    result.x = ((col >> 0) & 0xFF) / 255.0f;
    result.y = ((col >> 8) & 0xFF) / 255.0f;
    result.z = ((col >> 16) & 0xFF) / 255.0f;
    result.w = ((col >> 24) & 0xFF) / 255.0f;
    return result;
}

} // namespace editor::utils

