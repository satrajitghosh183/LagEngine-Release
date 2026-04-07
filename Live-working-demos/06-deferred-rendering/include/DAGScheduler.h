#pragma once

#include "RenderGraph.h"
#include <thread>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Forward declaration for GLFW window
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
    int getWorkerThreadCount() const { return m_workerThreads.size(); }
    
    // Set the main window for creating shared contexts
    void setMainWindow(GLFWwindow* window);
    
    // AI backend integration (for Full System mode)
    void setAIBackend(void* aiBackend);  // PipelineController* (void* to avoid circular dependency)
    void adaptStreamCount(int recommendedStreams);  // Called by AI backend

private:
    void start();
    void stop();
    std::shared_ptr<RenderGraph> m_graph;
    SchedulingMode m_mode;
    
    GLFWwindow* m_mainWindow;  // Main window (for main thread GL context)
    void* m_aiBackend;  // PipelineController* (void* to avoid circular dependency)
    
    // CUDA streams for parallel compute
    std::vector<void*> m_cudaStreams;  // cudaStream_t (void* to avoid CUDA header in .h)
    int m_cudaStreamCount;
    
    // CPU worker threads (no OpenGL contexts - for CPU-side work only)
    std::vector<std::thread> m_cpuWorkerThreads;
    std::vector<std::queue<std::shared_ptr<RenderTask>>> m_cpuTaskQueues;
    std::vector<WorkerStats> m_cpuWorkerStats;
    
    // Legacy worker threads (for backward compatibility, but won't be used in WSL mode)
    std::vector<GLFWwindow*> m_workerContexts;
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
    void cpuWorkerThread(int workerID);  // CPU worker (no OpenGL)
    bool tryStealTask(int thiefID, std::shared_ptr<RenderTask>& outTask);
    float calculateStealCost(std::shared_ptr<RenderTask> task, int thiefID) const;
    void executeBaseline();
    void executeDAGOnly();
    void executeDAGWorkStealing();
    void executeFullSystem();  // Full system with AI adaptation
    float estimateGPUTime() const; // Estimate GPU time from profiler data
    void waitForTasksComplete(const std::vector<std::shared_ptr<RenderTask>>& tasks); // Wait for specific tasks to complete
    
    // Three-tier execution methods
    void executeGLTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks);  // Phase 1: GL on main thread
    void launchCUDATasks(const std::vector<std::shared_ptr<RenderTask>>& tasks); // Phase 2: CUDA streams
    void dispatchCPUTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks); // Phase 3: CPU workers
    void synchronizeAll();  // Phase 4: Wait for all to complete
    
    // CUDA stream management
    void initializeCudaStreams(int count);
    void destroyCudaStreams();
};

