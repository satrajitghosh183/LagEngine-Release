#include "ConsolePanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace editor {

ConsolePanel* ConsolePanel::s_instance = nullptr;

ConsolePanel::ConsolePanel() 
    : EditorPanel("Console", true) {
    s_instance = this;
    
    // Add some demo log entries
    info("VerletX Engine Editor initialized");
    info("Vulkan Context created successfully");
    debug("Loading scene assets...");
    warning("Texture 'ground_diffuse.png' not found, using fallback");
    info("Physics world initialized with gravity: (0, -9.81, 0)");
    info("Cloth simulation created: 20x20 particles, 760 springs");
    debug("Compiling shaders...");
    info("All shaders compiled successfully");
    error("Failed to load mesh: 'missing_model.obj'");
    info("Scene loaded in 0.234 seconds");
}

void ConsolePanel::render() {
    if (!beginPanel(ImGuiWindowFlags_MenuBar)) {
        endPanel();
        return;
    }
    
    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            ImGui::MenuItem("Auto Scroll", nullptr, &m_autoScroll);
            ImGui::MenuItem("Collapse Duplicates", nullptr, &m_collapse);
            ImGui::Separator();
            if (ImGui::MenuItem("Clear")) {
                clear();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    renderToolbar();
    
    ImGui::Separator();
    
    renderLogList();
    
    endPanel();
}

void ConsolePanel::renderToolbar() {
    // Clear button
    if (ImGui::Button("Clear")) {
        clear();
    }
    
    ImGui::SameLine();
    
    // Collapse toggle
    ImGui::Checkbox("Collapse", &m_collapse);
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    
    // Filter buttons with counts
    ImVec4 infoColor = getLevelColor(LogLevel::Info);
    ImVec4 warnColor = getLevelColor(LogLevel::Warning);
    ImVec4 errColor = getLevelColor(LogLevel::Error);
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_showInfo ? ImVec4(infoColor.x * 0.5f, infoColor.y * 0.5f, infoColor.z * 0.5f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button(("\xef\x81\x9a " + std::to_string(m_infoCount) + "##Info").c_str())) {
        m_showInfo = !m_showInfo;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Info Messages");
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_showWarnings ? ImVec4(warnColor.x * 0.5f, warnColor.y * 0.5f, warnColor.z * 0.5f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button(("\xef\x81\xb1 " + std::to_string(m_warningCount) + "##Warn").c_str())) {
        m_showWarnings = !m_showWarnings;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Warnings");
    
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, m_showErrors ? ImVec4(errColor.x * 0.5f, errColor.y * 0.5f, errColor.z * 0.5f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button(("\xef\x81\x97 " + std::to_string(m_errorCount) + "##Err").c_str())) {
        m_showErrors = !m_showErrors;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle Errors");
    
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(20, 0));
    ImGui::SameLine();
    
    // Search bar
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##Search", "\xef\x80\x82 Search...", m_searchBuffer, sizeof(m_searchBuffer));
    ImGui::PopStyleVar();
}

void ConsolePanel::renderLogList() {
    std::string searchStr = m_searchBuffer;
    std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), ::tolower);
    
    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (const auto& entry : m_entries) {
        // Filter by level
        if (entry.level == LogLevel::Info && !m_showInfo) continue;
        if (entry.level == LogLevel::Warning && !m_showWarnings) continue;
        if (entry.level == LogLevel::Error && !m_showErrors) continue;
        if (entry.level == LogLevel::Debug && !m_showDebug) continue;
        
        // Filter by search
        if (!searchStr.empty()) {
            std::string msgLower = entry.message;
            std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
            if (msgLower.find(searchStr) == std::string::npos) continue;
        }
        
        ImVec4 color = getLevelColor(entry.level);
        const char* icon = getLevelIcon(entry.level);
        
        // Background color for errors and warnings
        if (entry.level == LogLevel::Error) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.4f, 0.1f, 0.1f, 0.3f));
        } else if (entry.level == LogLevel::Warning) {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.4f, 0.3f, 0.1f, 0.3f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        }
        
        ImGui::BeginGroup();
        
        // Timestamp
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("[%s]", entry.timestamp.c_str());
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        
        // Icon
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", icon);
        ImGui::PopStyleColor();
        
        ImGui::SameLine();
        
        // Message
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextWrapped("%s", entry.message.c_str());
        ImGui::PopStyleColor();
        
        // Count badge for collapsed entries
        if (m_collapse && entry.count > 1) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));
            ImGui::SmallButton(std::to_string(entry.count).c_str());
            ImGui::PopStyleColor();
        }
        
        ImGui::EndGroup();
        
        // Copy to clipboard on double-click
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            ImGui::SetClipboardText(entry.message.c_str());
        }
        
        ImGui::PopStyleColor();
        
        ImGui::Separator();
    }
    
    ImGui::PopStyleVar();
    
    // Auto-scroll
    if (m_scrollToBottom || (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
        ImGui::SetScrollHereY(1.0f);
        m_scrollToBottom = false;
    }
    
    ImGui::EndChild();
}

void ConsolePanel::addEntry(const std::string& message, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S");
    
    // Check for duplicate (for collapse)
    if (m_collapse && !m_entries.empty()) {
        auto& last = m_entries.back();
        if (last.message == message && last.level == level) {
            last.count++;
            last.timestamp = ss.str();
            return;
        }
    }
    
    // Add new entry
    LogEntry entry;
    entry.message = message;
    entry.timestamp = ss.str();
    entry.level = level;
    entry.count = 1;
    
    m_entries.push_back(entry);
    
    // Update counts
    switch (level) {
        case LogLevel::Info:
        case LogLevel::Debug:
            m_infoCount++;
            break;
        case LogLevel::Warning:
            m_warningCount++;
            break;
        case LogLevel::Error:
            m_errorCount++;
            break;
    }
    
    // Remove old entries if over limit
    while (m_entries.size() > MAX_ENTRIES) {
        auto& removed = m_entries.front();
        switch (removed.level) {
            case LogLevel::Info:
            case LogLevel::Debug:
                m_infoCount--;
                break;
            case LogLevel::Warning:
                m_warningCount--;
                break;
            case LogLevel::Error:
                m_errorCount--;
                break;
        }
        m_entries.pop_front();
    }
    
    if (m_autoScroll) {
        m_scrollToBottom = true;
    }
}

void ConsolePanel::log(const std::string& message, LogLevel level) {
    if (s_instance) {
        s_instance->addEntry(message, level);
    }
}

void ConsolePanel::info(const std::string& message) {
    log(message, LogLevel::Info);
}

void ConsolePanel::warning(const std::string& message) {
    log(message, LogLevel::Warning);
}

void ConsolePanel::error(const std::string& message) {
    log(message, LogLevel::Error);
}

void ConsolePanel::debug(const std::string& message) {
    log(message, LogLevel::Debug);
}

void ConsolePanel::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
    m_infoCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;
}

void ConsolePanel::scrollToBottom() {
    m_scrollToBottom = true;
}

ImVec4 ConsolePanel::getLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::Info:    return ImVec4(0.8f, 0.85f, 0.9f, 1.0f);
        case LogLevel::Warning: return ImVec4(0.95f, 0.8f, 0.25f, 1.0f);
        case LogLevel::Error:   return ImVec4(0.95f, 0.3f, 0.3f, 1.0f);
        case LogLevel::Debug:   return ImVec4(0.5f, 0.7f, 0.9f, 1.0f);
        default:                return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

const char* ConsolePanel::getLevelIcon(LogLevel level) const {
    switch (level) {
        case LogLevel::Info:    return "\xef\x81\x9a";  // info circle
        case LogLevel::Warning: return "\xef\x81\xb1";  // warning triangle
        case LogLevel::Error:   return "\xef\x81\x97";  // times circle
        case LogLevel::Debug:   return "\xef\x86\x88";  // bug
        default:                return "\xef\x81\x9a";
    }
}

const char* ConsolePanel::getLevelName(LogLevel level) const {
    switch (level) {
        case LogLevel::Info:    return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error:   return "Error";
        case LogLevel::Debug:   return "Debug";
        default:                return "Unknown";
    }
}

void ConsolePanel::update(float) {
    // Any per-frame updates
}

} // namespace editor

