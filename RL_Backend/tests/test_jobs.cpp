#include "rldemo/JobSystem.hpp"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <thread>

using namespace rldemo;

// Ensure clean state regardless of test order (Catch2 may shuffle)
static void ensure_clean_job_system() {
    JobSystem::Shutdown();
}

TEST_CASE("JobSystem initializes and shuts down", "[jobs]") {
    ensure_clean_job_system();
    JobSystem::Initialize(2);
    REQUIRE(JobSystem::GetWorkerCount() == 2);
    JobSystem::Shutdown();
}

TEST_CASE("Simple job execution", "[jobs]") {
    ensure_clean_job_system();
    JobSystem::Initialize(2);
    std::atomic<int> x{0};
    auto h = JobSystem::Enqueue([&x]() { x.store(42, std::memory_order_release); }, {});
    JobSystem::Wait(h);
    REQUIRE(x.load(std::memory_order_acquire) == 42);
    JobSystem::Shutdown();
}

TEST_CASE("Fan-out spawn", "[jobs]") {
    ensure_clean_job_system();
    JobSystem::Initialize(4);
    std::atomic<int> sum{0};
    JobHandle parent;
    // Spawn automatically increments parent counter for each child,
    // and decrements when the child completes
    parent.counter = new std::atomic<uint32_t>(0);
    for (int i = 0; i < 10; ++i) {
        // Don't manually increment - Spawn handles counter management
        JobSystem::Spawn([&sum, i]() { sum += i; }, parent);
    }
    while (parent.counter->load() > 0) std::this_thread::yield();
    REQUIRE(sum == 45);  // 0+1+2+...+9
    delete parent.counter;
    JobSystem::Shutdown();
}
