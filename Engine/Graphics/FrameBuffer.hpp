#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Framebuffer specification
     */
    struct FramebufferSpec {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        uint32_t Samples = 1;  // MSAA samples
        bool SwapChainTarget = false;  // Is this the main screen framebuffer?
        
        // Attachments
        bool HasColorAttachment = true;
        bool HasDepthAttachment = true;
    };

    /**
     * @brief Framebuffer - render to texture
     * 
     * Features:
     * - Render to texture for post-processing
     * - Multiple color attachments
     * - Depth/stencil attachments
     * - MSAA support
     */
    class Framebuffer {
    public:
        Framebuffer(const FramebufferSpec& spec);
        ~Framebuffer();
        
        /**
         * @brief Bind framebuffer for rendering
         */
        void Bind();
        
        /**
         * @brief Unbind framebuffer (back to default)
         */
        void Unbind();
        
        /**
         * @brief Resize framebuffer
         */
        void Resize(uint32_t width, uint32_t height);
        
        /**
         * @brief Get color attachment texture ID
         */
        uint32_t GetColorAttachment(uint32_t index = 0) const;
        
        /**
         * @brief Get depth attachment texture ID
         */
        uint32_t GetDepthAttachment() const { return m_DepthAttachment; }
        
        /**
         * @brief Get specification
         */
        const FramebufferSpec& GetSpecification() const { return m_Spec; }
        
    private:
        void Invalidate();
        void Release();
        
    private:
        uint32_t m_RendererID = 0;
        uint32_t m_ColorAttachment = 0;
        uint32_t m_DepthAttachment = 0;
        
        FramebufferSpec m_Spec;
    };
}