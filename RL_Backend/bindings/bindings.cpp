/**
 * pybind11 bindings for RL_Backend environments
 * FrameSchedulerEnv: workload matching demo_indirectdraw
 * WorkStealSchedulerEnv: N-body physics on JobSystem, 75 obs dims, scheduling actions
 */

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include "rldemo/JobSystem.hpp"
#include "rldemo/NBodySim.hpp"
#include <chrono>
#include <random>
#include <cmath>
#include <mutex>
#include <memory>
#include <vector>
#include <algorithm>

namespace py = pybind11;

static constexpr int kMaxWorkersForObs = 16;

struct InstanceStaging {
    std::vector<float> data;
    std::mutex mutex;
    static constexpr int kFloatsPerInstance = 20;
};

/**
 * FrameSchedulerEnv - Legacy environment (kept for compatibility)
 */
class FrameSchedulerEnv {
public:
    static constexpr int kObsDim = 8;
    static constexpr int kActDim = 4;

    FrameSchedulerEnv() : m_Rng(0), m_Initialized(false) {}

    py::array_t<float> reset() {
        m_StepCount = 0;
        m_EWMAJobTimeNs = 1e6f;
        m_EWMAFrameTimeMs = 16.67f;

        if (!m_Initialized) {
            uint32_t workers = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
            rldemo::JobSystem::Initialize(workers);
            m_WorkerCount = rldemo::JobSystem::GetWorkerCount();
            m_Initialized = true;
        }
        rldemo::JobSystem::ResetStats();
        runDemoWorkload(10000);
        return getObs();
    }

    py::object step(py::array_t<float> action) {
        if (action.size() != kActDim) {
            throw std::runtime_error("Action must have 4 elements");
        }
        auto a = action.unchecked<1>();
        int cubeCount = 100 + static_cast<int>(std::max(0.f, std::min(1.f, a(0))) * 99900);
        float camAngle = std::max(0.f, std::min(1.f, a(1))) * 6.28318f;

        rldemo::JobSystem::ResetStats();
        float frameTimeMs = runDemoWorkload(cubeCount, camAngle);

        auto stats = rldemo::JobSystem::GetStats();
        m_EWMAJobTimeNs = 0.9f * m_EWMAJobTimeNs + 0.1f * (stats.jobCount > 0 ? static_cast<float>(stats.totalJobTimeNs / stats.jobCount) : 1e6f);
        m_EWMAFrameTimeMs = 0.9f * m_EWMAFrameTimeMs + 0.1f * frameTimeMs;

        float reward = -frameTimeMs;
        if (frameTimeMs > 33.33f) reward -= 10.0f;

        m_StepCount++;
        bool done = m_StepCount >= 500;

        py::dict info;
        info["frame_time"] = frameTimeMs;
        info["steals"] = static_cast<int64_t>(stats.steals);
        info["pushes"] = static_cast<int64_t>(stats.pushes);
        info["job_count"] = static_cast<int64_t>(stats.jobCount);
        info["cube_count"] = static_cast<int64_t>(cubeCount);

        return py::make_tuple(getObs(), reward, done, info);
    }

    int getObsDim() const { return kObsDim; }
    int getActDim() const { return kActDim; }

    ~FrameSchedulerEnv() {
        if (m_Initialized) {
            rldemo::JobSystem::Shutdown();
            m_Initialized = false;
        }
    }

private:
    float runDemoWorkload(int cubeCount, float camAngle = 0.f) {
        auto start = std::chrono::high_resolution_clock::now();
        InstanceStaging staging;
        staging.data.reserve(cubeCount * InstanceStaging::kFloatsPerInstance);

        const int batchSize = std::max(1, cubeCount / static_cast<int>(m_WorkerCount));

        for (uint32_t w = 0; w < m_WorkerCount; ++w) {
            int startIdx = static_cast<int>(w) * batchSize;
            int endIdx = (w == m_WorkerCount - 1) ? cubeCount : static_cast<int>(w + 1) * batchSize;

            rldemo::JobSystem::Enqueue([&staging, startIdx, endIdx, camAngle]() {
                std::mt19937 rng(startIdx + 12345);
                std::uniform_real_distribution<float> dist(-50.f, 50.f);
                for (int i = startIdx; i < endIdx; ++i) {
                    float x = dist(rng), y = dist(rng), z = dist(rng);
                    float c = cosf(camAngle), s = sinf(camAngle);
                    float model[16] = {
                        c, -s, 0, 0,  s, c, 0, 0,  0, 0, 1, 0,  x, y, z, 1
                    };
                    float color[4] = {(i % 100) / 100.f, 0.5f, 0.8f, 1.f};
                    {
                        std::lock_guard<std::mutex> lock(staging.mutex);
                        for (int j = 0; j < 16; ++j) staging.data.push_back(model[j]);
                        for (int j = 0; j < 4; ++j) staging.data.push_back(color[j]);
                    }
                }
            }, {});
        }
        rldemo::JobSystem::WaitForAll();

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float, std::milli>(end - start).count();
    }

    py::array_t<float> getObs() {
        auto stats = rldemo::JobSystem::GetStats();
        auto obs = py::array_t<float>(kObsDim);
        auto r = obs.mutable_unchecked<1>();
        float queueProxy = stats.jobCount > 0 ? std::min(1.f, static_cast<float>(stats.pushes) / 10000.f) : 0.5f;
        r(0) = queueProxy;
        r(1) = m_EWMAJobTimeNs / 1e6f;
        r(2) = std::min(1.f, static_cast<float>(stats.steals) / 1000.f);
        r(3) = m_EWMAFrameTimeMs / 100.f;
        r(4) = 0.f;
        r(5) = static_cast<float>(m_WorkerCount) / 16.f;
        r(6) = stats.jobCount > 0 ? std::min(1.f, static_cast<float>(stats.pops) / 5000.f) : 0.5f;
        r(7) = stats.jobCount > 0 ? std::min(1.f, static_cast<float>(stats.totalJobTimeNs) / 1e9f) : 0.5f;
        return obs;
    }

    std::mt19937 m_Rng;
    int m_StepCount = 0;
    float m_EWMAJobTimeNs = 1e6f;
    float m_EWMAFrameTimeMs = 16.67f;
    uint32_t m_WorkerCount = 4;
    bool m_Initialized = false;
};

// ---------------------------------------------------------------------------
// JobSchedulerEnv - FIXED workload, RL optimizes work distribution
// ---------------------------------------------------------------------------
/**
 * JobSchedulerEnv - Fixed heavy workload (100k cubes), RL controls work distribution
 * 
 * Observation (4 * W + 7 dims where W = num workers):
 *   Per worker (4 features each):
 *     - queue_depth (normalized)
 *     - job_time (normalized avg job execution time)
 *     - idle_ratio (fraction of time idle)
 *     - steal_success_rate
 *   Global (7 features):
 *     - frame_time_ewma (normalized)
 *     - frame_time_variance (normalized)
 *     - total_steals (normalized)
 *     - steal_attempts (normalized)
 *     - worker_count (normalized)
 *     - total_job_time (normalized)
 *     - global_queue_depth (normalized)
 * 
 * Action (W dims): batch_factor per worker - determines work distribution
 *   Action[i] in [0.01, 1.0] -> normalized to sum to 1.0
 *   Worker i processes batch_factor[i] * total_work items
 * 
 * Reward: -frame_time_ms - 0.1 * variance_penalty
 *   Encourages minimizing frame time AND variance (consistency)
 */
class JobSchedulerEnv {
public:
    static constexpr int kObsDim = 4 * kMaxWorkersForObs + 7;  // 71 dims
    static constexpr int kActDim = kMaxWorkersForObs;          // 16 dims

    JobSchedulerEnv() : m_Initialized(false), m_CubeCount(100000) {}

    py::array_t<float> reset() {
        m_StepCount = 0;
        m_EWMAFrameTimeMs = 16.67f;
        m_FrameTimeVariance = 0.f;
        m_LastFrameTimes.clear();

        if (!m_Initialized) {
            uint32_t workers = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
            rldemo::JobSystem::Initialize(workers);
            m_WorkerCount = rldemo::JobSystem::GetWorkerCount();
            m_Initialized = true;
        }

        // Reset worker timing stats
        for (int i = 0; i < kMaxWorkersForObs; ++i) {
            m_WorkerJobTimes[i] = 0.f;
            m_WorkerIdleRatios[i] = 0.5f;
        }

        rldemo::JobSystem::ResetStats();
        // Warmup with uniform distribution
        runWorkload(nullptr);
        return getObs();
    }

    py::object step(py::array_t<float> action) {
        if (action.size() != kActDim) {
            throw std::runtime_error("Action must have " + std::to_string(kActDim) + " elements");
        }
        auto a = action.unchecked<1>();

        // Parse batch factors from action
        float batchFactors[kMaxWorkersForObs];
        float sum = 0.f;
        for (int i = 0; i < kMaxWorkersForObs; ++i) {
            batchFactors[i] = std::max(0.01f, std::min(1.f, a(i)));
            sum += batchFactors[i];
        }
        // Normalize to sum to 1.0
        for (int i = 0; i < kMaxWorkersForObs; ++i) {
            batchFactors[i] /= sum;
        }

        rldemo::JobSystem::ResetStats();
        float frameTimeMs = runWorkload(batchFactors);

        // Update EWMA and variance
        m_EWMAFrameTimeMs = 0.9f * m_EWMAFrameTimeMs + 0.1f * frameTimeMs;
        float diff = frameTimeMs - m_EWMAFrameTimeMs;
        m_FrameTimeVariance = 0.9f * m_FrameTimeVariance + 0.1f * diff * diff;

        // Track recent frame times for variance calculation
        m_LastFrameTimes.push_back(frameTimeMs);
        if (m_LastFrameTimes.size() > 20) {
            m_LastFrameTimes.erase(m_LastFrameTimes.begin());
        }

        // Reward: minimize frame time + variance penalty for consistency
        float reward = -frameTimeMs - 0.1f * std::sqrt(m_FrameTimeVariance);

        m_StepCount++;
        bool done = m_StepCount >= 500;

        auto stats = rldemo::JobSystem::GetStats();
        py::dict info;
        info["frame_time"] = frameTimeMs;
        info["frame_time_ewma"] = m_EWMAFrameTimeMs;
        info["frame_time_std"] = std::sqrt(m_FrameTimeVariance);
        info["steals"] = static_cast<int64_t>(stats.steals);
        info["steal_attempts"] = static_cast<int64_t>(stats.stealAttempts);
        info["cube_count"] = static_cast<int64_t>(m_CubeCount);
        info["worker_count"] = static_cast<int64_t>(m_WorkerCount);

        return py::make_tuple(getObs(), reward, done, info);
    }

    int getObsDim() const { return kObsDim; }
    int getActDim() const { return kActDim; }
    
    void setCubeCount(int n) {
        m_CubeCount = std::max(10000, std::min(200000, n));
    }

    ~JobSchedulerEnv() {
        if (m_Initialized) {
            rldemo::JobSystem::Shutdown();
            m_Initialized = false;
        }
    }

private:
    float runWorkload(const float* batchFactors) {
        const int n = m_CubeCount;
        const uint32_t W = m_WorkerCount;

        // Calculate work distribution
        int limits[kMaxWorkersForObs + 1];
        if (batchFactors) {
            limits[0] = 0;
            for (uint32_t w = 0; w < W && w < static_cast<uint32_t>(kMaxWorkersForObs); ++w) {
                limits[w + 1] = limits[w] + static_cast<int>(batchFactors[w] * n);
            }
            limits[W] = n;  // Ensure last worker gets remaining work
            for (uint32_t w = W + 1; w <= static_cast<uint32_t>(kMaxWorkersForObs); ++w) {
                limits[w] = n;
            }
        } else {
            // Uniform distribution
            int b = std::max(1, n / static_cast<int>(W));
            for (uint32_t w = 0; w <= W; ++w) {
                limits[w] = (w == W) ? n : static_cast<int>(w) * b;
            }
        }

        InstanceStaging staging;
        staging.data.reserve(n * InstanceStaging::kFloatsPerInstance);

        auto start = std::chrono::high_resolution_clock::now();

        // Spawn jobs with the specified distribution
        // NOTE: Work complexity varies by index - first 30% cubes are 3x more complex
        // This creates a NON-UNIFORM workload where smart distribution matters!
        for (uint32_t w = 0; w < W; ++w) {
            int s = limits[w], e = limits[w + 1];
            if (s >= e) continue;

            rldemo::JobSystem::Enqueue([&staging, s, e, n]() {
                std::mt19937 rng(s + 12345);
                std::uniform_real_distribution<float> dist(-50.f, 50.f);
                std::vector<float> localData;
                localData.reserve((e - s) * InstanceStaging::kFloatsPerInstance);
                
                int complexThreshold = n * 3 / 10;  // First 30% are complex
                
                for (int i = s; i < e; ++i) {
                    float x = dist(rng), y = dist(rng), z = dist(rng);
                    
                    // Variable complexity: early indices have more computation
                    // This simulates real scenarios (e.g., detailed objects first, LOD)
                    int iterations = (i < complexThreshold) ? 3 : 1;
                    float angle = i * 0.01f;
                    
                    for (int iter = 0; iter < iterations; ++iter) {
                        // Extra work for complex cubes
                        float c = cosf(angle + iter * 0.1f);
                        float ss = sinf(angle + iter * 0.1f);
                        angle = angle + c * ss * 0.01f;  // Dependent computation
                    }
                    
                    float c = cosf(angle), ss = sinf(angle);
                    float model[16] = {
                        c, -ss, 0, 0,  ss, c, 0, 0,  0, 0, 1, 0,  x, y, z, 1
                    };
                    float color[4] = {(i % 100) / 100.f, 0.5f, 0.8f, 1.f};
                    for (int j = 0; j < 16; ++j) localData.push_back(model[j]);
                    for (int j = 0; j < 4; ++j) localData.push_back(color[j]);
                }
                
                // Batch insert to reduce lock contention
                {
                    std::lock_guard<std::mutex> lock(staging.mutex);
                    staging.data.insert(staging.data.end(), localData.begin(), localData.end());
                }
            }, {});
        }

        rldemo::JobSystem::WaitForAll();

        auto end = std::chrono::high_resolution_clock::now();
        float frameTimeMs = std::chrono::duration<float, std::milli>(end - start).count();

        // Update per-worker stats
        auto workerStats = rldemo::JobSystem::GetWorkerStats();
        for (size_t w = 0; w < workerStats.size() && w < kMaxWorkersForObs; ++w) {
            const auto& ws = workerStats[w];
            m_WorkerJobTimes[w] = 0.9f * m_WorkerJobTimes[w] + 
                0.1f * (ws.jobCount > 0 ? static_cast<float>(ws.totalTimeNs) / ws.jobCount / 1e6f : 0.f);
            uint64_t totalNs = ws.totalTimeNs + ws.idleNs;
            m_WorkerIdleRatios[w] = 0.9f * m_WorkerIdleRatios[w] + 
                0.1f * (totalNs > 0 ? static_cast<float>(ws.idleNs) / totalNs : 0.5f);
        }

        return frameTimeMs;
    }

    py::array_t<float> getObs() {
        auto stats = rldemo::JobSystem::GetStats();
        auto workerStats = rldemo::JobSystem::GetWorkerStats();
        size_t globalDepth = rldemo::JobSystem::GetGlobalQueueDepth();

        auto obs = py::array_t<float>(kObsDim);
        auto r = obs.mutable_unchecked<1>();
        int idx = 0;

        // Per-worker features (4 each)
        for (int w = 0; w < kMaxWorkersForObs; ++w) {
            if (w < static_cast<int>(workerStats.size())) {
                const auto& ws = workerStats[w];
                r(idx++) = std::min(1.f, static_cast<float>(ws.queueDepth) / 64.f);
                r(idx++) = std::min(1.f, m_WorkerJobTimes[w] / 10.f);  // Normalized job time
                r(idx++) = m_WorkerIdleRatios[w];
                float stealRate = (stats.stealAttempts > 0) ? 
                    std::min(1.f, static_cast<float>(stats.steals) / static_cast<float>(stats.stealAttempts)) : 0.5f;
                r(idx++) = stealRate;
            } else {
                r(idx++) = 0.f;
                r(idx++) = 0.f;
                r(idx++) = 0.5f;
                r(idx++) = 0.5f;
            }
        }

        // Global features (7)
        r(idx++) = m_EWMAFrameTimeMs / 100.f;
        r(idx++) = std::sqrt(m_FrameTimeVariance) / 50.f;
        r(idx++) = std::min(1.f, static_cast<float>(stats.steals) / 1000.f);
        r(idx++) = std::min(1.f, static_cast<float>(stats.stealAttempts) / 2000.f);
        r(idx++) = static_cast<float>(m_WorkerCount) / 16.f;
        r(idx++) = stats.jobCount > 0 ? std::min(1.f, static_cast<float>(stats.totalJobTimeNs) / 1e9f) : 0.5f;
        r(idx++) = std::min(1.f, static_cast<float>(globalDepth) / 256.f);

        return obs;
    }

    bool m_Initialized;
    int m_StepCount = 0;
    int m_CubeCount;
    float m_EWMAFrameTimeMs = 16.67f;
    float m_FrameTimeVariance = 0.f;
    uint32_t m_WorkerCount = 4;
    std::vector<float> m_LastFrameTimes;
    float m_WorkerJobTimes[kMaxWorkersForObs] = {0.f};
    float m_WorkerIdleRatios[kMaxWorkersForObs] = {0.5f};
};

// ---------------------------------------------------------------------------
// WorkStealSchedulerEnv
// ---------------------------------------------------------------------------
class WorkStealSchedulerEnv {
public:
    static constexpr int kObsDim = 4 * kMaxWorkersForObs + 11;  // 75
    static constexpr int kActDim = kMaxWorkersForObs + 5;       // 21

    WorkStealSchedulerEnv() : m_Initialized(false), m_BodyCount(2000) {}

    py::array_t<float> reset() {
        m_StepCount = 0;
        m_EWMAFrameTimeMs = 16.67f;
        m_FrameTimeVariance = 0.f;

        if (!m_Initialized) {
            uint32_t workers = std::max(1u, static_cast<uint32_t>(std::thread::hardware_concurrency()) - 1);
            rldemo::JobSystem::Initialize(workers);
            m_WorkerCount = rldemo::JobSystem::GetWorkerCount();
            m_Sim = std::make_unique<rldemo::NBodySim>(10000);
            m_Initialized = true;
        }

        m_Sim->Resize(static_cast<uint32_t>(m_BodyCount));
        m_Sim->RandomizePositionsAndVelocities(40.f, 1.f);
        rldemo::JobSystem::ResetStats();
        runPhysicsWorkload(m_BodyCount, nullptr);
        return getObs();
    }

    py::object step(py::array_t<float> action) {
        if (action.size() != kActDim) {
            throw std::runtime_error("Action must have " + std::to_string(kActDim) + " elements");
        }
        auto a = action.unchecked<1>();

        float batchFactors[kMaxWorkersForObs];
        float sum = 0.f;
        for (int i = 0; i < kMaxWorkersForObs; ++i) {
            batchFactors[i] = std::max(0.01f, std::min(1.f, a(i)));
            sum += batchFactors[i];
        }
        for (int i = 0; i < kMaxWorkersForObs; ++i) batchFactors[i] /= sum;

        rldemo::JobSystem::ResetStats();
        float frameTimeMs = runPhysicsWorkload(m_BodyCount, batchFactors);

        m_EWMAFrameTimeMs = 0.9f * m_EWMAFrameTimeMs + 0.1f * frameTimeMs;
        float diff = frameTimeMs - m_EWMAFrameTimeMs;
        m_FrameTimeVariance = 0.9f * m_FrameTimeVariance + 0.1f * diff * diff;

        float reward = -frameTimeMs - 0.1f * std::sqrt(m_FrameTimeVariance);
        if (frameTimeMs > 33.33f) reward -= 10.0f;

        m_StepCount++;
        bool done = m_StepCount >= 500;

        auto stats = rldemo::JobSystem::GetStats();
        py::dict info;
        info["frame_time"] = frameTimeMs;
        info["steals"] = static_cast<int64_t>(stats.steals);
        info["steal_attempts"] = static_cast<int64_t>(stats.stealAttempts);
        info["body_count"] = static_cast<int64_t>(m_BodyCount);

        return py::make_tuple(getObs(), reward, done, info);
    }

    int getObsDim() const { return kObsDim; }
    int getActDim() const { return kActDim; }
    void setBodyCount(int n) {
        m_BodyCount = std::max(500, std::min(8000, n));
        if (m_Sim) {
            m_Sim->Resize(static_cast<uint32_t>(m_BodyCount));
        }
    }

    ~WorkStealSchedulerEnv() {
        if (m_Initialized) {
            rldemo::JobSystem::Shutdown();
            m_Initialized = false;
        }
    }

private:
    float runPhysicsWorkload(int bodyCount, const float* batchFactors) {
        m_Sim->Resize(static_cast<uint32_t>(bodyCount));
        auto& bodies = m_Sim->GetBodies();
        const int n = bodyCount;
        const uint32_t W = m_WorkerCount;

        int limits[kMaxWorkersForObs + 1];
        if (batchFactors) {
            limits[0] = 0;
            for (uint32_t w = 0; w < W && w < static_cast<uint32_t>(kMaxWorkersForObs); ++w) {
                limits[w + 1] = limits[w] + static_cast<int>(batchFactors[w] * n);
            }
            limits[W] = n;
            for (uint32_t w = W + 1; w <= static_cast<uint32_t>(kMaxWorkersForObs); ++w) limits[w] = n;
        } else {
            int b = std::max(1, n / static_cast<int>(W));
            for (uint32_t w = 0; w <= W; ++w)
                limits[w] = (w == W) ? n : static_cast<int>(w) * b;
        }

        auto start = std::chrono::high_resolution_clock::now();

        for (uint32_t w = 0; w < W; ++w) {
            int s = limits[w], e = limits[w + 1];
            if (s >= e) continue;
            rldemo::JobSystem::Enqueue([this, &bodies, s, e]() {
                m_Sim->ComputeGravityRange(s, e, bodies);
            }, {});
        }
        rldemo::JobSystem::WaitForAll();

        for (uint32_t w = 0; w < W; ++w) {
            int s = limits[w], e = limits[w + 1];
            if (s >= e) continue;
            rldemo::JobSystem::Enqueue([this, &bodies, s, e]() {
                m_Sim->DetectAndResolveCollisionsRange(s, e, bodies);
            }, {});
        }
        rldemo::JobSystem::WaitForAll();

        const float dt = 0.016f;
        for (uint32_t w = 0; w < W; ++w) {
            int s = limits[w], e = limits[w + 1];
            if (s >= e) continue;
            rldemo::JobSystem::Enqueue([this, &bodies, s, e, dt]() {
                m_Sim->IntegrateRange(s, e, bodies, dt);
            }, {});
        }
        rldemo::JobSystem::WaitForAll();

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float, std::milli>(end - start).count();
    }

    py::array_t<float> getObs() {
        auto stats = rldemo::JobSystem::GetStats();
        auto workerStats = rldemo::JobSystem::GetWorkerStats();
        size_t globalDepth = rldemo::JobSystem::GetGlobalQueueDepth();

        auto obs = py::array_t<float>(kObsDim);
        auto r = obs.mutable_unchecked<1>();
        int idx = 0;
        for (int w = 0; w < kMaxWorkersForObs; ++w) {
            if (w < static_cast<int>(workerStats.size())) {
                const auto& ws = workerStats[w];
                r(idx++) = std::min(1.f, static_cast<float>(ws.queueDepth) / 64.f);
                float stealRate = (stats.stealAttempts > 0) ? std::min(1.f, static_cast<float>(stats.steals) / static_cast<float>(stats.stealAttempts)) : 0.5f;
                r(idx++) = stealRate;
                r(idx++) = (ws.jobCount > 0) ? std::min(1.f, static_cast<float>(ws.totalTimeNs) / 1e8f) : 0.f;
                uint64_t totalNs = ws.totalTimeNs + ws.idleNs;
                r(idx++) = (totalNs > 0) ? std::min(1.f, static_cast<float>(ws.idleNs) / static_cast<float>(totalNs)) : 0.5f;
            } else {
                r(idx++) = 0.f; r(idx++) = 0.5f; r(idx++) = 0.f; r(idx++) = 0.5f;
            }
        }
        r(idx++) = m_EWMAFrameTimeMs / 100.f;
        r(idx++) = static_cast<float>(m_BodyCount) / 8000.f;
        r(idx++) = std::min(1.f, static_cast<float>(globalDepth) / 256.f);
        r(idx++) = std::min(1.f, static_cast<float>(stats.steals) / 1000.f);
        r(idx++) = std::min(1.f, static_cast<float>(stats.stealAttempts) / 2000.f);
        r(idx++) = std::min(1.f, static_cast<float>(stats.pushes) / 5000.f);
        r(idx++) = static_cast<float>(m_WorkerCount) / 16.f;
        r(idx++) = stats.jobCount > 0 ? std::min(1.f, static_cast<float>(stats.totalJobTimeNs) / 1e9f) : 0.5f;
        r(idx++) = std::sqrt(m_FrameTimeVariance) / 50.f;
        r(idx++) = (stats.jobCount > 0) ? std::min(1.f, static_cast<float>(stats.pops) / 5000.f) : 0.5f;
        r(idx++) = 0.f;
        return obs;
    }

    bool m_Initialized;
    int m_StepCount = 0;
    int m_BodyCount;
    float m_EWMAFrameTimeMs = 16.67f;
    float m_FrameTimeVariance = 0.f;
    uint32_t m_WorkerCount = 4;
    std::unique_ptr<rldemo::NBodySim> m_Sim;
};

PYBIND11_MODULE(rldemo_env, m) {
    py::class_<FrameSchedulerEnv>(m, "FrameSchedulerEnv")
        .def(py::init<>())
        .def("reset", &FrameSchedulerEnv::reset)
        .def("step", &FrameSchedulerEnv::step)
        .def("get_obs_dim", &FrameSchedulerEnv::getObsDim)
        .def("get_act_dim", &FrameSchedulerEnv::getActDim);

    py::class_<JobSchedulerEnv>(m, "JobSchedulerEnv")
        .def(py::init<>())
        .def("reset", &JobSchedulerEnv::reset)
        .def("step", &JobSchedulerEnv::step)
        .def("get_obs_dim", &JobSchedulerEnv::getObsDim)
        .def("get_act_dim", &JobSchedulerEnv::getActDim)
        .def("set_cube_count", &JobSchedulerEnv::setCubeCount);

    py::class_<WorkStealSchedulerEnv>(m, "WorkStealSchedulerEnv")
        .def(py::init<>())
        .def("reset", &WorkStealSchedulerEnv::reset)
        .def("step", &WorkStealSchedulerEnv::step)
        .def("get_obs_dim", &WorkStealSchedulerEnv::getObsDim)
        .def("get_act_dim", &WorkStealSchedulerEnv::getActDim)
        .def("set_body_count", &WorkStealSchedulerEnv::setBodyCount);
}
