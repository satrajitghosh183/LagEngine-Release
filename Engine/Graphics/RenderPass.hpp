#pragma once

#include "RenderResource.hpp"
#include <functional>
#include <string>
#include <vector>

namespace GameEngine {

    /**
     * @brief Render pass in the render graph
     */
    class RenderPass {
    public:
        RenderPass(const std::string& name);
        ~RenderPass() = default;

        /**
         * @brief Set render target
         */
        RenderPass& SetRenderTarget(TextureHandle target);
        RenderPass& SetRenderTarget(RenderTargetHandle target);

        /**
         * @brief Add dependency on another pass
         */
        RenderPass& AddDependency(const std::string& passName);

        /**
         * @brief Set execute function
         */
        RenderPass& SetExecuteFunc(std::function<void()> func);

        /**
         * @brief Execute the pass
         */
        void Execute() const;

        /**
         * @brief Get pass name
         */
        const std::string& GetName() const { return m_Name; }

        /**
         * @brief Get dependencies
         */
        const std::vector<std::string>& GetDependencies() const { return m_Dependencies; }

        /**
         * @brief Get render target
         */
        TextureHandle GetRenderTarget() const { return m_RenderTarget; }
        RenderTargetHandle GetRenderTargetHandle() const { return m_RenderTargetHandle; }

    private:
        std::string m_Name;
        TextureHandle m_RenderTarget = INVALID_TEXTURE_HANDLE;
        RenderTargetHandle m_RenderTargetHandle = INVALID_RENDER_TARGET_HANDLE;
        std::vector<std::string> m_Dependencies;
        std::function<void()> m_ExecuteFunc;
    };

}

