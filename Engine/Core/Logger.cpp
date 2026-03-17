#include "Logger.hpp"
#include <ctime>

namespace GameEngine {

    std::ofstream Logger::s_LogFile;
    LogLevel Logger::s_MinLevel = LogLevel::Trace;
    bool Logger::s_ConsoleOutput = true;
    bool Logger::s_FileOutput = true;
    std::mutex Logger::s_Mutex;
    std::vector<Logger::LogCallback> Logger::s_Callbacks;

    void Logger::Init(const std::string& logFilePath, LogLevel minLevel) {
        s_MinLevel = minLevel;
        
        if (!logFilePath.empty()) {
            s_LogFile.open(logFilePath, std::ios::out | std::ios::app);
            if (!s_LogFile.is_open()) {
                std::cerr << "Failed to open log file: " << logFilePath << std::endl;
                s_FileOutput = false;
            }
        }
        
        GE_CORE_INFO("Logger initialized");
    }

    void Logger::Shutdown() {
        // Clear callbacks FIRST so no callback fires with stale references
        {
            std::lock_guard<std::mutex> lock(s_Mutex);
            s_Callbacks.clear();
        }
        if (s_LogFile.is_open()) {
            GE_CORE_INFO("Logger shutting down");
            s_LogFile.close();
        }
    }
    
    void Logger::AddCallback(LogCallback callback) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Callbacks.push_back(callback);
    }
    
    void Logger::ClearCallbacks() {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_Callbacks.clear();
    }
    
    void Logger::NotifyCallbacks(LogLevel level, const std::string& message, const std::string& timestamp,
                                 const std::vector<LogCallback>& callbacks) {
        LogEntry entry;
        entry.level = level;
        entry.message = message;
        entry.timestamp = timestamp;
        
        for (auto& callback : callbacks) {
            if (callback) {
                callback(entry);
            }
        }
    }

    void Logger::SetLevel(LogLevel level) {
        s_MinLevel = level;
    }

    LogLevel Logger::GetLevel() {
        return s_MinLevel;
    }

    void Logger::SetConsoleOutput(bool enabled) {
        s_ConsoleOutput = enabled;
    }

    void Logger::SetFileOutput(bool enabled) {
        s_FileOutput = enabled;
    }

    std::string Logger::GetTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm_result;
#ifdef _WIN32
        localtime_s(&tm_result, &time);
#else
        localtime_r(&time, &tm_result);
#endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_result, "%Y-%m-%d %H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return oss.str();
    }

    std::string Logger::GetLevelString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:    return "TRACE";
            case LogLevel::Debug:    return "DEBUG";
            case LogLevel::Info:     return "INFO";
            case LogLevel::Warning:  return "WARN";
            case LogLevel::Error:    return "ERROR";
            case LogLevel::Critical: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }

    std::string Logger::GetLevelColor(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:    return "\033[37m";  // White
            case LogLevel::Debug:    return "\033[36m";  // Cyan
            case LogLevel::Info:     return "\033[32m";  // Green
            case LogLevel::Warning:  return "\033[33m";  // Yellow
            case LogLevel::Error:    return "\033[31m";  // Red
            case LogLevel::Critical: return "\033[35m";  // Magenta
            default:                 return "\033[0m";   // Reset
        }
    }

    void Logger::WriteToConsole(LogLevel level, const std::string& message) {
        std::cout << GetLevelColor(level) 
                  << "[" << GetTimestamp() << "] "
                  << "[" << GetLevelString(level) << "] "
                  << message 
                  << "\033[0m" << std::endl;
    }

    void Logger::WriteToFile(LogLevel level, const std::string& message) {
        if (s_LogFile.is_open()) {
            s_LogFile << "[" << GetTimestamp() << "] "
                     << "[" << GetLevelString(level) << "] "
                     << message << std::endl;
            s_LogFile.flush();
        }
    }
}