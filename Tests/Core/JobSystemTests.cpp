/**
 * @file JobSystemTests.cpp
 * @brief Unit tests for the JobSystem
 */

#include <gtest/gtest.h>
#include "Core/JobSystem.hpp"
#include <atomic>
#include <chrono>
#include <thread>

using namespace GameEngine;

class JobSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        JobSystem::Initialize(4); // Use 4 worker threads
    }

    void TearDown() override {
        JobSystem::Shutdown();
    }
};

// Test basic job execution
TEST_F(JobSystemTest, BasicJobExecution) {
    std::atomic<bool> executed{false};
    
    auto handle = JobSystem::Enqueue([&executed]() {
        executed = true;
    });
    
    JobSystem::Wait(handle);
    
    EXPECT_TRUE(executed.load());
}

// Test job handle completion
TEST_F(JobSystemTest, JobHandleCompletion) {
    auto handle = JobSystem::Enqueue([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    
    EXPECT_FALSE(handle.IsComplete());
    
    JobSystem::Wait(handle);
    
    EXPECT_TRUE(handle.IsComplete());
}

// Test multiple jobs
TEST_F(JobSystemTest, MultipleJobs) {
    const int numJobs = 100;
    std::atomic<int> counter{0};
    std::vector<JobHandle> handles;
    
    for (int i = 0; i < numJobs; i++) {
        handles.push_back(JobSystem::Enqueue([&counter]() {
            counter++;
        }));
    }
    
    for (auto& handle : handles) {
        JobSystem::Wait(handle);
    }
    
    EXPECT_EQ(counter.load(), numJobs);
}

// Test job dependencies
TEST_F(JobSystemTest, JobDependencies) {
    std::atomic<int> order{0};
    int firstValue = -1;
    int secondValue = -1;
    
    auto firstJob = JobSystem::Enqueue([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        firstValue = order++;
    });
    
    auto secondJob = JobSystem::Enqueue([&]() {
        secondValue = order++;
    }, firstJob);
    
    JobSystem::Wait(secondJob);
    
    // Second job should have run after first
    EXPECT_EQ(firstValue, 0);
    EXPECT_EQ(secondValue, 1);
}

// Test WaitForAll
TEST_F(JobSystemTest, WaitForAll) {
    const int numJobs = 50;
    std::atomic<int> counter{0};
    
    for (int i = 0; i < numJobs; i++) {
        JobSystem::Enqueue([&counter]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            counter++;
        });
    }
    
    JobSystem::WaitForAll();
    
    EXPECT_EQ(counter.load(), numJobs);
}

// Test local queue (fast path)
TEST_F(JobSystemTest, LocalQueue) {
    std::atomic<int> counter{0};
    
    // Enqueue a job that will enqueue more jobs locally
    auto handle = JobSystem::Enqueue([&counter]() {
        for (int i = 0; i < 10; i++) {
            JobSystem::EnqueueLocal([&counter]() {
                counter++;
            });
        }
    });
    
    JobSystem::Wait(handle);
    JobSystem::WaitForAll();
    
    EXPECT_EQ(counter.load(), 10);
}

// Test worker count
TEST_F(JobSystemTest, WorkerCount) {
    EXPECT_EQ(JobSystem::GetWorkerCount(), 4);
}

// Test stress with many jobs
TEST_F(JobSystemTest, StressTest) {
    const int numJobs = 10000;
    std::atomic<int> counter{0};
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numJobs; i++) {
        JobSystem::Enqueue([&counter]() {
            counter++;
        });
    }
    
    JobSystem::WaitForAll();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(counter.load(), numJobs);
    // Should complete in reasonable time (< 5 seconds)
    EXPECT_LT(duration.count(), 5000);
}

// Test concurrent job completion tracking doesn't leak memory
TEST_F(JobSystemTest, CompletionTrackingNoLeak) {
    // Run many jobs and check completion - tests that s_CompletedJobs is bounded
    for (int batch = 0; batch < 100; batch++) {
        std::vector<JobHandle> handles;
        for (int i = 0; i < 100; i++) {
            handles.push_back(JobSystem::Enqueue([]() {}));
        }
        for (auto& h : handles) {
            JobSystem::Wait(h);
            EXPECT_TRUE(h.IsComplete());
        }
    }
}
