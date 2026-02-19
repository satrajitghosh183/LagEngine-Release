#pragma once

#include "EditorPanel.hpp"
#include <vector>
#include <string>
#include <deque>
#include <mutex>

namespace editor {

/**
 * @brief Console Panel - shows log messages, warnings, and errors
 * Similar to Unity's Console or LumixEngine's Log
 */
class ConsolePanel : public EditorPanel {
public:
    ConsolePanel();
    
    void render() override;
    void update(float dt) override;
    
    // Log levels
    enum class LogLevel {
        Info,
        Warning,
        Error,
        Debug
    };
    
    // Add log messages
    static void log(const std::string& message, LogLevel level = LogLevel::Info);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);
    static void debug(const std::string& message);
    
    // Console control
    void clear();
    void scrollToBottom();
    
    // Filters
    void setShowInfo(bool show) { m_showInfo = show; }
    void setShowWarnings(bool show) { m_showWarnings = show; }
    void setShowErrors(bool show) { m_showErrors = show; }
    void setShowDebug(bool show) { m_showDebug = show; }

private:
    struct LogEntry {
        std::string message;
        std::string timestamp;
        LogLevel level;
        int count = 1;  // For collapsing duplicate messages
    };
    
    void renderToolbar();
    void renderLogList();
    void addEntry(const std::string& message, LogLevel level);
    ImVec4 getLevelColor(LogLevel level) const;
    const char* getLevelIcon(LogLevel level) const;
    const char* getLevelName(LogLevel level) const;
    
    static ConsolePanel* s_instance;
    
    std::deque<LogEntry> m_entries;
    std::mutex m_mutex;
    
    static constexpr size_t MAX_ENTRIES = 1000;
    
    // Filters
    bool m_showInfo = true;
    bool m_showWarnings = true;
    bool m_showErrors = true;
    bool m_showDebug = true;
    bool m_collapse = true;  // Collapse duplicate messages
    
    // Search
    char m_searchBuffer[256] = "";
    
    // UI state
    bool m_autoScroll = true;
    bool m_scrollToBottom = false;
    
    // Counts
    int m_infoCount = 0;
    int m_warningCount = 0;
    int m_errorCount = 0;
};

} // namespace editor

