#include "rldemo/JobSystem.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <algorithm>

namespace rldemo {

// Static members
std::atomic<uint64_t> JobSystem::s_NextJobId(1);
std::atomic<uint32_t> JobSystem::s_ActiveJobs(0);
uint32_t JobSystem::s_WorkerCount = 0;
std::vector<std::unique_ptr<JobSystem::WorkStealingDeque>> JobSystem::s_Deques;
std::vector<std::thread> JobSystem::s_Workers;
JobSystem::GlobalQueue JobSystem::s_GlobalQueue;
std::atomic<bool> JobSystem::s_Shutdown(false);
thread_local uint32_t JobSystem::s_WorkerId = UINT32_MAX;
bool JobSystem::s_Initialized = false;

std::atomic<uint64_t> JobSystem::s_StatsPushes(0);
std::atomic<uint64_t> JobSystem::s_StatsPops(0);
std::atomic<uint64_t> JobSystem::s_StatsSteals(0);
std::atomic<uint64_t> JobSystem::s_StatsStealAttempts(0);
std::atomic<uint64_t> JobSystem::s_StatsTotalTimeNs(0);
std::atomic<uint64_t> JobSystem::s_StatsJobCount(0);
std::array<std::atomic<uint64_t>, JobSystem::kMaxWorkers> JobSystem::s_WorkerJobCount;
std::array<std::atomic<uint64_t>, JobSystem::kMaxWorkers> JobSystem::s_WorkerTotalTimeNs;
std::array<std::atomic<uint64_t>, JobSystem::kMaxWorkers> JobSystem::s_WorkerIdleNs;

// JobHandle
bool JobHandle::IsComplete() const {
    if (id == 0) return true;
    if (!counter) return true;
    return counter->load(std::memory_order_acquire) == 0;
}

void JobHandle::Wait() const {
    while (!IsComplete()) {
        std::this_thread::yield();
    }
}

// WorkStealingDeque - Chase-Lev
void JobSystem::WorkStealingDeque::Push(Job job) {
    size_t b = m_Bottom.load(std::memory_order_relaxed);
    m_Jobs[b % kCapacity] = std::move(job);
    std::atomic_thread_fence(std::memory_order_release);
    m_Bottom.store(b + 1, std::memory_order_relaxed);
}

bool JobSystem::WorkStealingDeque::TryPop(Job& out) {
    size_t b = m_Bottom.load(std::memory_order_relaxed);
    if (b == 0) return false;
    b--;
    m_Bottom.store(b, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    size_t t = m_Top.load(std::memory_order_relaxed);
    if (t <= b) {
        out = std::move(m_Jobs[b % kCapacity]);
        if (t == b) {
            if (!m_Top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
                return false;
        }
        return true;
    }
    m_Bottom.store(b + 1, std::memory_order_relaxed);
    return false;
}

bool JobSystem::WorkStealingDeque::TrySteal(Job& out) {
    size_t t = m_Top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    size_t b = m_Bottom.load(std::memory_order_acquire);
    if (t >= b) return false;
    out = std::move(m_Jobs[t % kCapacity]);
    if (!m_Top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
        return false;
    return true;
}

bool JobSystem::WorkStealingDeque::IsEmpty() const {
    size_t t = m_Top.load(std::memory_order_relaxed);
    size_t b = m_Bottom.load(std::memory_order_relaxed);
    return t >= b;
}

size_t JobSystem::WorkStealingDeque::Size() const {
    size_t t = m_Top.load(std::memory_order_relaxed);
    size_t b = m_Bottom.load(std::memory_order_relaxed);
    return (b >= t) ? (b - t) : 0;
}

// GlobalQueue
void JobSystem::GlobalQueue::Push(Job job) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Jobs.push_back(std::move(job));
}

bool JobSystem::GlobalQueue::TryPop(Job& out) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Jobs.empty()) return false;
    out = std::move(m_Jobs.back());
    m_Jobs.pop_back();
    return true;
}

size_t JobSystem::GlobalQueue::Size() const {
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Jobs.size();
}

// JobSystem
void JobSystem::Initialize(uint32_t workerCount) {
    if (s_Initialized) {
        spdlog::warn("JobSystem already initialized");
        return;
    }
    if (workerCount == 0) {
        workerCount = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
    }
    if (workerCount > JobSystem::kMaxWorkers) workerCount = JobSystem::kMaxWorkers;
    s_WorkerCount = workerCount;
    s_Shutdown.store(false);
    s_Deques.resize(workerCount);
    for (uint32_t i = 0; i < kMaxWorkers; ++i) {
        s_WorkerJobCount[i].store(0);
        s_WorkerTotalTimeNs[i].store(0);
        s_WorkerIdleNs[i].store(0);
    }
    for (uint32_t i = 0; i < workerCount; ++i) {
        s_Deques[i] = std::make_unique<WorkStealingDeque>();
    }
    spdlog::info("JobSystem initializing with {} workers", workerCount);
    for (uint32_t i = 0; i < workerCount; ++i) {
        s_Workers.emplace_back(WorkerThread, i);
    }
    s_Initialized = true;
}

void JobSystem::Shutdown() {
    if (!s_Initialized) return;
    s_Shutdown.store(true);
    for (auto& w : s_Workers) {
        if (w.joinable()) w.join();
    }
    s_Workers.clear();
    s_Deques.clear();
    s_WorkerCount = 0;
    s_Initialized = false;
    spdlog::info("JobSystem shutdown");
}

JobHandle JobSystem::Enqueue(JobFunc func, JobHandle dependency) {
    if (!s_Initialized) {
        if (func) func();
        return {};
    }
    
    // Create a completion counter for this job so Wait() can block until done
    // Counter is decremented by WorkerThread after job execution
    auto* counter = new std::atomic<uint32_t>(1);
    
    Job job;
    job.func = std::move(func);
    job.id = s_NextJobId.fetch_add(1, std::memory_order_relaxed);
    job.completionCounter = counter;  // WorkerThread will decrement this
    s_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
    s_StatsPushes.fetch_add(1, std::memory_order_relaxed);
    s_GlobalQueue.Push(std::move(job));
    
    JobHandle h;
    h.id = job.id;
    h.counter = counter;
    return h;
}

void JobSystem::EnqueueLocal(JobFunc func) {
    if (!s_Initialized) {
        if (func) func();
        return;
    }
    uint32_t wid = s_WorkerId;
    if (wid >= s_WorkerCount) {
        Enqueue(std::move(func), {});
        return;
    }
    Job job;
    job.func = std::move(func);
    job.id = s_NextJobId.fetch_add(1, std::memory_order_relaxed);
    job.completionCounter = nullptr;
    s_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
    s_StatsPushes.fetch_add(1, std::memory_order_relaxed);
    s_Deques[wid]->Push(std::move(job));
}

JobHandle JobSystem::Spawn(JobFunc func, JobHandle parent) {
    if (!s_Initialized) {
        if (func) func();
        return {};
    }
    std::atomic<uint32_t>* counter = nullptr;
    if (parent.counter) {
        counter = parent.counter;
        counter->fetch_add(1, std::memory_order_relaxed);
    } else {
        counter = new std::atomic<uint32_t>(1);
    }
    Job job;
    // Lambda decrements counter when job completes
    job.func = [func = std::move(func), counter]() {
        if (func) func();
        if (counter) counter->fetch_sub(1, std::memory_order_release);
    };
    job.id = s_NextJobId.fetch_add(1, std::memory_order_relaxed);
    // Don't set completionCounter - the lambda handles it to avoid double decrement
    job.completionCounter = nullptr;
    s_ActiveJobs.fetch_add(1, std::memory_order_relaxed);
    s_StatsPushes.fetch_add(1, std::memory_order_relaxed);
    uint32_t wid = s_WorkerId;
    if (wid < s_WorkerCount) {
        s_Deques[wid]->Push(std::move(job));
    } else {
        s_GlobalQueue.Push(std::move(job));
    }
    JobHandle h;
    h.id = job.id;
    h.counter = counter;
    return h;
}

void JobSystem::Wait(JobHandle handle) {
    handle.Wait();
}

void JobSystem::WaitForAll() {
    while (s_ActiveJobs.load(std::memory_order_acquire) > 0) {
        std::this_thread::yield();
    }
}

JobSystem::Job JobSystem::GetJob(uint32_t workerId) {
    Job job;
    if (s_Deques[workerId]->TryPop(job)) {
        s_StatsPops.fetch_add(1, std::memory_order_relaxed);
        return job;
    }
    if (s_GlobalQueue.TryPop(job)) {
        return job;
    }
    for (uint32_t i = 0; i < s_WorkerCount; ++i) {
        uint32_t victim = (workerId + i) % s_WorkerCount;
        if (victim == workerId) continue;
        s_StatsStealAttempts.fetch_add(1, std::memory_order_relaxed);
        if (s_Deques[victim]->TrySteal(job)) {
            s_StatsSteals.fetch_add(1, std::memory_order_relaxed);
            return job;
        }
    }
    return {};
}

void JobSystem::WorkerThread(uint32_t workerId) {
    s_WorkerId = workerId;
    auto lastIdleStart = std::chrono::high_resolution_clock::now();
    while (!s_Shutdown.load(std::memory_order_acquire)) {
        Job job = GetJob(workerId);
        if (!job.func) {
            std::this_thread::yield();
            continue;
        }
        auto now = std::chrono::high_resolution_clock::now();
        uint64_t idleNs = std::chrono::duration_cast<std::chrono::nanoseconds>(now - lastIdleStart).count();
        s_WorkerIdleNs[workerId].fetch_add(idleNs, std::memory_order_relaxed);
        auto start = now;
        job.func();
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t execNs = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        s_WorkerTotalTimeNs[workerId].fetch_add(execNs, std::memory_order_relaxed);
        s_WorkerJobCount[workerId].fetch_add(1, std::memory_order_relaxed);
        lastIdleStart = end;
        s_StatsTotalTimeNs.fetch_add(execNs, std::memory_order_relaxed);
        s_StatsJobCount.fetch_add(1, std::memory_order_relaxed);
        if (job.completionCounter) {
            job.completionCounter->fetch_sub(1, std::memory_order_release);
        }
        s_ActiveJobs.fetch_sub(1, std::memory_order_release);
    }
}

JobSystem::Stats JobSystem::GetStats() {
    Stats s;
    s.pushes = s_StatsPushes.load(std::memory_order_relaxed);
    s.pops = s_StatsPops.load(std::memory_order_relaxed);
    s.steals = s_StatsSteals.load(std::memory_order_relaxed);
    s.stealAttempts = s_StatsStealAttempts.load(std::memory_order_relaxed);
    s.totalJobTimeNs = s_StatsTotalTimeNs.load(std::memory_order_relaxed);
    s.jobCount = s_StatsJobCount.load(std::memory_order_relaxed);
    return s;
}

void JobSystem::ResetStats() {
    s_StatsPushes.store(0);
    s_StatsPops.store(0);
    s_StatsSteals.store(0);
    s_StatsStealAttempts.store(0);
    s_StatsTotalTimeNs.store(0);
    s_StatsJobCount.store(0);
    for (size_t i = 0; i < JobSystem::kMaxWorkers; ++i) {
        s_WorkerJobCount[i].store(0);
        s_WorkerTotalTimeNs[i].store(0);
        s_WorkerIdleNs[i].store(0);
    }
}

std::vector<JobSystem::WorkerStat> JobSystem::GetWorkerStats() {
    std::vector<WorkerStat> out;
    out.reserve(s_WorkerCount);
    for (uint32_t i = 0; i < s_WorkerCount; ++i) {
        WorkerStat ws;
        ws.queueDepth = s_Deques[i]->Size();
        ws.jobCount = s_WorkerJobCount[i].load(std::memory_order_relaxed);
        ws.totalTimeNs = s_WorkerTotalTimeNs[i].load(std::memory_order_relaxed);
        ws.idleNs = s_WorkerIdleNs[i].load(std::memory_order_relaxed);
        out.push_back(ws);
    }
    return out;
}

size_t JobSystem::GetGlobalQueueDepth() {
    return s_GlobalQueue.Size();
}

} // namespace rldemo
