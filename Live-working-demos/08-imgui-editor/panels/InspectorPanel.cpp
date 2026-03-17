#include "InspectorPanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace editor {

InspectorPanel::InspectorPanel() 
    : EditorPanel("Inspector", true) {
}

void InspectorPanel::render() {
    if (!beginPanel()) {
        endPanel();
        return;
    }
    
    if (!m_sceneRenderer) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "No scene renderer");
        endPanel();
        return;
    }
    
    SceneObject* selected = m_sceneRenderer->getSelectedObject();
    
    if (!selected) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select an object to view its properties");
        endPanel();
        return;
    }
    
    // Object header
    renderObjectInfo(selected);
    
    ImGui::Separator();
    ImGui::Spacing();
    
    // Transform component
    renderTransformComponent(selected);
    
    ImGui::Spacing();
    
    // Appearance component
    renderAppearanceComponent(selected);
    
    endPanel();
}

void InspectorPanel::renderObjectInfo(SceneObject* obj) {
    // Object name (editable)
    static char nameBuffer[256];
    strncpy(nameBuffer, obj->name.c_str(), sizeof(nameBuffer) - 1);
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
    
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##ObjectName", nameBuffer, sizeof(nameBuffer))) {
        obj->name = nameBuffer;
    }
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    // Object type (readonly)
    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.55f, 1.0f), "Type: %s", obj->type.c_str());
    
    // Visibility checkbox
    ImGui::Checkbox("Visible", &obj->visible);
}

void InspectorPanel::renderTransformComponent(SceneObject* obj) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap;
    
    bool open = ImGui::TreeNodeEx("Transform", flags);
    
    if (open) {
        ImGui::Spacing();
        
        // Position
        drawVec3Control("Position", obj->transform.position, 0.0f);
        
        // Rotation
        drawVec3Control("Rotation", obj->transform.rotation, 0.0f);
        
        // Scale
        drawVec3Control("Scale", obj->transform.scale, 1.0f);
        
        ImGui::TreePop();
    }
    
    ImGui::PopStyleVar();
}

void InspectorPanel::renderAppearanceComponent(SceneObject* obj) {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed;
    
    bool open = ImGui::TreeNodeEx("Appearance", flags);
    
    if (open) {
        ImGui::Spacing();
        
        // Color picker
        drawColorControl("Color", obj->color);
        
        // Mesh type info
        ImGui::Text("Mesh: %s", obj->type.c_str());
        
        ImGui::TreePop();
    }
    
    ImGui::PopStyleVar();
}

bool InspectorPanel::drawVec3Control(const char* label, glm::vec3& values, float resetValue, float columnWidth) {
    bool changed = false;
    
    ImGui::PushID(label);
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, columnWidth);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    float itemWidth = (ImGui::GetContentRegionAvail().x - 8.0f * 2) / 3.0f;
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    
    // X
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("X", ImVec2(20, 20))) {
        values.x = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth - 24);
    if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f")) {
        changed = true;
    }
    
    ImGui::SameLine();
    
    // Y
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.6f, 0.1f, 1.0f));
    if (ImGui::Button("Y", ImVec2(20, 20))) {
        values.y = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth - 24);
    if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f")) {
        changed = true;
    }
    
    ImGui::SameLine();
    
    // Z
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.3f, 0.8f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.4f, 0.9f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.2f, 0.7f, 1.0f));
    if (ImGui::Button("Z", ImVec2(20, 20))) {
        values.z = resetValue;
        changed = true;
    }
    ImGui::PopStyleColor(3);
    
    ImGui::SameLine();
    ImGui::SetNextItemWidth(itemWidth - 24);
    if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f")) {
        changed = true;
    }
    
    ImGui::PopStyleVar();
    
    ImGui::Columns(1);
    
    ImGui::PopID();
    
    return changed;
}

bool InspectorPanel::drawFloatControl(const char* label, float* value, float min, float max, const char* format) {
    ImGui::PushID(label);
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::SliderFloat("##value", value, min, max, format);
    
    ImGui::Columns(1);
    ImGui::PopID();
    
    return changed;
}

bool InspectorPanel::drawColorControl(const char* label, glm::vec3& color) {
    ImGui::PushID(label);
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    ImGui::SetNextItemWidth(-1);
    bool changed = ImGui::ColorEdit3("##color", glm::value_ptr(color));
    
    ImGui::Columns(1);
    ImGui::PopID();
    
    return changed;
}

bool InspectorPanel::drawCheckbox(const char* label, bool* value) {
    ImGui::PushID(label);
    
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    bool changed = ImGui::Checkbox("##checkbox", value);
    
    ImGui::Columns(1);
    ImGui::PopID();
    
    return changed;
}

void InspectorPanel::drawReadonlyText(const char* label, const char* text) {
    ImGui::Columns(2);
    ImGui::SetColumnWidth(0, 100.0f);
    ImGui::Text("%s", label);
    ImGui::NextColumn();
    
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", text);
    
    ImGui::Columns(1);
}

void InspectorPanel::update(float /*dt*/) {
    // Update logic if needed
}

} // namespace editor
