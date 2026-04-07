#pragma once

#include "../RHIDevice.hpp"
#include <glad/glad.h>

namespace GameEngine { namespace RHI {

    // =====================================================================
    // OpenGL Buffer
    // =====================================================================

    class OpenGLBuffer : public RHIBuffer {
    public:
        OpenGLBuffer(BufferUsage usage, BufferAccess access, const void* data, size_t size);
        ~OpenGLBuffer() override;

        void bind() override;
        void unbind() override;
        void setData(const void* data, size_t size, size_t offset = 0) override;
        void* map() override;
        void unmap() override;
        size_t getSize() const override { return m_Size; }
        BufferUsage getUsage() const override { return m_Usage; }
        GLuint getHandle() const { return m_Handle; }

    private:
        GLuint m_Handle = 0;
        GLenum m_Target = GL_ARRAY_BUFFER;
        GLenum m_GLUsage = GL_STATIC_DRAW;
        BufferUsage m_Usage;
        size_t m_Size = 0;
    };

    // =====================================================================
    // OpenGL Shader
    // =====================================================================

    class OpenGLShader : public RHIShader {
    public:
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~OpenGLShader() override;

        void bind() override;
        void unbind() override;
        void setInt(const std::string& name, int value) override;
        void setFloat(const std::string& name, float value) override;
        void setVec2(const std::string& name, const glm::vec2& value) override;
        void setVec3(const std::string& name, const glm::vec3& value) override;
        void setVec4(const std::string& name, const glm::vec4& value) override;
        void setMat3(const std::string& name, const glm::mat3& value) override;
        void setMat4(const std::string& name, const glm::mat4& value) override;
        bool isValid() const override { return m_Program != 0; }

    private:
        GLint getUniformLocation(const std::string& name);
        GLuint compileShader(GLenum type, const std::string& source);

        GLuint m_Program = 0;
        std::unordered_map<std::string, GLint> m_UniformCache;
    };

    // =====================================================================
    // OpenGL Texture
    // =====================================================================

    class OpenGLTexture : public RHITexture {
    public:
        OpenGLTexture(const TextureDesc& desc, const void* data);
        ~OpenGLTexture() override;

        void bind(uint32_t slot = 0) override;
        void unbind() override;
        void setData(const void* data, uint32_t width, uint32_t height) override;
        uint32_t getWidth() const override { return m_Width; }
        uint32_t getHeight() const override { return m_Height; }
        uint32_t getNativeHandle() const override { return m_Handle; }

    private:
        GLuint m_Handle = 0;
        uint32_t m_Width = 0, m_Height = 0;
        GLenum m_InternalFormat = GL_RGBA8;
        GLenum m_DataFormat = GL_RGBA;
    };

    // =====================================================================
    // OpenGL Framebuffer
    // =====================================================================

    class OpenGLFramebuffer : public RHIFramebuffer {
    public:
        OpenGLFramebuffer(const FramebufferDesc& desc);
        ~OpenGLFramebuffer() override;

        void bind() override;
        void unbind() override;
        void resize(uint32_t width, uint32_t height) override;
        uint32_t getColorAttachment(uint32_t index = 0) const override;
        uint32_t getDepthAttachment() const override { return m_DepthAttachment; }
        uint32_t getWidth() const override { return m_Desc.Width; }
        uint32_t getHeight() const override { return m_Desc.Height; }

    private:
        void invalidate();

        GLuint m_Handle = 0;
        std::vector<GLuint> m_ColorAttachments;
        GLuint m_DepthAttachment = 0;
        FramebufferDesc m_Desc;
    };

    // =====================================================================
    // OpenGL Pipeline
    // =====================================================================

    class OpenGLPipeline : public RHIPipeline {
    public:
        explicit OpenGLPipeline(const PipelineDesc& desc);
        void bind() override;
        void unbind() override;
        const PipelineDesc& getDesc() const override { return m_Desc; }

    private:
        PipelineDesc m_Desc;
        GLuint m_VAO = 0;
    };

    // =====================================================================
    // OpenGL Device
    // =====================================================================

    class OpenGLDevice : public RHIDevice {
    public:
        OpenGLDevice() = default;
        ~OpenGLDevice() override = default;

        Ref<RHIBuffer> createBuffer(BufferUsage usage, BufferAccess access,
                                     const void* data, size_t size) override;
        Ref<RHIShader> createShader(const std::string& vertexSrc,
                                     const std::string& fragmentSrc) override;
        Ref<RHITexture> createTexture(const TextureDesc& desc, const void* data = nullptr) override;
        Ref<RHIFramebuffer> createFramebuffer(const FramebufferDesc& desc) override;
        Ref<RHIPipeline> createPipeline(const PipelineDesc& desc) override;

        void setViewport(const Viewport& vp) override;
        void setScissor(const ScissorRect& rect) override;
        void clear(float r, float g, float b, float a, float depth = 1.0f) override;
        void draw(uint32_t vertexCount, uint32_t firstVertex = 0) override;
        void drawIndexed(uint32_t indexCount, uint32_t firstIndex = 0) override;

        std::string getBackendName() const override { return "OpenGL"; }
    };

}} // namespace GameEngine::RHI
