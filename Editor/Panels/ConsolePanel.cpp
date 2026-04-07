#include "ConsolePanel.hpp"
#include <imgui.h>
#include <cstring>
#include <algorithm>

namespace GameEngine {

    ConsolePanel::ConsolePanel() {
        // Register callback with logger
        Logger::AddCallback([this](const LogEntry& entry) {
            OnLogReceived(entry);
        });
    }

    ConsolePanel::~ConsolePanel() {
        // Note: We don't remove the callback here because Logger uses a vector
        // and doesn't support removing individual callbacks. The callback will
        // be cleared when Logger::Shutdown() is called.
    }

    void ConsolePanel::OnLogReceived(const LogEntry& entry) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        DisplayEntry displayEntry;
        displayEntry.level = entry.level;
        displayEntry.message = entry.message;
        displayEntry.timestamp = entry.timestamp;
        
        m_Entries.push_back(displayEntry);
        
        // Limit the number of entries to prevent memory issues
        if (m_Entries.size() > MAX_ENTRIES) {
            m_Entries.erase(m_Entries.begin(), m_Entries.begin() + (m_Entries.size() - MAX_ENTRIES));
        }
        
        if (m_AutoScroll) {
            m_ScrollToBottom = true;
        }
    }

    void ConsolePanel::Clear() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Entries.clear();
    }

    void ConsolePanel::AddLog(LogLevel level, const std::string& message) {
        LogEntry entry;
        entry.level = level;
        entry.message = message;
        entry.timestamp = "";
        OnLogReceived(entry);
    }

    void ConsolePanel::OnImGuiRender() {
        ImGui::Begin("Console");
        
        // Toolbar
        if (ImGui::Button("Clear")) {
            Clear();
        }
        
        ImGui::SameLine();
        ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        
        // Level filters
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Trace));
        ImGui::Checkbox("T", &m_ShowTrace);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Trace");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Debug));
        ImGui::Checkbox("D", &m_ShowDebug);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Debug");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Info));
        ImGui::Checkbox("I", &m_ShowInfo);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Info");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Warning));
        ImGui::Checkbox("W", &m_ShowWarning);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Warning");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Error));
        ImGui::Checkbox("E", &m_ShowError);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Error");
        
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, GetColorForLevel(LogLevel::Critical));
        ImGui::Checkbox("C", &m_ShowCritical);
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Critical");
        
        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();
        
        // Search filter
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##Filter", m_FilterText, sizeof(m_FilterText));
        ImGui::SameLine();
        if (ImGui::Button("X##ClearFilter")) {
            m_FilterText[0] = '\0';
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear filter");
        
        ImGui::Separator();
        
        // Log entries
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            
            int entryIndex = 0;
            for (const auto& entry : m_Entries) {
                // Check level filter
                bool showLevel = false;
                switch (entry.level) {
                    case LogLevel::Trace:    showLevel = m_ShowTrace; break;
                    case LogLevel::Debug:    showLevel = m_ShowDebug; break;
                    case LogLevel::Info:     showLevel = m_ShowInfo; break;
                    case LogLevel::Warning:  showLevel = m_ShowWarning; break;
                    case LogLevel::Error:    showLevel = m_ShowError; break;
                    case LogLevel::Critical: showLevel = m_ShowCritical; break;
                }

                if (!showLevel) continue;

                // Check text filter
                if (m_FilterText[0] != '\0') {
                    if (entry.message.find(m_FilterText) == std::string::npos) {
                        continue;
                    }
                }

                // Display entry
                ImVec4 color = GetColorForLevel(entry.level);

                ImGui::PushID(entryIndex++);
                ImGui::PushStyleColor(ImGuiCol_Text, color);

                // Format: [TIME] [LEVEL] Message
                if (!entry.timestamp.empty()) {
                    ImGui::TextUnformatted(("[" + entry.timestamp + "] ").c_str());
                    ImGui::SameLine(0, 0);
                }

                ImGui::Text("%s ", GetLevelIcon(entry.level));
                ImGui::SameLine(0, 0);

                ImGui::Selectable(entry.message.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);

                ImGui::PopStyleColor();

                // Context menu for copying
                if (ImGui::BeginPopupContextItem()) {
                    if (ImGui::MenuItem("Copy")) {
                        ImGui::SetClipboardText(entry.message.c_str());
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
        }
        
        if (m_ScrollToBottom) {
            ImGui::SetScrollHereY(1.0f);
            m_ScrollToBottom = false;
        }
        
        ImGui::EndChild();
        
        ImGui::End();
    }

    ImVec4 ConsolePanel::GetColorForLevel(LogLevel level) const {
        switch (level) {
            case LogLevel::Trace:    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // Gray
            case LogLevel::Debug:    return ImVec4(0.4f, 0.8f, 0.8f, 1.0f);  // Cyan
            case LogLevel::Info:     return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);  // Green
            case LogLevel::Warning:  return ImVec4(0.9f, 0.9f, 0.2f, 1.0f);  // Yellow
            case LogLevel::Error:    return ImVec4(0.9f, 0.3f, 0.3f, 1.0f);  // Red
            case LogLevel::Critical: return ImVec4(0.9f, 0.2f, 0.9f, 1.0f);  // Magenta
            default:                 return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
        }
    }

    const char* ConsolePanel::GetLevelIcon(LogLevel level) const {
        switch (level) {
            case LogLevel::Trace:    return "[T]";
            case LogLevel::Debug:    return "[D]";
            case LogLevel::Info:     return "[I]";
            case LogLevel::Warning:  return "[W]";
            case LogLevel::Error:    return "[E]";
            case LogLevel::Critical: return "[!]";
            default:                 return "[?]";
        }
    }

}
