#include "DAGScheduler.h"
#include "Profiler.h"
#include "RenderGraph.h"
#define GLFW_INCLUDE_VULKAN
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
    initializeCudaStreams(4);
}

DAGScheduler::~DAGScheduler() {
    stop();
    destroyCudaStreams();
}

void DAGScheduler::setWorkerThreadCount(int count) {
    stop();

    m_workerCount = count;

    m_cpuTaskQueues.clear();
    m_cpuTaskQueues.resize(count);
    m_cpuWorkerStats.clear();
    m_cpuWorkerStats.resize(count);

    for (auto& stats : m_cpuWorkerStats) {
        stats = WorkerStats();
    }

    m_taskQueues.clear();
    m_taskQueues.resize(count);
    m_workerStats.clear();
    m_workerStats.resize(count);
    m_lastWorkerState.clear();
    m_lastWorkerState.resize(count);

    for (auto& stats : m_workerStats) {
        stats = WorkerStats();
    }
}

void DAGScheduler::stop() {
    if (m_running) {
        m_running = false;
        m_workAvailable.notify_all();
        m_cpuWorkAvailable.notify_all();

        for (auto& thread : m_cpuWorkerThreads) {
            if (thread.joinable()) thread.join();
        }
        m_cpuWorkerThreads.clear();

        for (auto& thread : m_workerThreads) {
            if (thread.joinable()) thread.join();
        }
        m_workerThreads.clear();
    }
}

void DAGScheduler::executeFrame() {
    auto start = std::chrono::high_resolution_clock::now();

    m_graph->resetFrame();

    Profiler::instance().startProfile("Frame");

    // In Vulkan, GPU timing is handled by the application via timestamp queries.
    // For now, estimate GPU time from profiler data.

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
            executeFullSystem();
            break;
    }

    Profiler::instance().endProfile("Frame");

    // Estimate GPU time from profiler data
    m_gpuTime = estimateGPUTime();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    m_frameTime = duration.count() / 1000.0f;
    m_cpuTime = std::max(0.0f, m_frameTime - m_gpuTime);
}

void DAGScheduler::executeBaseline() {
    if (!m_graph) {
        std::cerr << "ERROR: RenderGraph is null!" << std::endl;
        return;
    }

    auto& tasks = m_graph->getTasks();
    if (tasks.empty()) {
        std::cerr << "WARNING: No tasks in render graph!" << std::endl;
        return;
    }

    std::vector<std::shared_ptr<RenderTask>> sortedTasks;
    std::unordered_map<std::shared_ptr<RenderTask>, bool> visited;

    std::function<void(std::shared_ptr<RenderTask>)> visit = [&](std::shared_ptr<RenderTask> task) {
        if (!task || visited[task]) return;
        for (const auto& dep : task->getDependencies()) {
            if (auto depPtr = dep.lock()) visit(depPtr);
        }
        visited[task] = true;
        sortedTasks.push_back(task);
    };

    for (const auto& task : tasks) {
        if (task) visit(task);
    }

    for (auto& task : sortedTasks) {
        if (task) {
            Profiler::instance().startProfile(task->getName());
            task->execute();
            Profiler::instance().endProfile(task->getName());
        }
    }
}

void DAGScheduler::executeDAGOnly() {
    if (!m_graph) return;

    if (!m_running) start();

    for (const auto& task : m_graph->getTasks()) {
        if (task) task->reset();
    }

    auto levels = m_graph->getLevels();
    if (levels.empty()) return;

    for (const auto& level : levels) {
        if (level.empty()) continue;

        std::vector<std::shared_ptr<RenderTask>> readyTasks;
        for (const auto& task : level) {
            if (!task) continue;
            bool depsReady = true;
            for (const auto& dep : task->getDependencies()) {
                if (auto depPtr = dep.lock()) {
                    if (!depPtr->isComplete()) { depsReady = false; break; }
                }
            }
            if (depsReady) readyTasks.push_back(task);
        }
        if (readyTasks.empty()) continue;

        std::vector<std::shared_ptr<RenderTask>> glTasks, cudaTasks, cpuTasks;
        for (const auto& task : readyTasks) {
            switch (task->getExecutor()) {
                case TaskExecutor::OpenGL_MainThread: glTasks.push_back(task); break;
                case TaskExecutor::CUDA_Stream:       cudaTasks.push_back(task); break;
                case TaskExecutor::CPU_Worker:        cpuTasks.push_back(task); break;
            }
        }

        executeGLTasks(glTasks);
        launchCUDATasks(cudaTasks);
        dispatchCPUTasks(cpuTasks);
        synchronizeAll();
        waitForTasksComplete(readyTasks);
    }
}

void DAGScheduler::executeDAGWorkStealing() {
    if (!m_graph) return;
    if (!m_running) start();
    m_workStealingEnabled = true;

    for (const auto& task : m_graph->getTasks()) {
        if (task) task->reset();
    }

    auto levels = m_graph->getLevels();
    if (levels.empty()) return;

    for (const auto& level : levels) {
        if (level.empty()) continue;

        std::vector<std::shared_ptr<RenderTask>> readyTasks;
        for (const auto& task : level) {
            if (!task) continue;
            bool depsReady = true;
            for (const auto& dep : task->getDependencies()) {
                if (auto depPtr = dep.lock()) {
                    if (!depPtr->isComplete()) { depsReady = false; break; }
                }
            }
            if (depsReady) readyTasks.push_back(task);
        }
        if (readyTasks.empty()) continue;

        std::vector<std::shared_ptr<RenderTask>> glTasks, cudaTasks, cpuTasks;
        for (const auto& task : readyTasks) {
            switch (task->getExecutor()) {
                case TaskExecutor::OpenGL_MainThread: glTasks.push_back(task); break;
                case TaskExecutor::CUDA_Stream:       cudaTasks.push_back(task); break;
                case TaskExecutor::CPU_Worker:        cpuTasks.push_back(task); break;
            }
        }

        executeGLTasks(glTasks);
        launchCUDATasks(cudaTasks);

        std::sort(cpuTasks.begin(), cpuTasks.end(),
            [](const std::shared_ptr<RenderTask>& a, const std::shared_ptr<RenderTask>& b) {
                return a->getCostEstimate() > b->getCostEstimate();
            });
        dispatchCPUTasks(cpuTasks);
        synchronizeAll();
        waitForTasksComplete(readyTasks);
    }
}

void DAGScheduler::executeFullSystem() {
    executeDAGWorkStealing();
}

void DAGScheduler::setAIBackend(void* aiBackend) {
    m_aiBackend = aiBackend;
}

void DAGScheduler::adaptStreamCount(int recommendedStreams) {
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

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            if (!m_taskQueues[workerID].empty()) {
                task = m_taskQueues[workerID].front();
                m_taskQueues[workerID].pop();
                gotTask = true;
            }
        }

        if (!gotTask && m_workStealingEnabled) {
            gotTask = tryStealTask(workerID, task);
        }

        if (!gotTask) {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_workAvailable.wait(lock, [this, workerID]() {
                return !m_running || !m_taskQueues[workerID].empty();
            });
            continue;
        }

        if (task && !task->isComplete()) {
            auto start = std::chrono::high_resolution_clock::now();

            GLState currentState = task->getGLState();
            if (m_lastWorkerState[workerID].shaderID != 0) {
                if (currentState.distance(m_lastWorkerState[workerID]) > 0.1f) {
                    m_workerStats[workerID].stateChanges++;
                }
            }
            m_lastWorkerState[workerID] = currentState;

            Profiler::instance().startProfile(task->getName());
            task->execute();
            task->markComplete();
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

    for (int victimID = 0; victimID < m_workerCount; ++victimID) {
        if (victimID == thiefID) continue;

        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (m_taskQueues[victimID].empty()) continue;

        auto candidateTask = m_taskQueues[victimID].front();
        float stealCost = calculateStealCost(candidateTask, thiefID);

        int thiefQueueSize = static_cast<int>(m_taskQueues[thiefID].size());
        int victimQueueSize = static_cast<int>(m_taskQueues[victimID].size());
        float queueImbalance = (float)(victimQueueSize - thiefQueueSize);

        float benefit = queueImbalance * 1.0f;
        float totalCost = stealCost;

        if (benefit - totalCost > stealThreshold && victimQueueSize > thiefQueueSize) {
            m_taskQueues[victimID].pop();
            outTask = candidateTask;
            m_workerStats[thiefID].stealAttempts++;
            m_workerStats[thiefID].successfulSteals++;
            m_workerStats[victimID].stealAttempts++;
            return true;
        }
    }
    return false;
}

float DAGScheduler::calculateStealCost(std::shared_ptr<RenderTask> task, int thiefID) const {
    if (m_lastWorkerState[thiefID].shaderID == 0) return 0.0f;
    GLState taskState = task->getGLState();
    float stateDistance = taskState.distance(m_lastWorkerState[thiefID]);
    return stateDistance * 0.5f;
}

void DAGScheduler::start() {
    if (m_running) return;
    m_running = true;
    m_workerThreads.clear();
    m_cpuWorkerThreads.clear();

    for (int i = 0; i < m_workerCount; ++i) {
        m_cpuWorkerStats[i] = {};
        m_cpuWorkerThreads.emplace_back(&DAGScheduler::cpuWorkerThread, this, i);
    }

    for (int i = 0; i < m_workerCount; ++i) {
        m_workerStats[i] = {};
        m_lastWorkerState[i] = {};
        m_workerThreads.emplace_back(&DAGScheduler::workerThread, this, i);
    }
}

float DAGScheduler::estimateGPUTime() const {
    auto& profiler = Profiler::instance();
    auto& frameData = profiler.getFrameData();

    float gpuTime = 0.0f;
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
    const int maxIterations = 10000;
    int iterations = 0;

    while (iterations < maxIterations) {
        bool allComplete = true;
        for (const auto& task : tasks) {
            if (task && !task->isComplete()) { allComplete = false; break; }
        }
        if (allComplete) break;
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        iterations++;
    }

    if (iterations >= maxIterations) {
        std::cerr << "WARNING: waitForTasksComplete() timed out!" << std::endl;
    }
}

void DAGScheduler::setMainWindow(GLFWwindow* window) {
    m_mainWindow = window;
}

void DAGScheduler::initializeCudaStreams(int count) {
    destroyCudaStreams();

    m_cudaStreamCount = count;
    m_cudaStreams.resize(count, nullptr);

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
        std::cerr << "Warning: No CUDA devices available." << std::endl;
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
    for (const auto& task : tasks) {
        if (!task) continue;
        Profiler::instance().startProfile(task->getName());
        task->execute();
        Profiler::instance().endProfile(task->getName());
    }
}

void DAGScheduler::launchCUDATasks(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    int streamIdx = 0;
    for (const auto& task : tasks) {
        if (!task) continue;

        int streamIndex = streamIdx % m_cudaStreamCount;
        task->setCudaStream(streamIndex);

        if (streamIndex < static_cast<int>(m_cudaStreams.size()) && m_cudaStreams[streamIndex]) {
            task->setCudaStreamPtr(m_cudaStreams[streamIndex]);
            Profiler::instance().startProfile(task->getName());
            task->execute();
            Profiler::instance().endProfile(task->getName());
            streamIdx++;
        } else {
            Profiler::instance().startProfile(task->getName());
            task->execute();
            Profiler::instance().endProfile(task->getName());
        }
    }
}

void DAGScheduler::dispatchCPUTasks(const std::vector<std::shared_ptr<RenderTask>>& tasks) {
    if (tasks.empty()) return;

    std::vector<std::shared_ptr<RenderTask>> sortedTasks = tasks;
    std::sort(sortedTasks.begin(), sortedTasks.end(),
        [](const std::shared_ptr<RenderTask>& a, const std::shared_ptr<RenderTask>& b) {
            return a->getCostEstimate() > b->getCostEstimate();
        });

    {
        std::unique_lock<std::mutex> lock(m_cpuQueueMutex);

        std::vector<float> workerLoad(m_cpuTaskQueues.size(), 0.0f);
        for (size_t i = 0; i < m_cpuTaskQueues.size(); ++i) {
            workerLoad[i] = static_cast<float>(m_cpuTaskQueues[i].size());
            if (i < m_cpuWorkerStats.size()) {
                workerLoad[i] += m_cpuWorkerStats[i].totalWorkTime * 0.1f;
            }
        }

        for (const auto& task : sortedTasks) {
            if (!task) continue;
            int bestWorker = 0;
            float minLoad = workerLoad[0];
            for (size_t i = 1; i < workerLoad.size(); ++i) {
                if (workerLoad[i] < minLoad) { minLoad = workerLoad[i]; bestWorker = static_cast<int>(i); }
            }
            m_cpuTaskQueues[bestWorker].push(task);
            workerLoad[bestWorker] += task->getCostEstimate();
        }
    }

    m_cpuWorkAvailable.notify_all();
}

void DAGScheduler::synchronizeAll() {
    for (auto* streamPtr : m_cudaStreams) {
        if (streamPtr) {
            cudaStream_t stream = reinterpret_cast<cudaStream_t>(streamPtr);
            cudaStreamSynchronize(stream);
        }
    }

    while (true) {
        bool allQueuesEmpty = true;
        {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            for (const auto& queue : m_cpuTaskQueues) {
                if (!queue.empty()) { allQueuesEmpty = false; break; }
            }
        }

        if (allQueuesEmpty) {
            bool allComplete = true;
            if (m_graph) {
                for (const auto& task : m_graph->getTasks()) {
                    if (task && task->getExecutor() == TaskExecutor::CPU_Worker && !task->isComplete()) {
                        allComplete = false; break;
                    }
                }
            }
            if (allComplete) break;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void DAGScheduler::cpuWorkerThread(int workerID) {
    while (m_running) {
        std::shared_ptr<RenderTask> task;
        bool gotTask = false;

        {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            if (workerID < static_cast<int>(m_cpuTaskQueues.size()) && !m_cpuTaskQueues[workerID].empty()) {
                task = m_cpuTaskQueues[workerID].front();
                m_cpuTaskQueues[workerID].pop();
                gotTask = true;
            }
        }

        if (!gotTask && m_workStealingEnabled) {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);

            int bestVictim = -1;
            int maxImbalance = 0;
            int myQueueSize = workerID < static_cast<int>(m_cpuTaskQueues.size())
                ? static_cast<int>(m_cpuTaskQueues[workerID].size()) : 0;

            for (int victimID = 0; victimID < static_cast<int>(m_cpuTaskQueues.size()); ++victimID) {
                if (victimID == workerID) continue;
                int victimQueueSize = static_cast<int>(m_cpuTaskQueues[victimID].size());
                int imbalance = victimQueueSize - myQueueSize;
                if (victimQueueSize > myQueueSize && !m_cpuTaskQueues[victimID].empty() && imbalance > maxImbalance) {
                    maxImbalance = imbalance;
                    bestVictim = victimID;
                }
            }

            if (bestVictim >= 0) {
                task = m_cpuTaskQueues[bestVictim].front();
                m_cpuTaskQueues[bestVictim].pop();
                gotTask = true;
                if (workerID < static_cast<int>(m_cpuWorkerStats.size())) {
                    m_cpuWorkerStats[workerID].stealAttempts++;
                    m_cpuWorkerStats[workerID].successfulSteals++;
                }
            }
        }

        if (!gotTask) {
            std::unique_lock<std::mutex> lock(m_cpuQueueMutex);
            m_cpuWorkAvailable.wait(lock, [this, workerID]() {
                if (!m_running) return true;
                if (workerID < static_cast<int>(m_cpuTaskQueues.size()) && !m_cpuTaskQueues[workerID].empty()) return true;
                if (m_workStealingEnabled) {
                    for (int i = 0; i < static_cast<int>(m_cpuTaskQueues.size()); ++i) {
                        if (i != workerID && !m_cpuTaskQueues[i].empty()) return true;
                    }
                }
                return false;
            });
            continue;
        }

        if (task && !task->isComplete()) {
            auto start = std::chrono::high_resolution_clock::now();
            Profiler::instance().startProfile(task->getName());
            task->execute();
            task->markComplete();
            Profiler::instance().endProfile(task->getName());
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            if (workerID < static_cast<int>(m_cpuWorkerStats.size())) {
                m_cpuWorkerStats[workerID].totalWorkTime += duration.count() / 1000.0f;
                m_cpuWorkerStats[workerID].tasksExecuted++;
            }
        }
    }
}
