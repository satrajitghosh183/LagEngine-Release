#pragma once

#include "Types.hpp"
#include <functional>
#include <atomic>
#include <thread>
#include <vector>
#include <array>
#include <mutex>
#include <cstdint>

namespace rldemo {

/**
 * JobHandle - tracks job completion for dependency DAG
 */
struct JobHandle {
    uint64_t id = 0;
    std::atomic<uint32_t>* counter = nullptr;  // decremented when job completes

    bool IsComplete() const;
    void Wait() const;
};

/**
 * JobSystem with Chase-Lev work-stealing deques
 *
 * - Per-worker Chase-Lev deque (lock-free push/pop from owner, steal from bottom)
 * - Global injector queue (MPMC, mutex) for external threads
 * - Job dependencies: JobHandle with atomic counter; spawn(child) increments parent
 * - Instrumentation: pushes, pops, steals, avg job time
 */
class JobSystem {
public:
    using JobFunc = std::function<void()>;

    static void Initialize(uint32_t workerCount = 0);
    static void Shutdown();

    /** Enqueue to global queue (for main/render thread) */
    static JobHandle Enqueue(JobFunc func, JobHandle dependency = {});

    /** Enqueue to current worker's local deque (fast path) */
    static void EnqueueLocal(JobFunc func);

    /** Spawn child job - parent handle's counter incremented; child completes -> decrement */
    static JobHandle Spawn(JobFunc func, JobHandle parent = {});

    static void Wait(JobHandle handle);
    static void WaitForAll();

    static uint32_t GetWorkerCount() { return s_WorkerCount; }

    // Instrumentation
    struct Stats {
        uint64_t pushes = 0;
        uint64_t pops = 0;
        uint64_t steals = 0;
        uint64_t stealAttempts = 0;
        uint64_t totalJobTimeNs = 0;
        uint64_t jobCount = 0;
    };
    static Stats GetStats();
    static void ResetStats();

    /** Per-worker instrumentation for RL (queue depth, job count, total time, idle time) */
    struct WorkerStat {
        size_t queueDepth = 0;
        uint64_t jobCount = 0;
        uint64_t totalTimeNs = 0;
        uint64_t idleNs = 0;
    };
    static std::vector<WorkerStat> GetWorkerStats();
    static size_t GetGlobalQueueDepth();

private:
    struct Job {
        JobFunc func;
        uint64_t id = 0;
        std::atomic<uint32_t>* completionCounter = nullptr;  // decrement when done
    };

    // Chase-Lev deque: lock-free, owner pushes/pops from bottom, stealers pop from top
    class WorkStealingDeque {
    public:
        static constexpr size_t kCapacity = 4096;
        void Push(Job job);
        bool TryPop(Job& out);   // owner pops from bottom
        bool TrySteal(Job& out); // stealer pops from top
        bool IsEmpty() const;
        size_t Size() const;

    private:
        std::atomic<size_t> m_Top{0};
        std::atomic<size_t> m_Bottom{0};
        Job m_Jobs[kCapacity];
    };

    // Global queue for injector
    class GlobalQueue {
    public:
        void Push(Job job);
        bool TryPop(Job& out);
        size_t Size() const;
    private:
        std::vector<Job> m_Jobs;
        mutable std::mutex m_Mutex;
    };

    static void WorkerThread(uint32_t workerId);
    static Job GetJob(uint32_t workerId);

    static std::atomic<uint64_t> s_NextJobId;
    static std::atomic<uint32_t> s_ActiveJobs;
    static uint32_t s_WorkerCount;
    static std::vector<std::unique_ptr<WorkStealingDeque>> s_Deques;
    static std::vector<std::thread> s_Workers;
    static GlobalQueue s_GlobalQueue;
    static std::atomic<bool> s_Shutdown;
    static thread_local uint32_t s_WorkerId;
    static bool s_Initialized;

    // Instrumentation
    static constexpr uint32_t kMaxWorkers = 64;
    static std::atomic<uint64_t> s_StatsPushes;
    static std::atomic<uint64_t> s_StatsPops;
    static std::atomic<uint64_t> s_StatsSteals;
    static std::atomic<uint64_t> s_StatsStealAttempts;
    static std::atomic<uint64_t> s_StatsTotalTimeNs;
    static std::atomic<uint64_t> s_StatsJobCount;
    static std::array<std::atomic<uint64_t>, kMaxWorkers> s_WorkerJobCount;
    static std::array<std::atomic<uint64_t>, kMaxWorkers> s_WorkerTotalTimeNs;
    static std::array<std::atomic<uint64_t>, kMaxWorkers> s_WorkerIdleNs;
};

} // namespace rldemo
