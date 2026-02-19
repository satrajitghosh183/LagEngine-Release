#include "ToolbarPanel.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace GameEngine {

    void ToolbarPanel::OnImGuiRender() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        
        auto& colors = ImGui::GetStyle().Colors;
        const auto& buttonHovered = colors[ImGuiCol_ButtonHovered];
        const auto& buttonActive = colors[ImGuiCol_ButtonActive];
        
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonHovered.x, buttonHovered.y, buttonHovered.z, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonActive.x, buttonActive.y, buttonActive.z, 0.5f));
        
        ImGui::Begin("##toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        
        float size = ImGui::GetWindowHeight() - 4.0f;
        
        // Center the buttons
        float windowWidth = ImGui::GetWindowWidth();
        float buttonsWidth = size * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
        ImGui::SetCursorPosX((windowWidth - buttonsWidth) * 0.5f);
        
        if (m_Context) {
            EditorState state = m_Context->GetState();
            bool isEditing = (state == EditorState::Edit);
            bool isPlaying = (state == EditorState::Play);
            bool isPaused = (state == EditorState::Pause);
            
            // Play button
            if (isPlaying) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.8f, 0.2f, 0.7f));
            }
            
            if (ImGui::Button(isEditing ? "|>" : "||", ImVec2(size, size))) {
                if (isEditing) {
                    m_Context->Play();
                } else if (isPlaying) {
                    m_Context->Pause();
                } else if (isPaused) {
                    m_Context->Resume();
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip(isEditing ? "Play (Ctrl+P)" : (isPlaying ? "Pause" : "Resume"));
            }
            
            if (isPlaying) {
                ImGui::PopStyleColor();
            }
            
            ImGui::SameLine();
            
            // Stop button
            bool canStop = !isEditing;
            if (!canStop) {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            }
            
            if (ImGui::Button("[]", ImVec2(size, size)) && canStop) {
                m_Context->Stop();
            }
            if (ImGui::IsItemHovered() && canStop) {
                ImGui::SetTooltip("Stop (Ctrl+Shift+P)");
            }
            
            if (!canStop) {
                ImGui::PopStyleVar();
            }
            
            ImGui::SameLine();
            
            // State indicator
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 20);
            ImVec4 stateColor;
            const char* stateText;
            
            switch (state) {
                case EditorState::Edit:
                    stateColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
                    stateText = "Edit Mode";
                    break;
                case EditorState::Play:
                    stateColor = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                    stateText = "Playing";
                    break;
                case EditorState::Pause:
                    stateColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f);
                    stateText = "Paused";
                    break;
            }
            
            ImGui::TextColored(stateColor, "%s", stateText);
        } else {
            // No context - show disabled buttons
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
            ImGui::Button("|>", ImVec2(size, size));
            ImGui::SameLine();
            ImGui::Button("[]", ImVec2(size, size));
            ImGui::PopStyleVar();
        }
        
        ImGui::End();
        
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

}
