/**
 * @file TimeTests.cpp
 * @brief Unit tests for Time utilities
 */

#include <gtest/gtest.h>
#include "Core/Time.hpp"
#include <thread>
#include <chrono>

using namespace GameEngine;

// ==================== Time Class Tests ====================

TEST(TimeTest, GetTime) {
    float time1 = Time::GetTime();
    
    // Sleep a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    float time2 = Time::GetTime();
    
    // Time should have advanced
    EXPECT_GT(time2, time1);
}

TEST(TimeTest, DeltaTime) {
    // Delta time should be positive
    float dt = Time::GetDeltaTime();
    EXPECT_GE(dt, 0.0f);
}

TEST(TimeTest, FixedDeltaTime) {
    float fixedDt = Time::GetFixedTimestep();
    EXPECT_GT(fixedDt, 0.0f);
    
    // Fixed delta time is typically 1/60 or similar
    EXPECT_LE(fixedDt, 0.1f);  // Should be reasonable
}

TEST(TimeTest, TimeScale) {
    float originalScale = Time::GetTimeScale();
    
    Time::SetTimeScale(0.5f);
    EXPECT_FLOAT_EQ(Time::GetTimeScale(), 0.5f);
    
    Time::SetTimeScale(2.0f);
    EXPECT_FLOAT_EQ(Time::GetTimeScale(), 2.0f);
    
    // Restore
    Time::SetTimeScale(originalScale);
}

TEST(TimeTest, FrameCount) {
    uint64_t frame1 = Time::GetFrameCount();
    
    // Frame count should be non-negative
    EXPECT_GE(frame1, 0u);
}

// ==================== Timer Utility Tests ====================

TEST(TimerTest, BasicTimer) {
    auto start = std::chrono::high_resolution_clock::now();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_GE(duration.count(), 50);
    EXPECT_LE(duration.count(), 100);  // Allow some overhead
}

TEST(TimerTest, MicrosecondPrecision) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Busy wait for ~1ms
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) {
        x += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    EXPECT_GT(duration.count(), 0);
}
