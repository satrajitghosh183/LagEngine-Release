#include "RenderGraph.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <functional>

RenderGraph::RenderGraph() {
}

std::shared_ptr<RenderTask> RenderGraph::addTask(TaskType type, const std::string& name, float costEstimate, TaskExecutor executor) {
    auto task = std::make_shared<RenderTask>(type, name, costEstimate, executor);
    m_tasks.push_back(task);
    return task;
}

void RenderGraph::addDependency(std::shared_ptr<RenderTask> from, std::shared_ptr<RenderTask> to) {
    to->addDependency(from);
}

bool RenderGraph::build() {
    // Check for cycles
    if (hasCycle()) {
        return false;
    }
    
    // Topological sort into levels
    topologicalSort();
    
    return true;
}

bool RenderGraph::hasCycle() const {
    // Simple DFS cycle detection
    std::unordered_map<std::shared_ptr<RenderTask>, int> visited;
    
    std::function<bool(std::shared_ptr<RenderTask>)> dfs = [&](std::shared_ptr<RenderTask> task) -> bool {
        visited[task] = 1; // Visiting
        
        for (const auto& dep : task->getDependencies()) {
            if (auto depPtr = dep.lock()) {
                if (visited[depPtr] == 1) {
                    return true; // Cycle detected
                }
                if (visited[depPtr] == 0 && dfs(depPtr)) {
                    return true;
                }
            }
        }
        
        visited[task] = 2; // Visited
        return false;
    };
    
    for (const auto& task : m_tasks) {
        visited[task] = 0; // Unvisited
    }
    
    for (const auto& task : m_tasks) {
        if (visited[task] == 0 && dfs(task)) {
            return true;
        }
    }
    
    return false;
}

void RenderGraph::topologicalSort() {
    m_levels.clear();
    
    // Compute in-degrees
    std::unordered_map<std::shared_ptr<RenderTask>, int> inDegree;
    for (const auto& task : m_tasks) {
        inDegree[task] = 0;
    }
    
    for (const auto& task : m_tasks) {
        for (const auto& dep : task->getDependencies()) {
            if (auto depPtr = dep.lock()) {
                inDegree[task]++;
            }
        }
    }
    
    // Level-by-level topological sort
    std::queue<std::shared_ptr<RenderTask>> queue;
    
    // Find all tasks with in-degree 0 (first level)
    for (const auto& task : m_tasks) {
        if (inDegree[task] == 0) {
            queue.push(task);
        }
    }
    
    // Build dependency map for faster lookup
    std::unordered_map<std::shared_ptr<RenderTask>, std::vector<std::shared_ptr<RenderTask>>> dependents;
    for (const auto& task : m_tasks) {
        for (const auto& dep : task->getDependencies()) {
            if (auto depPtr = dep.lock()) {
                dependents[depPtr].push_back(task);
            }
        }
    }
    
    while (!queue.empty()) {
        std::vector<std::shared_ptr<RenderTask>> currentLevel;
        size_t levelSize = queue.size();
        
        // Process all tasks at current level
        for (size_t i = 0; i < levelSize; ++i) {
            auto task = queue.front();
            queue.pop();
            currentLevel.push_back(task);
            
            // Find dependent tasks
            auto it = dependents.find(task);
            if (it != dependents.end()) {
                for (const auto& dependent : it->second) {
                    inDegree[dependent]--;
                    if (inDegree[dependent] == 0) {
                        queue.push(dependent);
                    }
                }
            }
        }
        
        m_levels.push_back(currentLevel);
    }
}

void RenderGraph::resetFrame() {
    for (auto& task : m_tasks) {
        task->reset();
    }
}

std::shared_ptr<RenderTask> RenderGraph::getTask(const std::string& name) const {
    for (const auto& task : m_tasks) {
        if (task->getName() == name) {
            return task;
        }
    }
    return nullptr;
}

