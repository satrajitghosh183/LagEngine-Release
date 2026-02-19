/**
 * @file LoggerTests.cpp
 * @brief Unit tests for the Logger system
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Core/Logger.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace GameEngine;

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::Init("test.log", LogLevel::Trace);
    }

    void TearDown() override {
        Logger::Shutdown();
        // Clean up test log file
        std::remove("test.log");
    }
};

// Test basic logging at all levels
TEST_F(LoggerTest, LogAllLevels) {
    EXPECT_NO_THROW(GE_CORE_TRACE("Trace message"));
    EXPECT_NO_THROW(GE_CORE_DEBUG("Debug message"));
    EXPECT_NO_THROW(GE_CORE_INFO("Info message"));
    EXPECT_NO_THROW(GE_CORE_WARN("Warning message"));
    EXPECT_NO_THROW(GE_CORE_ERROR("Error message"));
    EXPECT_NO_THROW(GE_CORE_CRITICAL("Critical message"));
}

// Test format string with arguments
TEST_F(LoggerTest, FormatStrings) {
    EXPECT_NO_THROW(GE_CORE_INFO("Value: {}", 42));
    EXPECT_NO_THROW(GE_CORE_INFO("Two values: {} and {}", 1, 2));
    EXPECT_NO_THROW(GE_CORE_INFO("Named: {0}, {1}, {0}", "first", "second"));
    EXPECT_NO_THROW(GE_CORE_INFO("Float: {}", 3.14f));
    EXPECT_NO_THROW(GE_CORE_INFO("String: {}", std::string("hello")));
}

// Test log level filtering
TEST_F(LoggerTest, LevelFiltering) {
    Logger::SetLevel(LogLevel::Warning);
    
    std::atomic<int> callCount{0};
    Logger::AddCallback([&callCount](const LogEntry& entry) {
        callCount++;
    });
    
    GE_CORE_TRACE("Should not appear");
    GE_CORE_DEBUG("Should not appear");
    GE_CORE_INFO("Should not appear");
    GE_CORE_WARN("Should appear");
    GE_CORE_ERROR("Should appear");
    
    // Give time for callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    EXPECT_EQ(callCount.load(), 2);
}

// Test callback system
TEST_F(LoggerTest, CallbackSystem) {
    std::vector<LogEntry> capturedLogs;
    std::mutex logMutex;
    
    Logger::AddCallback([&](const LogEntry& entry) {
        std::lock_guard<std::mutex> lock(logMutex);
        capturedLogs.push_back(entry);
    });
    
    GE_CORE_INFO("Test message");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    std::lock_guard<std::mutex> lock(logMutex);
    ASSERT_EQ(capturedLogs.size(), 1);
    EXPECT_EQ(capturedLogs[0].level, LogLevel::Info);
    EXPECT_THAT(capturedLogs[0].message, ::testing::HasSubstr("Test message"));
}

// Test thread safety - concurrent logging from multiple threads
TEST_F(LoggerTest, ThreadSafety) {
    const int numThreads = 10;
    const int logsPerThread = 100;
    std::atomic<int> totalLogs{0};
    
    Logger::AddCallback([&totalLogs](const LogEntry& entry) {
        totalLogs++;
    });
    
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back([i, logsPerThread]() {
            for (int j = 0; j < logsPerThread; j++) {
                GE_CORE_INFO("Thread {} log {}", i, j);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Give time for all callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(totalLogs.load(), numThreads * logsPerThread);
}

// Test callback clearing
TEST_F(LoggerTest, ClearCallbacks) {
    std::atomic<int> callCount{0};
    
    Logger::AddCallback([&callCount](const LogEntry& entry) {
        callCount++;
    });
    
    GE_CORE_INFO("First");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(callCount.load(), 1);
    
    Logger::ClearCallbacks();
    
    GE_CORE_INFO("Second");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(callCount.load(), 1); // Should still be 1
}

// Test console/file output toggling
TEST_F(LoggerTest, OutputToggling) {
    EXPECT_NO_THROW(Logger::SetConsoleOutput(false));
    EXPECT_NO_THROW(GE_CORE_INFO("No console output"));
    EXPECT_NO_THROW(Logger::SetConsoleOutput(true));
    
    EXPECT_NO_THROW(Logger::SetFileOutput(false));
    EXPECT_NO_THROW(GE_CORE_INFO("No file output"));
    EXPECT_NO_THROW(Logger::SetFileOutput(true));
}
