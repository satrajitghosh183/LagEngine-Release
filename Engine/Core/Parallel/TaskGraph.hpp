#pragma once

#include "../JobSystem.hpp"
#include <functional>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <mutex>

namespace GameEngine {

    /**
     * @brief DAG-based task graph for complex dependency chains
     *
     * Build a graph of tasks with explicit dependencies, then execute
     * the entire graph with maximum parallelism. Tasks with no
     * unsatisfied dependencies run concurrently.
     *
     * Usage:
     *   TaskGraph graph;
     *   auto a = graph.AddTask("LoadMeshes", [&]{ ... });
     *   auto b = graph.AddTask("LoadTextures", [&]{ ... });
     *   auto c = graph.AddTask("BuildScene", [&]{ ... });
     *   graph.AddDependency(c, a);  // c depends on a
     *   graph.AddDependency(c, b);  // c depends on b
     *   graph.Execute();  // a and b run in parallel, c runs after both
     */
    class TaskGraph {
    public:
        using TaskID = uint32_t;
        using TaskFunc = std::function<void()>;

        static constexpr TaskID INVALID_TASK = UINT32_MAX;

        TaskID AddTask(const std::string& name, TaskFunc func) {
            TaskID id = static_cast<TaskID>(m_Tasks.size());
            m_Tasks.push_back({name, std::move(func), {}, {}, 0});
            m_NameToId[name] = id;
            return id;
        }

        TaskID FindTask(const std::string& name) const {
            auto it = m_NameToId.find(name);
            return it != m_NameToId.end() ? it->second : INVALID_TASK;
        }

        void AddDependency(TaskID task, TaskID dependsOn) {
            m_Tasks[task].Dependencies.push_back(dependsOn);
            m_Tasks[dependsOn].Dependents.push_back(task);
        }

        void AddDependency(const std::string& task, const std::string& dependsOn) {
            AddDependency(FindTask(task), FindTask(dependsOn));
        }

        /**
         * @brief Execute the task graph with maximum parallelism
         * Blocks until all tasks complete.
         */
        void Execute() {
            if (m_Tasks.empty()) return;

            // Initialize in-degree (number of unsatisfied dependencies)
            std::vector<std::atomic<int32_t>> inDegree(m_Tasks.size());
            for (size_t i = 0; i < m_Tasks.size(); i++) {
                inDegree[i].store(static_cast<int32_t>(m_Tasks[i].Dependencies.size()),
                                  std::memory_order_relaxed);
            }

            // Track completion
            std::atomic<uint32_t> completed(0);
            std::mutex completionMutex;
            std::condition_variable completionCV;

            // Recursive lambda to schedule a task and its dependents
            std::function<void(TaskID)> scheduleTask;
            scheduleTask = [&](TaskID id) {
                JobSystem::Enqueue([&, id]() {
                    // Execute the task
                    if (m_Tasks[id].Func) {
                        m_Tasks[id].Func();
                    }

                    // Notify dependents
                    for (TaskID dep : m_Tasks[id].Dependents) {
                        int32_t remaining = inDegree[dep].fetch_sub(1, std::memory_order_acq_rel) - 1;
                        if (remaining == 0) {
                            scheduleTask(dep);
                        }
                    }

                    uint32_t done = completed.fetch_add(1, std::memory_order_acq_rel) + 1;
                    if (done == static_cast<uint32_t>(m_Tasks.size())) {
                        std::lock_guard<std::mutex> lock(completionMutex);
                        completionCV.notify_all();
                    }
                });
            };

            // Enqueue all root tasks (no dependencies)
            for (size_t i = 0; i < m_Tasks.size(); i++) {
                if (inDegree[i].load(std::memory_order_relaxed) == 0) {
                    scheduleTask(static_cast<TaskID>(i));
                }
            }

            // Wait for all tasks to complete
            std::unique_lock<std::mutex> lock(completionMutex);
            completionCV.wait(lock, [&]() {
                return completed.load(std::memory_order_acquire) == static_cast<uint32_t>(m_Tasks.size());
            });
        }

        void Clear() {
            m_Tasks.clear();
            m_NameToId.clear();
        }

        size_t GetTaskCount() const { return m_Tasks.size(); }

    private:
        struct Task {
            std::string Name;
            TaskFunc Func;
            std::vector<TaskID> Dependencies;
            std::vector<TaskID> Dependents;
            int32_t InDegree = 0;
        };

        std::vector<Task> m_Tasks;
        std::unordered_map<std::string, TaskID> m_NameToId;
    };

} // namespace GameEngine
