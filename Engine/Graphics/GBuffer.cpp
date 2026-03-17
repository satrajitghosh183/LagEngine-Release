#include "GBuffer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    GBuffer::~GBuffer() {
        Cleanup();
    }

    void GBuffer::Init(int width, int height) {
        m_Width = width;
        m_Height = height;

        glGenFramebuffers(1, &m_FBO);
        if (m_FBO) CreateTextures();

        Logger::Log(LogLevel::Info,
            "GBuffer initialized (" + std::to_string(m_Width) + "x" + std::to_string(m_Height) + ")");
    }

    void GBuffer::Resize(int width, int height) {
        if (width == m_Width && height == m_Height)
            return;

        m_Width = width;
        m_Height = height;

        // Delete old textures but keep the FBO handle
        if (m_PositionTexture) glDeleteTextures(1, &m_PositionTexture);
        if (m_NormalTexture)   glDeleteTextures(1, &m_NormalTexture);
        if (m_AlbedoTexture)   glDeleteTextures(1, &m_AlbedoTexture);
        if (m_MetallicRoughnessTexture) glDeleteTextures(1, &m_MetallicRoughnessTexture);
        if (m_DepthTexture)    glDeleteTextures(1, &m_DepthTexture);

        if (m_FBO) CreateTextures();

        Logger::Log(LogLevel::Debug,
            "GBuffer resized to " + std::to_string(m_Width) + "x" + std::to_string(m_Height));
    }

    void GBuffer::CreateTextures() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        // Position buffer (RGB16F) - world-space position
        glGenTextures(1, &m_PositionTexture);
        glBindTexture(GL_TEXTURE_2D, m_PositionTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PositionTexture, 0);

        // Normal buffer (RGB16F) - world-space normal
        glGenTextures(1, &m_NormalTexture);
        glBindTexture(GL_TEXTURE_2D, m_NormalTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_NormalTexture, 0);

        // Albedo buffer (RGBA8) - base color + alpha
        glGenTextures(1, &m_AlbedoTexture);
        glBindTexture(GL_TEXTURE_2D, m_AlbedoTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_AlbedoTexture, 0);

        // MetallicRoughness buffer (RG8) - metallic in R, roughness in G
        glGenTextures(1, &m_MetallicRoughnessTexture);
        glBindTexture(GL_TEXTURE_2D, m_MetallicRoughnessTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, m_Width, m_Height, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_MetallicRoughnessTexture, 0);

        // Depth buffer (DEPTH_COMPONENT24)
        glGenTextures(1, &m_DepthTexture);
        glBindTexture(GL_TEXTURE_2D, m_DepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthTexture, 0);

        // Tell OpenGL which color attachments we'll use for rendering
        GLenum attachments[4] = {
            GL_COLOR_ATTACHMENT0,
            GL_COLOR_ATTACHMENT1,
            GL_COLOR_ATTACHMENT2,
            GL_COLOR_ATTACHMENT3
        };
        glDrawBuffers(4, attachments);

        // Verify framebuffer completeness
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            Logger::Log(LogLevel::Error,
                "GBuffer framebuffer incomplete! Status: " + std::to_string(status));
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GBuffer::BindForWriting() {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_FBO);
        glViewport(0, 0, m_Width, m_Height);
    }

    void GBuffer::BindForReading() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FBO);
    }

    void GBuffer::BindTextures() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_PositionTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_NormalTexture);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_AlbedoTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_MetallicRoughnessTexture);
    }

    void GBuffer::Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GBuffer::Cleanup() {
        if (m_PositionTexture)         { glDeleteTextures(1, &m_PositionTexture);         m_PositionTexture = 0; }
        if (m_NormalTexture)           { glDeleteTextures(1, &m_NormalTexture);            m_NormalTexture = 0; }
        if (m_AlbedoTexture)           { glDeleteTextures(1, &m_AlbedoTexture);            m_AlbedoTexture = 0; }
        if (m_MetallicRoughnessTexture){ glDeleteTextures(1, &m_MetallicRoughnessTexture); m_MetallicRoughnessTexture = 0; }
        if (m_DepthTexture)            { glDeleteTextures(1, &m_DepthTexture);             m_DepthTexture = 0; }
        if (m_FBO)                     { glDeleteFramebuffers(1, &m_FBO);                  m_FBO = 0; }
    }

}
