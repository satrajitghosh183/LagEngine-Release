#include "JobSystem.hpp"
#include "Logger.hpp"
#include <random>
#include <climits>
#include <unordered_set>

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
    
    // Initialization barrier members
    std::atomic<bool> JobSystem::s_WorkersReady(false);
    std::mutex JobSystem::s_InitMutex;
    std::condition_variable JobSystem::s_InitCV;

    // Job completion tracking
    std::unordered_set<uint64_t> JobSystem::s_CompletedJobs;
    std::deque<uint64_t> JobSystem::s_CompletedJobOrder;
    std::mutex JobSystem::s_CompletionMutex;
    std::condition_variable JobSystem::s_CompletionCV;
    std::mutex JobSystem::s_AllDoneMutex;
    std::condition_variable JobSystem::s_AllDoneCV;

    // JobHandle implementation
    bool JobHandle::IsComplete() const {
        if (id == 0) {
            return true;  // No job / invalid handle is considered complete
        }
        std::lock_guard<std::mutex> lock(JobSystem::s_CompletionMutex);
        return JobSystem::s_CompletedJobs.find(id) != JobSystem::s_CompletedJobs.end();
    }

    void JobSystem::MarkJobComplete(uint64_t jobId) {
        std::lock_guard<std::mutex> lock(s_CompletionMutex);
        s_CompletedJobs.insert(jobId);
        s_CompletedJobOrder.push_back(jobId);
        // Prune old entries to prevent unbounded growth (keep last 8192)
        const size_t maxCompleted = 8192;
        while (s_CompletedJobOrder.size() > maxCompleted) {
            uint64_t oldId = s_CompletedJobOrder.front();
            s_CompletedJobOrder.pop_front();
            s_CompletedJobs.erase(oldId);
        }
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
        s_WorkersReady.store(false);

        GE_CORE_INFO("Initializing JobSystem with {0} workers", workerCount);

        // Phase 1: Pre-allocate all WorkerState objects and reserve vector space
        s_Workers.reserve(workerCount);
        for (uint32_t i = 0; i < workerCount; ++i) {
            auto worker = std::make_unique<WorkerState>();
            worker->localQueue = std::make_unique<JobQueue>();
            worker->shouldStop.store(false);
            worker->workerId = i;
            s_Workers.push_back(std::move(worker));
        }

        // Phase 2: Start all worker threads (they will wait on the barrier)
        for (uint32_t i = 0; i < workerCount; ++i) {
            s_Workers[i]->thread = std::thread(WorkerThread, s_Workers[i].get());
        }

        // Phase 3: Signal all workers that initialization is complete
        {
            std::lock_guard<std::mutex> lock(s_InitMutex);
            s_WorkersReady.store(true);
        }
        s_InitCV.notify_all();

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
        s_WorkersReady.store(false);

        // Clear completed jobs tracking
        {
            std::lock_guard<std::mutex> lock(s_CompletionMutex);
            s_CompletedJobs.clear();
            s_CompletedJobOrder.clear();
        }

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
        std::unique_lock<std::mutex> lock(s_CompletionMutex);
        s_CompletionCV.wait(lock, [&handle] {
            return s_CompletedJobs.find(handle.id) != s_CompletedJobs.end();
        });
    }

    void JobSystem::WaitForAll() {
        if (!s_Initialized) return;
        std::unique_lock<std::mutex> lock(s_AllDoneMutex);
        s_AllDoneCV.wait(lock, [] {
            return s_ActiveJobs.load(std::memory_order_acquire) == 0;
        });
    }

    void JobSystem::WorkerThread(WorkerState* state) {
        s_LocalQueue = state->localQueue.get();
        s_WorkerId = state->workerId;

        // Wait for all workers to be initialized before starting work
        {
            std::unique_lock<std::mutex> lock(s_InitMutex);
            s_InitCV.wait(lock, [] { return s_WorkersReady.load(); });
        }

        while (!state->shouldStop.load(std::memory_order_acquire)) {
            Job job;

            // Try local queue first (fast path)
            if (state->localQueue->TryPop(job)) {
                // Check dependency
                if (job.dependency.id == 0 || job.dependency.IsComplete()) {
                    uint64_t jobId = job.id;
                    if (job.func) job.func();
                    MarkJobComplete(jobId);
                    uint32_t prev = s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                    if (prev == 1) {
                        std::lock_guard<std::mutex> lk(s_AllDoneMutex);
                        s_AllDoneCV.notify_all();
                    }
                } else {
                    // Dependency not ready, push back
                    state->localQueue->Push(std::move(job));
                }
                continue;
            }

            // Try global queue
            if (s_GlobalQueue.TryPop(job)) {
                if (job.dependency.id == 0 || job.dependency.IsComplete()) {
                    uint64_t jobId = job.id;
                    if (job.func) job.func();
                    MarkJobComplete(jobId);
                    uint32_t prev = s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                    if (prev == 1) {
                        std::lock_guard<std::mutex> lk(s_AllDoneMutex);
                        s_AllDoneCV.notify_all();
                    }
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
                        uint64_t jobId = job.id;
                        if (job.func) job.func();
                        MarkJobComplete(jobId);
                        uint32_t prev = s_ActiveJobs.fetch_sub(1, std::memory_order_release);
                        if (prev == 1) {
                            std::lock_guard<std::mutex> lk(s_AllDoneMutex);
                            s_AllDoneCV.notify_all();
                        }
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
