#pragma once

#include "RenderPass.hpp"
#include "../Core/Base.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace GameEngine {

    /**
     * @brief Render graph - manages render passes and their dependencies
     */
    class RenderGraph {
    public:
        RenderGraph();
        ~RenderGraph() = default;

        /**
         * @brief Add a render pass
         */
        RenderPass& AddPass(const std::string& name);

        /**
         * @brief Get pass by name
         */
        RenderPass* GetPass(const std::string& name);

        /**
         * @brief Execute all passes in dependency order
         */
        void Execute();

        /**
         * @brief Clear all passes
         */
        void Clear();

        /**
         * @brief Get all passes in execution order
         */
        const std::vector<RenderPass*>& GetExecutionOrder() const { return m_ExecutionOrder; }

    private:
        void TopologicalSort();

        std::unordered_map<std::string, Scope<RenderPass>> m_Passes;
        std::vector<RenderPass*> m_ExecutionOrder;
        bool m_Dirty = true;
    };

}

