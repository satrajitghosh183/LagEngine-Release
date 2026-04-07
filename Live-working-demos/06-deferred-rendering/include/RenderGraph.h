#pragma once

#include "RenderTask.h"
#include <vector>
#include <memory>

class RenderGraph {
public:
    RenderGraph();
    
    std::shared_ptr<RenderTask> addTask(TaskType type, const std::string& name, float costEstimate = 1.0f, TaskExecutor executor = TaskExecutor::OpenGL_MainThread);
    void addDependency(std::shared_ptr<RenderTask> from, std::shared_ptr<RenderTask> to);
    
    // Build dependency graph and validate
    bool build();
    
    // Topological sort into levels
    std::vector<std::vector<std::shared_ptr<RenderTask>>> getLevels() const { return m_levels; }
    
    // Get all tasks
    const std::vector<std::shared_ptr<RenderTask>>& getTasks() const { return m_tasks; }
    
    // Reset all tasks for next frame
    void resetFrame();
    
    // Get task by name
    std::shared_ptr<RenderTask> getTask(const std::string& name) const;

private:
    std::vector<std::shared_ptr<RenderTask>> m_tasks;
    std::vector<std::vector<std::shared_ptr<RenderTask>>> m_levels;
    
    bool hasCycle() const;
    void topologicalSort();
};

