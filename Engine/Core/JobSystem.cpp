#include "JobSystem.hpp"
#include "Logger.hpp"
#include <random>
#include <climits>

namespace GameEngine {

    // Static members
    std::atomic<uint64_t> JobSystem::s_NextJobId(1);
    std::atomic<uint32_t> JobSystem::s_ActiveJobs(0);
    uint32_t JobSystem::s_WorkerCount = 0;
    std::vector<std::unique_ptr<JobSystem::WorkerState>> JobSystem::s_Workers;
    JobSystem::JobQueue JobSystem::s_GlobalQueue;
    thread_local JobSystem::JobQueue* JobSystem::s_LocalQueue = nullptr;
    thread_local uint32_t JobSystem::s_WorkerId = UINT32_MAX;
    bool JobSystem::s_Initialized = false;

    // JobHandle implementation
    bool JobHandle::IsComplete() const {
        // Simple implementation - assumes job is complete if id is 0
        // In production, would track completion via atomic flags
        return id == 0;
    }

    // JobQueue implementation
    void JobSystem::JobQueue::Push(Job job) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Jobs.push_back(std::move(job));
    }

    bool JobSystem::JobQueue::TryPop(Job& out) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (m_Jobs.empty()) {
            return false;
        }
        out = std::move(m_Jobs.front());
        m_Jobs.pop_front();
        return true;
    }

    bool JobSystem::JobQueue::IsEmpty() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Jobs.empty();
    }

    size_t JobSystem::JobQueue::Size() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Jobs.size();
    }

    // JobSystem implementation
    void JobSystem::Initialize(uint32_t workerCount) {
        if (s_Initialized) {
            GE_CORE_WARN("JobSystem already initialized");
            return;
        }

        if (workerCount == 0) {
            workerCount = std::thread::hardware_concurrency();
            if (workerCount == 0) {
                workerCount = 4; // Fallback
            }
        }

        s_WorkerCount = workerCount;

        GE_CORE_INFO("Initializing JobSystem with {0} workers", workerCount);

        for (uint32_t i = 0; i < workerCount; ++i) {
            auto worker = std::make_unique<WorkerState>();
            worker->localQueue = std::make_unique<JobQueue>();
            worker->shouldStop.store(false);
            worker->workerId = i;
            
            WorkerState* workerPtr = worker.get();
            s_Workers.push_back(std::move(worker));
            
            // Start thread after adding to vector
            s_Workers.back()->thread = std::thread(WorkerThread, workerPtr);
        }

        s_Initialized = true;
        GE_CORE_INFO("JobSystem initialized");
    }

    void JobSystem::Shutdown() {
        if (!s_Initialized) {
            return;
        }

        GE_CORE_INFO("Shutting down JobSystem...");

        // Signal all workers to stop
        for (auto& worker : s_Workers) {
            worker->shouldStop.store(true);
        }

        // Wait for all workers to finish
        for (auto& worker : s_Workers) {
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }

        s_Workers.clear();
        s_WorkerCount = 0;
        s_Initialized = false;

        GE_CORE_INFO("JobSystem shutdown");
    }

    JobHandle JobSystem::Enqueue(JobFunc func, JobHandle dependency) {
        if (!s_Initialized) {
            // Fallback to immediate execution
            if (func) func();
            return {};
        }

        Job job;
        job.func = std::move(func);
        job.id = s_NextJobId.fetch_add(1, std::memory_order_relaxed);
        job.dependency = dependency;

        JobHandle handle;
        handle.id = job.id;
        
        s_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
        s_GlobalQueue.Push(std::move(job));

        return handle;
    }

    void JobSystem::EnqueueLocal(JobFunc func) {
        if (!s_Initialized) {
            if (func) func();
            return;
        }

        auto& localQueue = GetLocalQueue();
        Job job;
        job.func = std::move(func);
        job.id = s_NextJobId.fetch_add(1, std::memory_order_relaxed);

        s_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
        localQueue.Push(std::move(job));
    }

    void JobSystem::Wait(JobHandle handle) {
        if (handle.id == 0) {
            return;
        }

        // Simple spin-wait (TODO: Implement proper waiting with condition variables)
        while (!handle.IsComplete()) {
            std::this_thread::yield();
        }
    }

    void JobSystem::WaitForAll() {
        if (!s_Initialized) {
            return;
        }

        // Wait until all jobs are complete
        while (s_ActiveJobs.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
    }

    void JobSystem::WorkerThread(WorkerState* state) {
        s_LocalQueue = state->localQueue.get();
        s_WorkerId = state->workerId;

        while (!state->shouldStop.load(std::memory_order_acquire)) {
            Job job;

            // Try local queue first (fast path)
            if (state->localQueue->TryPop(job)) {
                // Check dependency
                if (job.dependency.id == 0 || job.dependency.IsComplete()) {
                    if (job.func) job.func();
                    s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                } else {
                    // Dependency not ready, push back
                    state->localQueue->Push(std::move(job));
                }
                continue;
            }

            // Try global queue
            if (s_GlobalQueue.TryPop(job)) {
                if (job.dependency.id == 0 || job.dependency.IsComplete()) {
                    if (job.func) job.func();
                    s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                } else {
                    // Dependency not ready, push back
                    s_GlobalQueue.Push(std::move(job));
                }
                continue;
            }

            // Try stealing from other workers
            bool stole = false;
            for (uint32_t i = 0; i < s_WorkerCount; ++i) {
                uint32_t targetId = (state->workerId + i) % s_WorkerCount;
                if (targetId == state->workerId) {
                    continue;
                }

                auto& targetQueue = *s_Workers[targetId]->localQueue;
                if (targetQueue.TryPop(job)) {
                    if (job.dependency.id == 0 || job.dependency.IsComplete()) {
                        if (job.func) job.func();
                        s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                        stole = true;
                        break;
                    } else {
                        // Push back
                        targetQueue.Push(std::move(job));
                    }
                }
            }

            if (!stole) {
                // No work available, yield
                std::this_thread::yield();
            }
        }
    }

    JobSystem::JobQueue& JobSystem::GetLocalQueue() {
        if (s_LocalQueue == nullptr) {
            // Main thread - create a temporary queue
            static thread_local JobQueue mainThreadQueue;
            s_LocalQueue = &mainThreadQueue;
        }
        return *s_LocalQueue;
    }

}
