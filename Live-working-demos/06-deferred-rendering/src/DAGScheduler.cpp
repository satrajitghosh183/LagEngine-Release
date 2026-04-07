#include "DAGScheduler.h"
#include "Profiler.h"
#include "RenderGraph.h"
#include <glad/glad.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <cuda_runtime.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <iostream>

DAGScheduler::DAGScheduler(std::shared_ptr<RenderGraph> graph)
    : m_graph(graph)
    , m_mode(SchedulingMode::Baseline)
    , m_running(false)
    , m_workStealingEnabled(false)
    , m_workerCount(0)
    , m_frameTime(0.0f)
    , m_cpuTime(0.0f)
    , m_gpuTime(0.0f)
    , m_mainWindow(nullptr)
    , m_aiBackend(nullptr)
    , m_cudaStreamCount(4)
{
    setWorkerThreadCount(4);
    initializeCudaStreams(4);  // Initialize 4 CUDA streams
}

DAGScheduler::~DAGScheduler() {
    stop();
    
    // Clean up CUDA streams
    destroyCudaStreams();
    
    // Clean up worker contexts
    for (auto* ctx : m_workerContexts) {
        if (ctx) {
            glfwDestroyWindow(ctx);
        }
    }
    m_workerContexts.clear();
}

void DAGScheduler::setWorkerThreadCount(int count) {
    stop();
    
    m_workerCount = count;
    
    // Initialize CPU worker threads (no OpenGL contexts)
    m_cpuTaskQueues.clear();
    m_cpuTaskQueues.resize(count);
    m_cpuWorkerStats.clear();
    m_cpuWorkerStats.resize(count);
    
    // Initialize stats
    for (auto& stats : m_cpuWorkerStats) {
        stats = WorkerStats();
    }
    
    // Legacy worker threads (not used in WSL mode, but kept for compatibility)
    m_taskQueues.clear();
    m_taskQueues.resize(count);
    m_workerStats.clear();
    m_workerStats.resize(count);
    m_lastWorkerState.clear();
    m_lastWorkerState.resize(count);
    m_workerContexts.clear();
    m_workerContexts.resize(count, nullptr);
    
    for (auto& stats : m_workerStats) {
        stats = WorkerStats();
    }
}

void DAGScheduler::stop() {
    if (m_running) {
        m_running = false;
        m_workAvailable.notify_all();
        m_cpuWorkAvailable.notify_all();
        
        // Stop CPU workers
        for (auto& thread : m_cpuWorkerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_cpuWorkerThreads.clear();
        
        // Stop legacy workers
        for (auto& thread : m_workerThreads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        m_workerThreads.clear();
    }
}

void DAGScheduler::executeFrame() {
    auto start = std::chrono::high_resolution_clock::now();
    
    m_graph->resetFrame();
    
    Profiler::instance().startProfile("Frame");
    
    // Create OpenGL timer query for GPU time measurement
    unsigned int gpuTimeQuery;
    glGenQueries(1, &gpuTimeQuery);
    glBeginQuery(GL_TIME_ELAPSED, gpuTimeQuery);
    
    switch (m_mode) {
        case SchedulingMode::Baseline:
            executeBaseline();
            break;
        case SchedulingMode::DAGOnly:
            executeDAGOnly();
            break;
        case SchedulingMode::DAGWorkStealing:
            executeDAGWorkStealing();
            break;
        case SchedulingMode::FullSystem:
            executeFullSystem(); // Full system with AI-driven adaptive policy
            break;
    }
    
    glEndQuery(GL_TIME_ELAPSED);
    
    Profiler::instance().endProfile("Frame");
    
    // Restore main context (in case worker threads changed it)
    if (m_mainWindow) {
        glfwMakeContextCurrent(m_mainWindow);
    }
    
    // Get GPU time from query
    GLuint64 gpuTimeNs = 0;
    GLint available = 0;
    glGetQueryObjectiv(gpuTimeQuery, GL_QUERY_RESULT_AVAILABLE, &available);
    if (available) {
        glGetQueryObjectui64v(gpuTimeQuery, GL_QUERY_RESULT, &gpuTimeNs);
        m_gpuTime = gpuTimeNs / 1000000.0f; // Convert nanoseconds to milliseconds
    } else {
        // Query not ready yet, estimate from profiler data
        m_gpuTime = estimateGPUTime();
    }
    glDeleteQueries(1, &gpuTimeQuery);
    
    // CPU time is total frame time minus GPU time (overhead/submission)
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_frameTime = duration.count() / 1000.0f;
    m_cpuTime = std::max(0.0f, m_frameTime - m_gpuTime);
}

void DAGScheduler::executeBaseline() {
    // Sequential execution - simplest mode
    if (!m_graph) {
        std::cerr << "ERROR: RenderGraph is null!" << std::endl;
        return;
    }
    
    auto& tasks = m_graph->getTasks();
    if (tasks.empty()) {
        std::cerr << "WARNING: No tasks in render graph!" << std::endl;
        return;
    }
    
    // Sort tasks by dependencies manually (simple approach)
    std::vector<std::shared_ptr<RenderTask>> sortedTasks;
    std::unordered_map<std::shared_ptr<RenderTask>, bool> visited;
    
    std::function<void(std::shared_ptr<RenderTask>)> visit = [&](std::shared_ptr<RenderTask> task) {
        if (!task || visited[task]) return;
        
        // Visit dependencies first
        for (const auto& dep : task->getDependencies()) {
            if (auto depPtr = dep.lock()) {
                visit(depPtr);
            }
        }
        
        visited[task] = true;
        sortedTasks.push_back(task);
    };
    
    for (const auto& task : tasks) {
        if (task) {
            visit(task);
        }
    }
    
    // Execute sequentially
    for (auto& task : sortedTasks) {
        if (task) {
            Profiler::instance().startProfile(task->getName());
            task->execute();
            Profiler::instance().endProfile(task->getName());
        }
    }
}

void DAGScheduler::executeDAGOnly() {
    // Three-tier parallelization: GL (main thread), CUDA (streams), CPU (workers)
    if (!m_graph) {
        std::cerr << "ERROR: RenderGraph is null!" << std::endl;
        return;
    }
    
    // Ensure CPU workers are running
    if (!m_running) {
        start();
    }
    
    // Reset all tasks
    for (const auto& task : m_graph->getTasks()) {
        if (task) {
            task->reset();
        }
    }
    
    auto levels = m_graph->getLevels();
    if (levels.empty()) {
        std::cerr << "WARNING: No levels in render graph!" << std::endl;
        return;
    }
    
    // Process level by level (respecting dependencies)
    for (const auto& level : levels) {
        if (level.empty()) continue;
        
        // Collect tasks that are ready to execute in this level
        std::vector<std::shared_ptr<RenderTask>> readyTasks;
        for (const auto& task : level) {
            if (!task) continue;
            
            // Check if dependencies are ready
            bool depsReady = true;
            for (const auto& dep : task->getDependencies()) {
                if (auto depPtr = dep.lock()) {
                    if (!depPtr->isComplete()) {
                        depsReady = false;
                        break;
                    }
                }
            }
            
            if (depsReady) {
                readyTasks.push_back(task);
            }
        }
        
        if (readyTasks.empty()) continue;
        
        // Separate tasks by executor type
        std::vector<std::shared_ptr<RenderTask>> glTasks;
        std::vector<std::shared_ptr<RenderTask>> cudaTasks;
        std::vector<std::shared_ptr<RenderTask>> cpuTasks;
        
        for (const auto& task : readyTasks) {
            switch (task->getExecutor()) {
                case TaskExecutor::OpenGL_MainThread:
                    glTasks.push_back(task);
                    break;
                case TaskExecutor::CUDA_Stream:
                    cudaTasks.push_back(task);
                    break;
                case TaskExecutor::CPU_Worker:
                    cpuTasks.push_back(task);
                    break;
            }
        }
        
        // Phase 1: Execute GL tasks on main thread (sequential, but respects DAG)
        executeGLTasks(glTasks);
        
        // Phase 2: Launch CUDA tasks on parallel streams
        launchCUDATasks(cudaTasks);
        
        // Phase 3: Dispatch CPU tasks to worker threads
        dispatchCPUTasks(cpuTasks);
        
        // Phase 4: Synchronize all (wait for CUDA and CPU tasks)
        synchronizeAll();
        
        // Wait for all tasks in this level to complete
        waitForTasksComplete(readyTasks);
    }
}

void DAGScheduler::executeDAGWorkStealing() {
    // Three-tier parallelization with work stealing: GL (main thread), CUDA (streams), CPU (workers)
    // Work stealing applies to CPU tasks only
    if (!m_graph) {
        std::cerr << "ERROR: RenderGraph is null!" << std::endl;
        return;
    }
    
    // Ensure CPU workers are running and work stealing is enabled
    if (!m_running) {
        start();
    }
    m_workStealingEnabled = true;
    
    // Reset all tasks
    for (const auto& task : m_graph->getTasks()) {
        if (task) {
            task->reset();
        }
    }
    
    auto levels = m_graph->getLevels();
    if (levels.empty()) {
        std::cerr << "WARNING: No levels in render graph!" << std::endl;
        return;
    }
    
    // Process level by level (respecting dependencies)
    for (const auto& level : levels) {
        if (level.empty()) continue;
        
        // Collect tasks that are ready to execute in this level
        std::vector<std::shared_ptr<RenderTask>> readyTasks;
        for (const auto& task : level) {
            if (!task) continue;
            
            // Check if dependencies are ready
            bool depsReady = true;
            for (const auto& dep : task->getDependencies()) {
                if (auto depPtr = dep.lock()) {
                    if (!depPtr->isComplete()) {
                        depsReady = false;
                        break;
                    }
                }
            }
            
            if (depsReady) {
                readyTasks.push_back(task);
            }
        }
        
        if (readyTasks.empty()) continue;
        
        // Separate tasks by executor type
        std::vector<std::shared_ptr<RenderTask>> glTasks;
        std::vector<std::shared_ptr<RenderTask>> cudaTasks;
        std::vector<std::shared_ptr<RenderTask>> cpuTasks;
        
        for (const auto& task : readyTasks) {
            switch (task->getExecutor()) {
                case TaskExecutor::OpenGL_MainThread:
                    glTasks.push_back(task);
                    break;
                case TaskExecutor::CUDA_Stream:
                    cudaTasks.push_back(task);
                    break;
                case TaskExecutor::CPU_Worker:
                    cpuTasks.push_back(task);
                    break;
            }
        }
        
        // Phase 1: Execute GL tasks on main thread (sequential, but respects DAG)
        executeGLTasks(glTasks);
        
        // Phase 2: Launch CUDA tasks on parallel streams
        launchCUDATasks(cudaTasks);
        
        // Phase 3: Dispatch CPU tasks to worker threads (with work stealing)
        // Sort CPU tasks by cost for better work distribution
        std::sort(cpuTasks.begin(), cpuTasks.end(), 
            [](const std::shared_ptr<RenderTask>& a, const std::shared_ptr<RenderTask>& b) {
                return a->getCostEstimate() > b->getCostEstimate(); // Higher cost first
            });
        dispatchCPUTasks(cpuTasks);
        
        // Phase 4: Synchronize all (wait for CUDA and CPU tasks)
        synchronizeAll();
        
        // Wait for all tasks in this level to complete
        waitForTasksComplete(readyTasks);
    }
}

void DAGScheduler::executeFullSystem() {
    // Full System: DAG + Work Stealing + AI-driven adaptive resource management
    // The AI backend (via PipelineController) will adapt stream counts dynamically
    // Execute with work stealing - the AI adaptation happens in main loop via adaptStreamCount()
    executeDAGWorkStealing();
}

void DAGScheduler::setAIBackend(void* aiBackend) {
    m_aiBackend = aiBackend;
}

void DAGScheduler::adaptStreamCount(int recommendedStreams) {
    // Dynamically adjust CUDA stream count based on AI recommendations
    if (recommendedStreams != m_cudaStreamCount && recommendedStreams >= 2 && recommendedStreams <= 8) {
        std::cout << "AI adapting: Changing CUDA streams from " << m_cudaStreamCount 
                  << " to " << recommendedStreams << std::endl;
        initializeCudaStreams(recommendedStreams);
    }
}

void DAGScheduler::workerThread(int workerID) {
    while (m_running) {
        std::shared_ptr<RenderTask> task;
        bool gotTask = false;
        
        // Try to get task from own queue
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (!m_taskQueues[workerID].empty()) {
                task = m_taskQueues[workerID].front();
                m_taskQueues[workerID].pop();
                gotTask = true;
            }
        }
        
        // If no task and work stealing enabled, try to steal
        if (!gotTask && m_workStealingEnabled) {
            gotTask = tryStealTask(workerID, task);
        }
        
        // If still no task, wait
        if (!gotTask) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_workAvailable.wait(lock, [this, workerID]() {
                return !m_running || !m_taskQueues[workerID].empty();
            });
            continue;
        }
        
        // Execute task
        // NOTE: This is legacy worker thread code - not used in three-tier system
        // In the new system, GL tasks run on main thread, CPU tasks use cpuWorkerThread
        if (task && !task->isComplete()) {
            auto start = std::chrono::high_resolution_clock::now();
            
            // Legacy code - worker threads no longer use OpenGL contexts
            // All GL tasks are executed on main thread in the new system
            
            // Track state change
            GLState currentState = task->getGLState();
            if (m_lastWorkerState[workerID].shaderID != 0) {
                if (currentState.distance(m_lastWorkerState[workerID]) > 0.1f) {
                    m_workerStats[workerID].stateChanges++;
                }
            }
            m_lastWorkerState[workerID] = currentState;
            
            Profiler::instance().startProfile(task->getName());
            task->execute();
            task->markComplete();  // Mark task as complete after execution
            Profiler::instance().endProfile(task->getName());
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            m_workerStats[workerID].totalWorkTime += duration.count() / 1000.0f;
            m_workerStats[workerID].tasksExecuted++;
        }
    }
}

bool DAGScheduler::tryStealTask(int thiefID, std::shared_ptr<RenderTask>& outTask) {
    const float stealThreshold = 0.5f;
    
    // Check other workers' queues
    for (int victimID = 0; victimID < m_workerCount; ++victimID) {
        if (victimID == thiefID) continue;
        
        std::lock_guard<std::mutex> lock(m_queueMutex);
        
        if (m_taskQueues[victimID].empty()) continue;
        
        auto candidateTask = m_taskQueues[victimID].front();
        float stealCost = calculateStealCost(candidateTask, thiefID);
        
        // Check queue depth imbalance
        int thiefQueueSize = m_taskQueues[thiefID].size();
        int victimQueueSize = m_taskQueues[victimID].size();
        float queueImbalance = (float)(victimQueueSize - thiefQueueSize);
        
        // Benefit - cost
        float benefit = queueImbalance * 1.0f; // Weight for queue imbalance
        float totalCost = stealCost;
        
        if (benefit - totalCost > stealThreshold && victimQueueSize > thiefQueueSize) {
            m_taskQueues[victimID].pop();
            outTask = candidateTask;
            
            m_workerStats[thiefID].stealAttempts++;
            m_workerStats[thiefID].successfulSteals++;
            m_workerStats[victimID].stealAttempts++; // Victim lost a task
            
            return true;
        }
    }
    
    return false;
}

float DAGScheduler::calculateStealCost(std::shared_ptr<RenderTask> task, int thiefID) const {
    if (m_lastWorkerState[thiefID].shaderID == 0) {
        return 0.0f; // First task, no state change cost
    }
    
    GLState taskState = task->getGLState();
    float stateDistance = taskState.distance(m_lastWorkerState[thiefID]);
    
    // State affinity penalty
    return stateDistance * 0.5f; // Weight for state changes
}

void DAGScheduler::start() {
    if (m_running) return;
    
    m_running = true;
    m_workerThreads.clear();
    m_cpuWorkerThreads.clear();
    
    // Start CPU worker threads (no OpenGL contexts)
    for (int i = 0; i < m_workerCount; ++i) {
        m_cpuWorkerStats[i] = {};
        m_cpuWorkerThreads.emplace_back(&DAGScheduler::cpuWorkerThread, this, i);
    }
    
    // Legacy worker threads (not used in WSL mode)
    for (int i = 0; i < m_workerCount; ++i) {
        m_workerStats[i] = {};
        m_lastWorkerState[i] = {};
        m_workerThreads.emplace_back(&DAGScheduler::workerThread, this, i);
    }
}

float DAGScheduler::estimateGPUTime() const {
    // Estimate GPU time by summing up GPU-bound render passes
    // This is a fallback when timer queries aren't ready
    auto& profiler = Profiler::instance();
    auto& frameData = profiler.getFrameData();
    
    float gpuTime = 0.0f;
    // GPU-bound tasks: ShadowPass, GBufferPass, LightingPass, PostFXPass, CudaParticles
    const char* gpuTasks[] = {"ShadowPass", "GBufferPass", "LightingPass", "PostFXPass", "CudaParticles"};
    
    for (const auto& data : frameData) {
        for (const char* taskName : gpuTasks) {
            if (data.name == taskName) {
                gpuTime += data.duration;
                break;
            }
        }
    }
    
    return gpuTime;
}

void DAGScheduler::waitForTasksComplete(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    // Wait for specific tasks to complete with a timeout
    const int maxIterations = 10000;  // Max 1 second at 100us per iteration
    int iterations = 0;
    
    while (iterations < maxIterations) {
        // Check if all specified tasks are complete
        bool allComplete = true;
        for (const auto& task : tasks) {
            if (task && !task->isComplete()) {
                allComplete = false;
                break;
            }
        }
        
        if (allComplete) {
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        iterations++;
    }
    
    if (iterations >= maxIterations) {
        std::cerr << "WARNING: waitForTasksComplete() timed out waiting for tasks!" << std::endl;
    }
}

void DAGScheduler::setMainWindow(GLFWwindow* window) {
    m_mainWindow = window;
    // Note: We don't create shared contexts anymore - all GL on main thread
}

void DAGScheduler::initializeCudaStreams(int count) {
    destroyCudaStreams();  // Clean up existing streams
    
    m_cudaStreamCount = count;
    m_cudaStreams.resize(count, nullptr);
    
    // Check if CUDA is available
    int deviceCount = 0;
    cudaGetDeviceCount(&deviceCount);
    
    if (deviceCount > 0) {
        for (int i = 0; i < count; ++i) {
            cudaStream_t stream;
            cudaError_t err = cudaStreamCreate(&stream);
            if (err == cudaSuccess) {
                m_cudaStreams[i] = reinterpret_cast<void*>(stream);
            } else {
                std::cerr << "Warning: Failed to create CUDA stream " << i << ": " << cudaGetErrorString(err) << std::endl;
                m_cudaStreams[i] = nullptr;
            }
        }
        std::cout << "Initialized " << count << " CUDA streams" << std::endl;
    } else {
        std::cerr << "Warning: No CUDA devices available. CUDA streams disabled." << std::endl;
    }
}

void DAGScheduler::destroyCudaStreams() {
    for (auto* streamPtr : m_cudaStreams) {
        if (streamPtr) {
            cudaStream_t stream = reinterpret_cast<cudaStream_t>(streamPtr);
            cudaStreamDestroy(stream);
        }
    }
    m_cudaStreams.clear();
    m_cudaStreamCount = 0;
}

void DAGScheduler::executeGLTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    // Execute all GL tasks sequentially on main thread
    for (const auto& task : tasks) {
        if (!task) continue;
        
        Profiler::instance().startProfile(task->getName());
        task->execute();
        Profiler::instance().endProfile(task->getName());
    }
}

void DAGScheduler::launchCUDATasks(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    // Launch CUDA tasks on parallel streams (async - true parallelism!)
    int streamIdx = 0;
    for (const auto& task : tasks) {
        if (!task) continue;
        
        // Assign stream to task
        int streamIndex = streamIdx % m_cudaStreamCount;
        task->setCudaStream(streamIndex);
        
        if (m_cudaStreams[streamIndex]) {
            cudaStream_t stream = reinterpret_cast<cudaStream_t>(m_cudaStreams[streamIndex]);
            
            // Store stream pointer in task so it can be accessed during execution
            task->setCudaStreamPtr(m_cudaStreams[streamIndex]);
            
            // Launch task on CUDA stream (async - returns immediately)
            Profiler::instance().startProfile(task->getName());
            task->execute();  // Task will use the stream pointer internally
            Profiler::instance().endProfile(task->getName());
            
            streamIdx++;
        } else {
            // No CUDA stream available, execute on CPU
            Profiler::instance().startProfile(task->getName());
            task->execute();
            Profiler::instance().endProfile(task->getName());
        }
    }
}

void DAGScheduler::dispatchCPUTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    // Distribute CPU tasks intelligently based on cost estimates
    // Heavier tasks go to workers with less load (better load balancing)
    if (tasks.empty()) return;
    
    // Sort tasks by cost (heavier first) for better distribution
    std::vector<std::shared_ptr<RenderTask>> sortedTasks = tasks;
    std::sort(sortedTasks.begin(), sortedTasks.end(),
        [](const std::shared_ptr<RenderTask>& a, const std::shared_ptr<RenderTask>& b) {
            return a->getCostEstimate() > b->getCostEstimate();
        });
    
    {
        std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
        
        // Calculate current load per worker (queue size + work time)
        std::vector<float> workerLoad(m_cpuTaskQueues.size(), 0.0f);
        for (size_t i = 0; i < m_cpuTaskQueues.size(); ++i) {
            workerLoad[i] = static_cast<float>(m_cpuTaskQueues[i].size());
            if (i < m_cpuWorkerStats.size()) {
                workerLoad[i] += m_cpuWorkerStats[i].totalWorkTime * 0.1f;  // Weight recent work
            }
        }
        
        // Distribute tasks to least loaded workers
        for (const auto& task : sortedTasks) {
            if (!task) continue;
            
            // Find worker with minimum load
            int bestWorker = 0;
            float minLoad = workerLoad[0];
            for (size_t i = 1; i < workerLoad.size(); ++i) {
                if (workerLoad[i] < minLoad) {
                    minLoad = workerLoad[i];
                    bestWorker = i;
                }
            }
            
            // Assign task to least loaded worker
            m_cpuTaskQueues[bestWorker].push(task);
            workerLoad[bestWorker] += task->getCostEstimate();  // Update load estimate
        }
    }
    
    // Notify CPU workers
    m_cpuWorkAvailable.notify_all();
}

void DAGScheduler::synchronizeAll() {
    // Wait for all CUDA streams to complete
    for (auto* streamPtr : m_cudaStreams) {
        if (streamPtr) {
            cudaStream_t stream = reinterpret_cast<cudaStream_t>(streamPtr);
            cudaStreamSynchronize(stream);
        }
    }
    
    // Wait for all CPU tasks to complete
    while (true) {
        bool allQueuesEmpty = true;
        {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            for (const auto& queue : m_cpuTaskQueues) {
                if (!queue.empty()) {
                    allQueuesEmpty = false;
                    break;
                }
            }
        }
        
        if (allQueuesEmpty) {
            // Check if all CPU tasks are complete
            bool allComplete = true;
            if (m_graph) {
                for (const auto& task : m_graph->getTasks()) {
                    if (task && task->getExecutor() == TaskExecutor::CPU_Worker && !task->isComplete()) {
                        allComplete = false;
                        break;
                    }
                }
            }
            if (allComplete) break;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void DAGScheduler::cpuWorkerThread(int workerID) {
    // CPU worker thread - NO OpenGL calls, only CPU-side work
    while (m_running) {
        std::shared_ptr<RenderTask> task;
        bool gotTask = false;
        
        // Try to get task from own queue
        {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            if (workerID < m_cpuTaskQueues.size() && !m_cpuTaskQueues[workerID].empty()) {
                task = m_cpuTaskQueues[workerID].front();
                m_cpuTaskQueues[workerID].pop();
                gotTask = true;
            }
        }
        
        // Work stealing: Always check for load imbalance and steal proactively
        // This ensures idle/underloaded workers help overloaded ones
        if (!gotTask && m_workStealingEnabled) {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            
            // Calculate load imbalance
            std::vector<int> queueSizes;
            for (const auto& queue : m_cpuTaskQueues) {
                queueSizes.push_back(queue.size());
            }
            
            int myQueueSize = workerID < queueSizes.size() ? queueSizes[workerID] : 0;
            
            // Find the most overloaded worker to steal from
            int bestVictim = -1;
            int maxImbalance = 0;
            
            for (int victimID = 0; victimID < m_cpuTaskQueues.size(); ++victimID) {
                if (victimID == workerID) continue;
                
                int victimQueueSize = victimID < queueSizes.size() ? queueSizes[victimID] : 0;
                int imbalance = victimQueueSize - myQueueSize;
                
                // Find worker with most tasks (aggressive stealing)
                if (victimQueueSize > myQueueSize && !m_cpuTaskQueues[victimID].empty() && imbalance > maxImbalance) {
                    maxImbalance = imbalance;
                    bestVictim = victimID;
                }
            }
            
            // Steal from the most overloaded worker
            if (bestVictim >= 0) {
                task = m_cpuTaskQueues[bestVictim].front();
                m_cpuTaskQueues[bestVictim].pop();
                gotTask = true;
                if (workerID < m_cpuWorkerStats.size()) {
                    m_cpuWorkerStats[workerID].stealAttempts++;
                    m_cpuWorkerStats[workerID].successfulSteals++;
                }
            }
        }
        
        // If no task, wait
        if (!gotTask) {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            m_cpuWorkAvailable.wait(lock, [this, workerID]() {
                if (!m_running) return true;
                if (workerID < m_cpuTaskQueues.size() && !m_cpuTaskQueues[workerID].empty()) return true;
                // Also check if any other queue has work (for stealing)
                if (m_workStealingEnabled) {
                    for (int i = 0; i < m_cpuTaskQueues.size(); ++i) {
                        if (i != workerID && !m_cpuTaskQueues[i].empty()) return true;
                    }
                }
                return false;
            });
            continue;
        }
        
        // Execute task (CPU-side work only, no OpenGL)
        if (task && !task->isComplete()) {
            auto start = std::chrono::high_resolution_clock::now();
            
            Profiler::instance().startProfile(task->getName());
            task->execute();
            task->markComplete();
            Profiler::instance().endProfile(task->getName());
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            if (workerID < m_cpuWorkerStats.size()) {
                m_cpuWorkerStats[workerID].totalWorkTime += duration.count() / 1000.0f;
                m_cpuWorkerStats[workerID].tasksExecuted++;
            }
        }
    }
}

