#include "OpenGLDevice.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

namespace GameEngine { namespace RHI {

    // =====================================================================
    // RHIDevice factory
    // =====================================================================

    Ref<RHIDevice> RHIDevice::Create(RHIBackend backend) {
        switch (backend) {
            case RHIBackend::OpenGL: return CreateRef<OpenGLDevice>();
            default: return nullptr;
        }
    }

    // =====================================================================
    // OpenGLBuffer
    // =====================================================================

    static GLenum toGLTarget(BufferUsage usage) {
        switch (usage) {
            case BufferUsage::Vertex:  return GL_ARRAY_BUFFER;
            case BufferUsage::Index:   return GL_ELEMENT_ARRAY_BUFFER;
            case BufferUsage::Uniform: return GL_UNIFORM_BUFFER;
            case BufferUsage::Storage: return GL_SHADER_STORAGE_BUFFER;
        }
        return GL_ARRAY_BUFFER;
    }

    static GLenum toGLUsage(BufferAccess access) {
        switch (access) {
            case BufferAccess::Static:  return GL_STATIC_DRAW;
            case BufferAccess::Dynamic: return GL_DYNAMIC_DRAW;
            case BufferAccess::Stream:  return GL_STREAM_DRAW;
        }
        return GL_STATIC_DRAW;
    }

    OpenGLBuffer::OpenGLBuffer(BufferUsage usage, BufferAccess access, const void* data, size_t size)
        : m_Usage(usage), m_Size(size) {
        m_Target = toGLTarget(usage);
        m_GLUsage = toGLUsage(access);
        glGenBuffers(1, &m_Handle);
        glBindBuffer(m_Target, m_Handle);
        glBufferData(m_Target, size, data, m_GLUsage);
    }

    OpenGLBuffer::~OpenGLBuffer() {
        if (m_Handle) glDeleteBuffers(1, &m_Handle);
    }

    void OpenGLBuffer::bind() { glBindBuffer(m_Target, m_Handle); }
    void OpenGLBuffer::unbind() { glBindBuffer(m_Target, 0); }

    void OpenGLBuffer::setData(const void* data, size_t size, size_t offset) {
        glBindBuffer(m_Target, m_Handle);
        if (offset == 0 && size == m_Size) {
            glBufferData(m_Target, size, data, m_GLUsage);
        } else {
            glBufferSubData(m_Target, offset, size, data);
        }
    }

    void* OpenGLBuffer::map() {
        glBindBuffer(m_Target, m_Handle);
        return glMapBuffer(m_Target, GL_READ_WRITE);
    }

    void OpenGLBuffer::unmap() {
        glBindBuffer(m_Target, m_Handle);
        glUnmapBuffer(m_Target);
    }

    // =====================================================================
    // OpenGLShader
    // =====================================================================

    OpenGLShader::OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc) {
        GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
        GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs) return;

        m_Program = glCreateProgram();
        glAttachShader(m_Program, vs);
        glAttachShader(m_Program, fs);
        glLinkProgram(m_Program);

        GLint success;
        glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetProgramInfoLog(m_Program, sizeof(log), nullptr, log);
            glDeleteProgram(m_Program);
            m_Program = 0;
        }

        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    OpenGLShader::~OpenGLShader() {
        if (m_Program) glDeleteProgram(m_Program);
    }

    GLuint OpenGLShader::compileShader(GLenum type, const std::string& source) {
        GLuint shader = glCreateShader(type);
        const char* src = source.c_str();
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void OpenGLShader::bind() { glUseProgram(m_Program); }
    void OpenGLShader::unbind() { glUseProgram(0); }

    GLint OpenGLShader::getUniformLocation(const std::string& name) {
        auto it = m_UniformCache.find(name);
        if (it != m_UniformCache.end()) return it->second;
        GLint loc = glGetUniformLocation(m_Program, name.c_str());
        m_UniformCache[name] = loc;
        return loc;
    }

    void OpenGLShader::setInt(const std::string& name, int value) {
        glUniform1i(getUniformLocation(name), value);
    }
    void OpenGLShader::setFloat(const std::string& name, float value) {
        glUniform1f(getUniformLocation(name), value);
    }
    void OpenGLShader::setVec2(const std::string& name, const glm::vec2& value) {
        glUniform2fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void OpenGLShader::setVec3(const std::string& name, const glm::vec3& value) {
        glUniform3fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void OpenGLShader::setVec4(const std::string& name, const glm::vec4& value) {
        glUniform4fv(getUniformLocation(name), 1, glm::value_ptr(value));
    }
    void OpenGLShader::setMat3(const std::string& name, const glm::mat3& value) {
        glUniformMatrix3fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }
    void OpenGLShader::setMat4(const std::string& name, const glm::mat4& value) {
        glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
    }

    // =====================================================================
    // OpenGLTexture
    // =====================================================================

    static GLenum toGLInternalFormat(TextureFormat fmt) {
        switch (fmt) {
            case TextureFormat::R8:    return GL_R8;
            case TextureFormat::RG8:   return GL_RG8;
            case TextureFormat::RGB8:  return GL_RGB8;
            case TextureFormat::RGBA8: return GL_RGBA8;
            case TextureFormat::R16F:  return GL_R16F;
            case TextureFormat::RG16F: return GL_RG16F;
            case TextureFormat::RGB16F: return GL_RGB16F;
            case TextureFormat::RGBA16F: return GL_RGBA16F;
            case TextureFormat::R32F:  return GL_R32F;
            case TextureFormat::RGBA32F: return GL_RGBA32F;
            case TextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
            case TextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
        }
        return GL_RGBA8;
    }

    static GLenum toGLDataFormat(TextureFormat fmt) {
        switch (fmt) {
            case TextureFormat::R8: case TextureFormat::R16F: case TextureFormat::R32F:
                return GL_RED;
            case TextureFormat::RG8: case TextureFormat::RG16F:
                return GL_RG;
            case TextureFormat::RGB8: case TextureFormat::RGB16F:
                return GL_RGB;
            case TextureFormat::RGBA8: case TextureFormat::RGBA16F: case TextureFormat::RGBA32F:
                return GL_RGBA;
            case TextureFormat::Depth24Stencil8:
                return GL_DEPTH_STENCIL;
            case TextureFormat::Depth32F:
                return GL_DEPTH_COMPONENT;
        }
        return GL_RGBA;
    }

    static GLenum toGLFilter(TextureFilter filter) {
        switch (filter) {
            case TextureFilter::Nearest:   return GL_NEAREST;
            case TextureFilter::Linear:    return GL_LINEAR;
            case TextureFilter::Trilinear: return GL_LINEAR_MIPMAP_LINEAR;
        }
        return GL_LINEAR;
    }

    static GLenum toGLWrap(TextureWrap wrap) {
        switch (wrap) {
            case TextureWrap::Repeat: return GL_REPEAT;
            case TextureWrap::Clamp:  return GL_CLAMP_TO_EDGE;
            case TextureWrap::Mirror: return GL_MIRRORED_REPEAT;
        }
        return GL_REPEAT;
    }

    OpenGLTexture::OpenGLTexture(const TextureDesc& desc, const void* data)
        : m_Width(desc.Width), m_Height(desc.Height) {
        m_InternalFormat = toGLInternalFormat(desc.Format);
        m_DataFormat = toGLDataFormat(desc.Format);

        glGenTextures(1, &m_Handle);
        glBindTexture(GL_TEXTURE_2D, m_Handle);

        GLenum dataType = GL_UNSIGNED_BYTE;
        if (desc.Format == TextureFormat::R16F || desc.Format == TextureFormat::RG16F ||
            desc.Format == TextureFormat::RGB16F || desc.Format == TextureFormat::RGBA16F ||
            desc.Format == TextureFormat::R32F || desc.Format == TextureFormat::RGBA32F ||
            desc.Format == TextureFormat::Depth32F) {
            dataType = GL_FLOAT;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, m_Width, m_Height, 0,
                     m_DataFormat, dataType, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, toGLFilter(desc.MinFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, toGLFilter(desc.MagFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, toGLWrap(desc.WrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, toGLWrap(desc.WrapT));

        if (desc.GenerateMipmaps && data) {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    }

    OpenGLTexture::~OpenGLTexture() {
        if (m_Handle) glDeleteTextures(1, &m_Handle);
    }

    void OpenGLTexture::bind(uint32_t slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, m_Handle);
    }

    void OpenGLTexture::unbind() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void OpenGLTexture::setData(const void* data, uint32_t width, uint32_t height) {
        m_Width = width;
        m_Height = height;
        glBindTexture(GL_TEXTURE_2D, m_Handle);
        glTexImage2D(GL_TEXTURE_2D, 0, m_InternalFormat, width, height, 0,
                     m_DataFormat, GL_UNSIGNED_BYTE, data);
    }

    // =====================================================================
    // OpenGLFramebuffer
    // =====================================================================

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferDesc& desc) : m_Desc(desc) {
        invalidate();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer() {
        if (m_Handle) glDeleteFramebuffers(1, &m_Handle);
        for (auto tex : m_ColorAttachments) {
            if (tex) glDeleteTextures(1, &tex);
        }
        if (m_DepthAttachment) glDeleteTextures(1, &m_DepthAttachment);
    }

    void OpenGLFramebuffer::invalidate() {
        if (m_Handle) {
            glDeleteFramebuffers(1, &m_Handle);
            for (auto tex : m_ColorAttachments) glDeleteTextures(1, &tex);
            if (m_DepthAttachment) glDeleteTextures(1, &m_DepthAttachment);
            m_ColorAttachments.clear();
            m_DepthAttachment = 0;
        }

        glGenFramebuffers(1, &m_Handle);
        glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);

        for (size_t i = 0; i < m_Desc.ColorAttachments.size(); i++) {
            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);

            GLenum internalFmt = toGLInternalFormat(m_Desc.ColorAttachments[i].Format);
            GLenum dataFmt = toGLDataFormat(m_Desc.ColorAttachments[i].Format);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, m_Desc.Width, m_Desc.Height, 0,
                         dataFmt, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i),
                                   GL_TEXTURE_2D, tex, 0);
            m_ColorAttachments.push_back(tex);
        }

        if (m_Desc.HasDepthStencil) {
            glGenTextures(1, &m_DepthAttachment);
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Desc.Width, m_Desc.Height, 0,
                         GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                   GL_TEXTURE_2D, m_DepthAttachment, 0);
        }

        if (!m_Desc.ColorAttachments.empty()) {
            std::vector<GLenum> drawBuffers;
            for (size_t i = 0; i < m_ColorAttachments.size(); i++) {
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i));
            }
            glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, m_Handle);
        glViewport(0, 0, m_Desc.Width, m_Desc.Height);
    }

    void OpenGLFramebuffer::unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::resize(uint32_t width, uint32_t height) {
        m_Desc.Width = width;
        m_Desc.Height = height;
        invalidate();
    }

    uint32_t OpenGLFramebuffer::getColorAttachment(uint32_t index) const {
        if (index < m_ColorAttachments.size()) return m_ColorAttachments[index];
        return 0;
    }

    // =====================================================================
    // OpenGLPipeline
    // =====================================================================

    static GLenum toGLTopology(PrimitiveTopology topo) {
        switch (topo) {
            case PrimitiveTopology::Triangles:     return GL_TRIANGLES;
            case PrimitiveTopology::TriangleStrip: return GL_TRIANGLE_STRIP;
            case PrimitiveTopology::Lines:         return GL_LINES;
            case PrimitiveTopology::LineStrip:     return GL_LINE_STRIP;
            case PrimitiveTopology::Points:        return GL_POINTS;
        }
        return GL_TRIANGLES;
    }

    OpenGLPipeline::OpenGLPipeline(const PipelineDesc& desc) : m_Desc(desc) {
        glGenVertexArrays(1, &m_VAO);
    }

    void OpenGLPipeline::bind() {
        if (m_Desc.Shader) m_Desc.Shader->bind();

        glBindVertexArray(m_VAO);

        // Apply blend state
        if (m_Desc.Blend.Enabled) {
            glEnable(GL_BLEND);
            auto toGL = [](BlendFactor f) -> GLenum {
                switch (f) {
                    case BlendFactor::Zero: return GL_ZERO;
                    case BlendFactor::One: return GL_ONE;
                    case BlendFactor::SrcAlpha: return GL_SRC_ALPHA;
                    case BlendFactor::OneMinusSrcAlpha: return GL_ONE_MINUS_SRC_ALPHA;
                    case BlendFactor::DstAlpha: return GL_DST_ALPHA;
                }
                return GL_ONE;
            };
            glBlendFuncSeparate(toGL(m_Desc.Blend.SrcColor), toGL(m_Desc.Blend.DstColor),
                                toGL(m_Desc.Blend.SrcAlpha), toGL(m_Desc.Blend.DstAlpha));
        } else {
            glDisable(GL_BLEND);
        }

        // Apply depth state
        if (m_Desc.Depth.TestEnabled) {
            glEnable(GL_DEPTH_TEST);
            auto toGL = [](CompareOp op) -> GLenum {
                switch (op) {
                    case CompareOp::Never: return GL_NEVER;
                    case CompareOp::Less: return GL_LESS;
                    case CompareOp::Equal: return GL_EQUAL;
                    case CompareOp::LessEqual: return GL_LEQUAL;
                    case CompareOp::Greater: return GL_GREATER;
                    case CompareOp::NotEqual: return GL_NOTEQUAL;
                    case CompareOp::GreaterEqual: return GL_GEQUAL;
                    case CompareOp::Always: return GL_ALWAYS;
                }
                return GL_LESS;
            };
            glDepthFunc(toGL(m_Desc.Depth.CompareFunc));
            glDepthMask(m_Desc.Depth.WriteEnabled ? GL_TRUE : GL_FALSE);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        // Apply rasterizer state
        if (m_Desc.Rasterizer.Culling != CullMode::None) {
            glEnable(GL_CULL_FACE);
            glCullFace(m_Desc.Rasterizer.Culling == CullMode::Front ? GL_FRONT : GL_BACK);
        } else {
            glDisable(GL_CULL_FACE);
        }

        glPolygonMode(GL_FRONT_AND_BACK,
                       m_Desc.Rasterizer.Fill == FillMode::Wireframe ? GL_LINE : GL_FILL);
    }

    void OpenGLPipeline::unbind() {
        glBindVertexArray(0);
        if (m_Desc.Shader) m_Desc.Shader->unbind();
    }

    // =====================================================================
    // OpenGLDevice
    // =====================================================================

    Ref<RHIBuffer> OpenGLDevice::createBuffer(BufferUsage usage, BufferAccess access,
                                               const void* data, size_t size) {
        return CreateRef<OpenGLBuffer>(usage, access, data, size);
    }

    Ref<RHIShader> OpenGLDevice::createShader(const std::string& vertexSrc,
                                               const std::string& fragmentSrc) {
        return CreateRef<OpenGLShader>(vertexSrc, fragmentSrc);
    }

    Ref<RHITexture> OpenGLDevice::createTexture(const TextureDesc& desc, const void* data) {
        return CreateRef<OpenGLTexture>(desc, data);
    }

    Ref<RHIFramebuffer> OpenGLDevice::createFramebuffer(const FramebufferDesc& desc) {
        return CreateRef<OpenGLFramebuffer>(desc);
    }

    Ref<RHIPipeline> OpenGLDevice::createPipeline(const PipelineDesc& desc) {
        return CreateRef<OpenGLPipeline>(desc);
    }

    void OpenGLDevice::setViewport(const Viewport& vp) {
        glViewport(static_cast<GLint>(vp.X), static_cast<GLint>(vp.Y),
                   static_cast<GLsizei>(vp.Width), static_cast<GLsizei>(vp.Height));
        glDepthRange(vp.MinDepth, vp.MaxDepth);
    }

    void OpenGLDevice::setScissor(const ScissorRect& rect) {
        glScissor(rect.X, rect.Y, rect.Width, rect.Height);
    }

    void OpenGLDevice::clear(float r, float g, float b, float a, float depth) {
        glClearColor(r, g, b, a);
        glClearDepth(depth);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLDevice::draw(uint32_t vertexCount, uint32_t firstVertex) {
        glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
    }

    void OpenGLDevice::drawIndexed(uint32_t indexCount, uint32_t firstIndex) {
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT,
                       reinterpret_cast<void*>(static_cast<size_t>(firstIndex) * sizeof(uint32_t)));
    }

}} // namespace GameEngine::RHI
