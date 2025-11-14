#pragma once

#include <string>
#include <memory>
#include <sstream>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <mutex>

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
     * @brief Thread-safe logging system with file and console output
     * 
     * Features:
     * - Multiple severity levels
     * - Colored console output
     * - File logging with rotation
     * - Thread-safe operations
     * - Timestamp support
     */
    class Logger {
    public:
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
        
    private:
        static std::string GetTimestamp();
        static std::string GetLevelString(LogLevel level);
        static std::string GetLevelColor(LogLevel level);
        static void WriteToConsole(LogLevel level, const std::string& message);
        static void WriteToFile(LogLevel level, const std::string& message);
        
        static std::ofstream s_LogFile;
        static LogLevel s_MinLevel;
        static bool s_ConsoleOutput;
        static bool s_FileOutput;
        static std::mutex s_Mutex;
    };
    
    // Template implementation
    template<typename... Args>
    void Logger::Log(LogLevel level, const std::string& format, Args&&... args) {
        if (level < s_MinLevel)
            return;
        
        std::lock_guard<std::mutex> lock(s_Mutex);
        
        // Format the message
        std::ostringstream oss;
        size_t pos = 0;
        size_t lastPos = 0;
        int argIndex = 0;
        
        // Simple format string replacement (replace {0}, {1}, etc.)
        auto formatArg = [&](auto&& arg) {
            std::string placeholder = "{" + std::to_string(argIndex++) + "}";
            while ((pos = format.find(placeholder, lastPos)) != std::string::npos) {
                oss << format.substr(lastPos, pos - lastPos) << arg;
                lastPos = pos + placeholder.length();
            }
        };
        
        (formatArg(std::forward<Args>(args)), ...);
        oss << format.substr(lastPos);
        
        std::string message = oss.str();
        
        if (s_ConsoleOutput)
            WriteToConsole(level, message);
        
        if (s_FileOutput)
            WriteToFile(level, message);
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