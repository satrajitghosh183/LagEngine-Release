/**
 * @file main.cpp
 * @brief Main entry point for GameEngine unit tests
 */

#include <gtest/gtest.h>
#include "Core/Logger.hpp"

int main(int argc, char** argv) {
    // Initialize the logger for test output
    GameEngine::Logger::Init("test_output.log", GameEngine::LogLevel::Warning);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    // Cleanup
    GameEngine::Logger::Shutdown();
    
    // Cleanup test log file
    std::remove("test_output.log");
    
    return result;
}
