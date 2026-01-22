#include "RenderGraph.hpp"
#include "../Core/Logger.hpp"
#include <algorithm>
#include <set>

namespace GameEngine {

    RenderGraph::RenderGraph() {
    }

    RenderPass& RenderGraph::AddPass(const std::string& name) {
        auto pass = CreateScope<RenderPass>(name);
        auto* passPtr = pass.get();
        m_Passes[name] = std::move(pass);
        m_Dirty = true;
        return *passPtr;
    }

    RenderPass* RenderGraph::GetPass(const std::string& name) {
        auto it = m_Passes.find(name);
        if (it != m_Passes.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    void RenderGraph::Execute() {
        if (m_Dirty) {
            TopologicalSort();
            m_Dirty = false;
        }

        for (auto* pass : m_ExecutionOrder) {
            if (pass) {
                pass->Execute();
            }
        }
    }

    void RenderGraph::Clear() {
        m_Passes.clear();
        m_ExecutionOrder.clear();
        m_Dirty = true;
    }

    void RenderGraph::TopologicalSort() {
        m_ExecutionOrder.clear();

        // Build dependency graph
        std::unordered_map<std::string, std::vector<std::string>> adjList;
        std::unordered_map<std::string, int> inDegree;

        // Initialize in-degrees
        for (const auto& [name, pass] : m_Passes) {
            inDegree[name] = 0;
            adjList[name] = {};
        }

        // Build adjacency list and calculate in-degrees
        for (const auto& [name, pass] : m_Passes) {
            for (const auto& dep : pass->GetDependencies()) {
                adjList[dep].push_back(name);
                inDegree[name]++;
            }
        }

        // Kahn's algorithm for topological sort
        std::vector<std::string> queue;
        for (const auto& [name, degree] : inDegree) {
            if (degree == 0) {
                queue.push_back(name);
            }
        }

        while (!queue.empty()) {
            std::string current = queue.front();
            queue.erase(queue.begin());

            auto* pass = GetPass(current);
            if (pass) {
                m_ExecutionOrder.push_back(pass);
            }

            for (const auto& neighbor : adjList[current]) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    queue.push_back(neighbor);
                }
            }
        }

        // Check for cycles
        if (m_ExecutionOrder.size() != m_Passes.size()) {
            GE_CORE_ERROR("Render graph contains cycles!");
            m_ExecutionOrder.clear();
            // Add all passes in arbitrary order as fallback
            for (const auto& [name, pass] : m_Passes) {
                m_ExecutionOrder.push_back(pass.get());
            }
        }
    }

}

