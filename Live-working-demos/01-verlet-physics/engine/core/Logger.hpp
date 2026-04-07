// ========== Logger.hpp ==========
#pragma once
#include <iostream>
#include <string>

namespace engine::core::log {
    enum class LogLevel {
        Info,
        Warning,
        Error
    };

    class Logger {
    public:
        static void log(const std::string& message, LogLevel level = LogLevel::Info);
    };
}