#pragma once

#include <imgui.h>
#include <string>
#include <functional>

namespace editor::widgets {

/**
 * @brief Reusable property editor widgets
 */

// Generic property row with label
inline void beginPropertyRow(const char* label, float labelWidth = 100.0f) {
    ImGui::PushID(label);
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, labelWidth);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
}

inline void endPropertyRow() {
    ImGui::Columns(1);
    ImGui::PopID();
}

// Float property
inline bool propertyFloat(const char* label, float* value, float min = 0.0f, float max = 0.0f, 
                          float speed = 0.1f, const char* format = "%.2f") {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::DragFloat("##value", value, speed, min, max, format);
    endPropertyRow();
    return changed;
}

// Int property
inline bool propertyInt(const char* label, int* value, int min = 0, int max = 0, int speed = 1) {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::DragInt("##value", value, static_cast<float>(speed), min, max);
    endPropertyRow();
    return changed;
}

// Bool property
inline bool propertyBool(const char* label, bool* value) {
    beginPropertyRow(label);
    bool changed = ImGui::Checkbox("##value", value);
    endPropertyRow();
    return changed;
}

// Color property
inline bool propertyColor3(const char* label, float* color) {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::ColorEdit3("##color", color, ImGuiColorEditFlags_NoInputs);
    endPropertyRow();
    return changed;
}

inline bool propertyColor4(const char* label, float* color) {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::ColorEdit4("##color", color, ImGuiColorEditFlags_NoInputs);
    endPropertyRow();
    return changed;
}

// String property
inline bool propertyString(const char* label, char* buffer, size_t bufferSize) {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::InputText("##value", buffer, bufferSize);
    endPropertyRow();
    return changed;
}

// Dropdown property
inline bool propertyDropdown(const char* label, int* currentItem, const char* const* items, int itemCount) {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::Combo("##value", currentItem, items, itemCount);
    endPropertyRow();
    return changed;
}

// Slider property
inline bool propertySlider(const char* label, float* value, float min, float max, const char* format = "%.2f") {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::SliderFloat("##value", value, min, max, format);
    endPropertyRow();
    return changed;
}

inline bool propertySliderInt(const char* label, int* value, int min, int max, const char* format = "%d") {
    beginPropertyRow(label);
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::SliderInt("##value", value, min, max, format);
    endPropertyRow();
    return changed;
}

// Vector3 property with colored XYZ buttons
inline bool propertyVec3(const char* label, float* values, float resetValue = 0.0f, float speed = 0.1f) {
    bool changed = false;
    
    ImGui::PushID(label);
    
    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    float lineHeight = ImGui::GetFrameHeight();
    ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };
    
    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.1f, 0.15f, 1.0f));
    if (ImGui::Button("X", buttonSize)) {
        values[0] = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    if (ImGui::DragFloat("##X", &values[0], speed)) changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();
    
    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    if (ImGui::Button("Y", buttonSize)) {
        values[1] = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    if (ImGui::DragFloat("##Y", &values[1], speed)) changed = true;
    ImGui::PopItemWidth();
    ImGui::SameLine();
    
    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.35f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.25f, 0.8f, 1.0f));
    if (ImGui::Button("Z", buttonSize)) {
        values[2] = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    ImGui::SameLine();
    if (ImGui::DragFloat("##Z", &values[2], speed)) changed = true;
    ImGui::PopItemWidth();
    
    ImGui::PopStyleVar();
    ImGui::Columns(1);
    ImGui::PopID();
    
    return changed;
}

// Read-only text property
inline void propertyReadOnly(const char* label, const char* value) {
    beginPropertyRow(label);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
    ImGui::Text("%s", value);
    ImGui::PopStyleColor();
    endPropertyRow();
}

// Button property
inline bool propertyButton(const char* label, const char* buttonText, const ImVec2& size = ImVec2(0, 0)) {
    beginPropertyRow(label);
    bool clicked = ImGui::Button(buttonText, size.x > 0 ? size : ImVec2(-1, 0));
    endPropertyRow();
    return clicked;
}

// Collapsing header with options button
inline bool componentHeader(const char* label, bool* enabled = nullptr, bool removable = false, 
                           std::function<void()> optionsCallback = nullptr) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | 
                               ImGuiTreeNodeFlags_Framed |
                               ImGuiTreeNodeFlags_AllowItemOverlap |
                               ImGuiTreeNodeFlags_FramePadding;
    
    bool open = ImGui::TreeNodeEx(label, flags);
    
    // Options button on the right
    ImGui::SameLine(ImGui::GetWindowWidth() - (removable ? 50 : 30));
    
    if (enabled) {
        if (ImGui::Checkbox(("##" + std::string(label) + "Enabled").c_str(), enabled)) {
            // Handle enable/disable
        }
        ImGui::SameLine();
    }
    
    if (ImGui::Button(("\xef\x81\x93##" + std::string(label) + "Options").c_str(), ImVec2(20, 20))) {
        ImGui::OpenPopup((std::string(label) + "OptionsPopup").c_str());
    }
    
    if (ImGui::BeginPopup((std::string(label) + "OptionsPopup").c_str())) {
        if (ImGui::MenuItem("Reset")) {}
        if (ImGui::MenuItem("Copy")) {}
        if (ImGui::MenuItem("Paste")) {}
        if (removable) {
            ImGui::Separator();
            if (ImGui::MenuItem("Remove Component")) {}
        }
        if (optionsCallback) {
            ImGui::Separator();
            optionsCallback();
        }
        ImGui::EndPopup();
    }
    
    ImGui::PopStyleVar();
    
    return open;
}

} // namespace editor::widgets

