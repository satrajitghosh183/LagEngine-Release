#include "Framebuffer.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>

namespace GameEngine {

    Framebuffer::Framebuffer(const FramebufferSpec& spec)
        : m_Spec(spec) {
        Invalidate();
    }

    Framebuffer::~Framebuffer() {
        Release();
    }

    void Framebuffer::Invalidate() {
        if (m_RendererID) {
            Release();
        }
        
        // Create framebuffer
        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        
        // Create color attachment
        if (m_Spec.HasColorAttachment) {
            glGenTextures(1, &m_ColorAttachment);
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Spec.Width, m_Spec.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
        }
        
        // Create depth attachment
        if (m_Spec.HasDepthAttachment) {
            glGenTextures(1, &m_DepthAttachment);
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Spec.Width, m_Spec.Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
        }
        
        // Check framebuffer completeness
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            GE_CORE_ERROR("Framebuffer is incomplete!");
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        GE_CORE_INFO("Framebuffer created: {0}x{1}", m_Spec.Width, m_Spec.Height);
    }

    void Framebuffer::Release() {
        if (m_RendererID) {
            glDeleteFramebuffers(1, &m_RendererID);
            m_RendererID = 0;
        }
        
        if (m_ColorAttachment) {
            glDeleteTextures(1, &m_ColorAttachment);
            m_ColorAttachment = 0;
        }
        
        if (m_DepthAttachment) {
            glDeleteTextures(1, &m_DepthAttachment);
            m_DepthAttachment = 0;
        }
    }

    void Framebuffer::Bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        glViewport(0, 0, m_Spec.Width, m_Spec.Height);
    }

    void Framebuffer::Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height) {
        if (width == 0 || height == 0 || width > 8192 || height > 8192) {
            GE_CORE_WARN("Invalid framebuffer size: {0}x{1}", width, height);
            return;
        }
        
        m_Spec.Width = width;
        m_Spec.Height = height;
        
        Invalidate();
    }

    uint32_t Framebuffer::GetColorAttachment(uint32_t index) const {
        GE_CORE_ASSERT(index == 0, "Only single color attachment supported currently");
        return m_ColorAttachment;
    }
}