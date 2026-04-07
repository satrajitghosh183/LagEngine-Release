#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <vector>
#include <string>
#include <mutex>

namespace GameEngine {

    /**
     * @brief Console panel - displays engine logs
     * 
     * Features:
     * - Display logs with color-coding by level
     * - Filter by log level
     * - Search/filter text
     * - Clear button
     * - Auto-scroll
     * - Copy to clipboard
     */
    class ConsolePanel {
    public:
        ConsolePanel();
        ~ConsolePanel();
        
        void OnImGuiRender();
        
        /**
         * @brief Clear all log entries
         */
        void Clear();
        
        /**
         * @brief Add a log entry manually (for testing)
         */
        void AddLog(LogLevel level, const std::string& message);
        
    private:
        void OnLogReceived(const LogEntry& entry);
        ImVec4 GetColorForLevel(LogLevel level) const;
        const char* GetLevelIcon(LogLevel level) const;
        
    private:
        struct DisplayEntry {
            LogLevel level;
            std::string message;
            std::string timestamp;
        };
        
        std::vector<DisplayEntry> m_Entries;
        std::mutex m_Mutex;
        
        // Filter settings
        bool m_ShowTrace = true;
        bool m_ShowDebug = true;
        bool m_ShowInfo = true;
        bool m_ShowWarning = true;
        bool m_ShowError = true;
        bool m_ShowCritical = true;
        
        char m_FilterText[256] = "";
        bool m_AutoScroll = true;
        bool m_ScrollToBottom = false;
        
        static constexpr size_t MAX_ENTRIES = 1000;
    };

}
