#include "RenderPass.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    RenderPass::RenderPass(const std::string& name)
        : m_Name(name) {
    }

    RenderPass& RenderPass::SetRenderTarget(TextureHandle target) {
        m_RenderTarget = target;
        m_RenderTargetHandle = INVALID_RENDER_TARGET_HANDLE;
        return *this;
    }

    RenderPass& RenderPass::SetRenderTarget(RenderTargetHandle target) {
        m_RenderTargetHandle = target;
        m_RenderTarget = INVALID_TEXTURE_HANDLE;
        return *this;
    }

    RenderPass& RenderPass::AddDependency(const std::string& passName) {
        m_Dependencies.push_back(passName);
        return *this;
    }

    RenderPass& RenderPass::SetExecuteFunc(std::function<void()> func) {
        m_ExecuteFunc = std::move(func);
        return *this;
    }

    void RenderPass::Execute() const {
        if (m_ExecuteFunc) {
            m_ExecuteFunc();
        }
    }

}

