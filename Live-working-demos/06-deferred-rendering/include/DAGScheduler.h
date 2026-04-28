#pragma once

#include "RenderGraph.h"
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>

struct GLFWwindow;

enum class SchedulingMode {
    Baseline,          // Sequential, no DAG, single stream
    DAGOnly,           // DAG scheduling, no work stealing
    DAGWorkStealing,   // DAG + work stealing
    FullSystem         // DAG + work stealing + adaptive policy
};

class DAGScheduler {
public:
    DAGScheduler(std::shared_ptr<RenderGraph> graph);
    ~DAGScheduler();

    void setMode(SchedulingMode mode) { m_mode = mode; }
    SchedulingMode getMode() const { return m_mode; }

    // Execute one frame
    void executeFrame();

    // Get worker statistics
    struct WorkerStats {
        int tasksExecuted = 0;
        int stealAttempts = 0;
        int successfulSteals = 0;
        float totalWorkTime = 0.0f;
        int stateChanges = 0;
    };

    std::vector<WorkerStats> getWorkerStats() const { return m_workerStats; }
    std::vector<WorkerStats> getCPUWorkerStats() const { return m_cpuWorkerStats; }

    // Get frame timing
    float getFrameTime() const { return m_frameTime; }
    float getCPUTime() const { return m_cpuTime; }
    float getGPUTime() const { return m_gpuTime; }

    // Work stealing control
    void enableWorkStealing(bool enable) { m_workStealingEnabled = enable; }
    bool isWorkStealingEnabled() const { return m_workStealingEnabled; }

    // Worker thread count
    void setWorkerThreadCount(int count);
    int getWorkerThreadCount() const { return static_cast<int>(m_workerThreads.size()); }

    // Set the main window (kept for compatibility)
    void setMainWindow(GLFWwindow* window);

    // AI backend integration
    void setAIBackend(void* aiBackend);
    void adaptStreamCount(int recommendedStreams);

private:
    void start();
    void stop();
    std::shared_ptr<RenderGraph> m_graph;
    SchedulingMode m_mode;

    GLFWwindow* m_mainWindow = nullptr;
    void* m_aiBackend = nullptr;

    // CUDA streams for parallel compute
    std::vector<void*> m_cudaStreams;
    int m_cudaStreamCount;

    // CPU worker threads
    std::vector<std::thread> m_cpuWorkerThreads;
    std::vector<std::queue<std::shared_ptr<RenderTask>>> m_cpuTaskQueues;
    std::vector<WorkerStats> m_cpuWorkerStats;

    // Legacy worker threads (kept for compatibility)
    std::vector<std::thread> m_workerThreads;
    std::vector<std::queue<std::shared_ptr<RenderTask>>> m_taskQueues;
    std::vector<WorkerStats> m_workerStats;
    std::vector<GLState> m_lastWorkerState;

    std::atomic<bool> m_running;
    std::mutex m_queueMutex;
    std::condition_variable m_workAvailable;
    std::mutex m_statsMutex;
    std::mutex m_cpuQueueMutex;
    std::condition_variable m_cpuWorkAvailable;

    bool m_workStealingEnabled;
    int m_workerCount;

    float m_frameTime;
    float m_cpuTime;
    float m_gpuTime;

    void workerThread(int workerID);
    void cpuWorkerThread(int workerID);
    bool tryStealTask(int thiefID, std::shared_ptr<RenderTask>& outTask);
    float calculateStealCost(std::shared_ptr<RenderTask> task, int thiefID) const;
    void executeBaseline();
    void executeDAGOnly();
    void executeDAGWorkStealing();
    void executeFullSystem();
    float estimateGPUTime() const;
    void waitForTasksComplete(const std::vector<std::shared_ptr<RenderTask>>& tasks);

    // Three-tier execution methods
    void executeGLTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks);
    void launchCUDATasks(const std::vector<std::shared_ptr<RenderTask>>& tasks);
    void dispatchCPUTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks);
    void synchronizeAll();

    // CUDA stream management
    void initializeCudaStreams(int count);
    void destroyCudaStreams();
};
