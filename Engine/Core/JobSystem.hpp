#pragma once

#include "Base.hpp"
#include <functional>
#include <atomic>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstdint>
#include <unordered_set>

namespace GameEngine {

    /**
     * @brief Job handle for tracking job completion
     */
    struct JobHandle {
        uint64_t id = 0;
        bool IsComplete() const;
    };

    /**
     * @brief Job system with work-stealing
     * 
     * Features:
     * - Per-thread local queues
     * - Global injector queue (MPMC, coarse mutex)
     * - Work-stealing with exponential backoff
     * - Dependency DAG resolution
     */
    class JobSystem {
    public:
        using JobFunc = std::function<void()>;

        /**
         * @brief Initialize job system
         */
        static void Initialize(uint32_t workerCount = 0);

        /**
         * @brief Shutdown job system
         */
        static void Shutdown();

        /**
         * @brief Enqueue job to global queue
         */
        static JobHandle Enqueue(JobFunc func, JobHandle dependency = {});

        /**
         * @brief Enqueue job to current thread's local queue (fast path)
         */
        static void EnqueueLocal(JobFunc func);

        /**
         * @brief Wait for job to complete
         */
        static void Wait(JobHandle handle);

        /**
         * @brief Wait for all jobs to complete
         */
        static void WaitForAll();

        /**
         * @brief Get worker count
         */
        static uint32_t GetWorkerCount() { return s_WorkerCount; }

    private:
        struct Job {
            JobFunc func;
            uint64_t id = 0;
            JobHandle dependency;
        };

        // Thread-safe job queue
        class JobQueue {
        public:
            void Push(Job job);
            bool TryPop(Job& out);
            bool IsEmpty() const;
            size_t Size() const;

        private:
            std::deque<Job> m_Jobs;
            mutable std::mutex m_Mutex;
        };

        // Worker thread state
        struct WorkerState {
            std::unique_ptr<JobQueue> localQueue;
            std::atomic<bool> shouldStop{false};
            std::thread thread;
            uint32_t workerId{0};
        };

        static void WorkerThread(WorkerState* state);
        static JobQueue& GetLocalQueue();
        static void MarkJobComplete(uint64_t jobId);

        static std::atomic<uint64_t> s_NextJobId;
        static std::atomic<uint32_t> s_ActiveJobs;
        static uint32_t s_WorkerCount;
        static std::vector<std::unique_ptr<WorkerState>> s_Workers;
        static JobQueue s_GlobalQueue;
        static thread_local JobQueue* s_LocalQueue;
        static thread_local uint32_t s_WorkerId;
        static bool s_Initialized;

        // Initialization barrier - workers wait until all are ready
        static std::atomic<bool> s_WorkersReady;
        static std::mutex s_InitMutex;
        static std::condition_variable s_InitCV;

        // Job completion tracking
        static std::unordered_set<uint64_t> s_CompletedJobs;
        static std::deque<uint64_t> s_CompletedJobOrder;
        static std::mutex s_CompletionMutex;
        static std::condition_variable s_CompletionCV;

        static std::mutex s_AllDoneMutex;
        static std::condition_variable s_AllDoneCV;

        friend struct JobHandle;
    };

}
