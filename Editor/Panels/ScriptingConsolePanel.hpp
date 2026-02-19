#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Scripting/ScriptEngine.hpp"
#include <string>
#include <vector>
#include <deque>

namespace GameEngine {

    /**
     * @brief Scripting console panel for Lua script execution
     * 
     * Features:
     * - Command history with scrollback
     * - Multi-line script input
     * - Error display
     * - Print output capture
     * - HOME key toggle
     */
    class ScriptingConsolePanel {
    public:
        ScriptingConsolePanel();
        ~ScriptingConsolePanel() = default;

        /**
         * @brief Render the panel UI
         */
        void OnImGuiRender();

        /**
         * @brief Toggle panel visibility
         */
        void Toggle() { m_IsOpen = !m_IsOpen; }

        /**
         * @brief Set panel visibility
         */
        void SetOpen(bool open) { m_IsOpen = open; }

        /**
         * @brief Check if panel is open
         */
        bool IsOpen() const { return m_IsOpen; }

    private:
        void ExecuteScript(const std::string& script);
        void AddLog(const std::string& log, bool isError = false);
        void ClearLog();

    private:
        bool m_IsOpen = false;
        
        // Input buffer
        char m_InputBuffer[4096] = "";
        std::string m_CurrentInput;
        
        // Command history
        std::deque<std::string> m_History;
        int m_HistoryIndex = -1;
        static constexpr size_t MAX_HISTORY = 100;
        
        // Log/scrollback
        struct LogEntry {
            std::string Text;
            bool IsError;
        };
        std::vector<LogEntry> m_Logs;
        bool m_AutoScroll = true;
        static constexpr size_t MAX_LOGS = 1000;
        
        // Lua print capture
        std::string m_CapturedPrint;
    };

}
