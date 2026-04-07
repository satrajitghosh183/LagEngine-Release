#include "core/Logger.hpp"

void engine::core::log::Logger::log(const std::string& message, LogLevel level) {
    switch (level) {
        case LogLevel::Info:    std::cout << "[INFO]    "; break;
        case LogLevel::Warning: std::cout << "[WARNING] "; break;
        case LogLevel::Error:   std::cout << "[ERROR]   "; break;
    }
    std::cout << message << std::endl;
}
