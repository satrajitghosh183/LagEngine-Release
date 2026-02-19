#include "ScriptingConsolePanel.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Scripting/ScriptEngine.hpp"
#include <imgui.h>
#include <algorithm>

namespace GameEngine {

    ScriptingConsolePanel::ScriptingConsolePanel() {
        AddLog("Scripting Console Ready. Press HOME to toggle.");
    }

    void ScriptingConsolePanel::OnImGuiRender() {
        if (!m_IsOpen) return;

        ImGui::Begin("Scripting Console", &m_IsOpen, ImGuiWindowFlags_None);

        // Log/scrollback area
        const float footerHeightToReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footerHeightToReserve), false, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
        
        for (const auto& log : m_Logs) {
            ImVec4 color = log.IsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted(log.Text.c_str());
            ImGui::PopStyleColor();
        }

        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();

        // Separator
        ImGui::Separator();

        // Input area
        bool reclaimFocus = false;
        ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

        if (ImGui::InputText("##Input", m_InputBuffer, sizeof(m_InputBuffer), inputFlags)) {
            std::string command = m_InputBuffer;
            if (!command.empty()) {
                // Add to history
                if (m_History.empty() || m_History.back() != command) {
                    m_History.push_back(command);
                    if (m_History.size() > MAX_HISTORY) {
                        m_History.pop_front();
                    }
                }
                m_HistoryIndex = -1;

                // Execute command
                ExecuteScript(command);

                // Clear input
                m_InputBuffer[0] = '\0';
                reclaimFocus = true;
            }
        }

        // Auto-focus on window appearing
        ImGui::SetItemDefaultFocus();
        if (reclaimFocus) {
            ImGui::SetKeyboardFocusHere(-1);
        }

        // Buttons
        ImGui::SameLine();
        if (ImGui::Button("Execute")) {
            std::string command = m_InputBuffer;
            if (!command.empty()) {
                if (m_History.empty() || m_History.back() != command) {
                    m_History.push_back(command);
                    if (m_History.size() > MAX_HISTORY) {
                        m_History.pop_front();
                    }
                }
                m_HistoryIndex = -1;
                ExecuteScript(command);
                m_InputBuffer[0] = '\0';
                reclaimFocus = true;
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            ClearLog();
        }

        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);

        ImGui::End();
    }

    void ScriptingConsolePanel::ExecuteScript(const std::string& script) {
        AddLog("> " + script);

        // Redirect Lua print() to this console
        ScriptEngine::SetPrintCallback([this](const std::string& s) { AddLog(s, false); });
        bool success = ScriptEngine::ExecuteString(script);

        if (!success) {
            // Error details are already logged by ScriptEngine::ExecuteString
            AddLog("Script execution failed. Check console for details.", true);
        }
        // Restore default (no capture) so game scripts don't flood the console
        ScriptEngine::SetPrintCallback(nullptr);
    }

    void ScriptingConsolePanel::AddLog(const std::string& log, bool isError) {
        LogEntry entry;
        entry.Text = log;
        entry.IsError = isError;
        
        m_Logs.push_back(entry);
        
        // Limit log size
        if (m_Logs.size() > MAX_LOGS) {
            m_Logs.erase(m_Logs.begin());
        }
    }

    void ScriptingConsolePanel::ClearLog() {
        m_Logs.clear();
        AddLog("Console cleared.");
    }

}
