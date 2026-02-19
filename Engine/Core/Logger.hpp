#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <functional>
#include <vector>

namespace GameEngine {

    /**
     * @brief Log severity levels
     */
    enum class LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Critical = 5
    };

    /**
     * @brief Log entry structure for callbacks
     */
    struct LogEntry {
        LogLevel level;
        std::string message;
        std::string timestamp;
    };

    /**
     * @brief Thread-safe logging system with file and console output
     * 
     * Features:
     * - Multiple severity levels
     * - Colored console output
     * - File logging with rotation
     * - Thread-safe operations
     * - Timestamp support
     * - Callback support for UI integration
     */
    class Logger {
    public:
        using LogCallback = std::function<void(const LogEntry&)>;
        
        /**
         * @brief Initialize the logging system
         * @param logFilePath Path to log file (optional)
         * @param minLevel Minimum log level to output
         */
        static void Init(const std::string& logFilePath = "engine.log", 
                        LogLevel minLevel = LogLevel::Trace);
        
        /**
         * @brief Shutdown logging system
         */
        static void Shutdown();
        
        /**
         * @brief Log a message with formatting
         */
        template<typename... Args>
        static void Log(LogLevel level, const std::string& format, Args&&... args);
        
        /**
         * @brief Set minimum log level
         */
        static void SetLevel(LogLevel level);
        
        /**
         * @brief Get current log level
         */
        static LogLevel GetLevel();
        
        /**
         * @brief Enable/disable console output
         */
        static void SetConsoleOutput(bool enabled);
        
        /**
         * @brief Enable/disable file output
         */
        static void SetFileOutput(bool enabled);
        
        /**
         * @brief Add a log callback (for UI integration)
         */
        static void AddCallback(LogCallback callback);
        
        /**
         * @brief Clear all callbacks
         */
        static void ClearCallbacks();
        
    private:
        static std::string GetTimestamp();
        static std::string GetLevelString(LogLevel level);
        static std::string GetLevelColor(LogLevel level);
        static void WriteToConsole(LogLevel level, const std::string& message);
        static void WriteToFile(LogLevel level, const std::string& message);
        static void NotifyCallbacks(LogLevel level, const std::string& message, const std::string& timestamp,
                                    const std::vector<LogCallback>& callbacks);
        
        static std::ofstream s_LogFile;
        static LogLevel s_MinLevel;
        static bool s_ConsoleOutput;
        static bool s_FileOutput;
        static std::mutex s_Mutex;
        static std::vector<LogCallback> s_Callbacks;
    };
    
    // Template implementation
    template<typename... Args>
    void Logger::Log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < s_MinLevel)
            return;

        std::lock_guard<std::mutex> lock(s_Mutex);

        std::string message;
        if constexpr (sizeof...(args) == 0) {
            message = format;
        } else {
            // Build a flat array of stringified arguments
            std::ostringstream argStreams[sizeof...(args)];
            int idx = 0;
            ((argStreams[idx++] << std::forward<Args>(args)), ...);
            std::string argStrs[sizeof...(args)];
            for (size_t i = 0; i < sizeof...(args); i++)
                argStrs[i] = argStreams[i].str();

            std::ostringstream oss;
            size_t lastPos = 0;
            int autoIndex = 0;
            size_t len = format.size();

            for (size_t i = 0; i < len; i++) {
                if (format[i] == '{' && i + 1 < len) {
                    // Check for {} (auto-index)
                    if (format[i + 1] == '}') {
                        oss << format.substr(lastPos, i - lastPos);
                        if (autoIndex < static_cast<int>(sizeof...(args)))
                            oss << argStrs[autoIndex++];
                        lastPos = i + 2;
                        i++; // skip '}'
                    }
                    // Check for {N} (numbered index)
                    else if (format[i + 1] >= '0' && format[i + 1] <= '9') {
                        size_t close = format.find('}', i + 1);
                        if (close != std::string::npos) {
                            int n = std::stoi(format.substr(i + 1, close - i - 1));
                            oss << format.substr(lastPos, i - lastPos);
                            if (n < static_cast<int>(sizeof...(args)))
                                oss << argStrs[n];
                            lastPos = close + 1;
                            i = close; // skip to '}'
                        }
                    }
                    // Check for {:X} style (formatting hint - just use auto-index)
                    else if (format[i + 1] == ':') {
                        size_t close = format.find('}', i + 1);
                        if (close != std::string::npos) {
                            oss << format.substr(lastPos, i - lastPos);
                            if (autoIndex < static_cast<int>(sizeof...(args)))
                                oss << argStrs[autoIndex++];
                            lastPos = close + 1;
                            i = close;
                        }
                    }
                }
            }
            oss << format.substr(lastPos);
            message = oss.str();
        }

        std::string timestamp = GetTimestamp();

        if (s_ConsoleOutput)
            WriteToConsole(level, message);

        if (s_FileOutput)
            WriteToFile(level, message);

        // Copy callbacks before notifying (invoke outside lock to avoid deadlock if callback logs)
        std::vector<LogCallback> callbacksCopy = s_Callbacks;
        NotifyCallbacks(level, message, timestamp, callbacksCopy);
    }
}

// Convenient logging macros
#define GE_CORE_TRACE(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Trace, __VA_ARGS__)
#define GE_CORE_DEBUG(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Debug, __VA_ARGS__)
#define GE_CORE_INFO(...)     ::GameEngine::Logger::Log(::GameEngine::LogLevel::Info, __VA_ARGS__)
#define GE_CORE_WARN(...)     ::GameEngine::Logger::Log(::GameEngine::LogLevel::Warning, __VA_ARGS__)
#define GE_CORE_ERROR(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Error, __VA_ARGS__)
#define GE_CORE_CRITICAL(...) ::GameEngine::Logger::Log(::GameEngine::LogLevel::Critical, __VA_ARGS__)

#define GE_TRACE(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Trace, __VA_ARGS__)
#define GE_DEBUG(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Debug, __VA_ARGS__)
#define GE_INFO(...)     ::GameEngine::Logger::Log(::GameEngine::LogLevel::Info, __VA_ARGS__)
#define GE_WARN(...)     ::GameEngine::Logger::Log(::GameEngine::LogLevel::Warning, __VA_ARGS__)
#define GE_ERROR(...)    ::GameEngine::Logger::Log(::GameEngine::LogLevel::Error, __VA_ARGS__)
#define GE_CRITICAL(...) ::GameEngine::Logger::Log(::GameEngine::LogLevel::Critical, __VA_ARGS__)