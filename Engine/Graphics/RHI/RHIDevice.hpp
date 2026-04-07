#pragma once

#include "RHITypes.hpp"
#include <string>
#include <vector>

namespace GameEngine { namespace RHI {

    // Forward declarations
    class RHIBuffer;
    class RHIShader;
    class RHITexture;
    class RHIFramebuffer;

    enum class RHIBackend { OpenGL, Vulkan /* future */ };

    // =====================================================================
    // Abstract Buffer
    // =====================================================================

    class RHIBuffer {
    public:
        virtual ~RHIBuffer() = default;
        virtual void bind() = 0;
        virtual void unbind() = 0;
        virtual void setData(const void* data, size_t size, size_t offset = 0) = 0;
        virtual void* map() = 0;
        virtual void unmap() = 0;
        virtual size_t getSize() const = 0;
        virtual BufferUsage getUsage() const = 0;
    };

    // =====================================================================
    // Abstract Shader
    // =====================================================================

    class RHIShader {
    public:
        virtual ~RHIShader() = default;
        virtual void bind() = 0;
        virtual void unbind() = 0;
        virtual void setInt(const std::string& name, int value) = 0;
        virtual void setFloat(const std::string& name, float value) = 0;
        virtual void setVec2(const std::string& name, const glm::vec2& value) = 0;
        virtual void setVec3(const std::string& name, const glm::vec3& value) = 0;
        virtual void setVec4(const std::string& name, const glm::vec4& value) = 0;
        virtual void setMat3(const std::string& name, const glm::mat3& value) = 0;
        virtual void setMat4(const std::string& name, const glm::mat4& value) = 0;
        virtual bool isValid() const = 0;
    };

    // =====================================================================
    // Abstract Texture
    // =====================================================================

    struct TextureDesc {
        uint32_t Width = 1, Height = 1;
        TextureFormat Format = TextureFormat::RGBA8;
        TextureFilter MinFilter = TextureFilter::Linear;
        TextureFilter MagFilter = TextureFilter::Linear;
        TextureWrap WrapS = TextureWrap::Repeat;
        TextureWrap WrapT = TextureWrap::Repeat;
        bool GenerateMipmaps = true;
    };

    class RHITexture {
    public:
        virtual ~RHITexture() = default;
        virtual void bind(uint32_t slot = 0) = 0;
        virtual void unbind() = 0;
        virtual void setData(const void* data, uint32_t width, uint32_t height) = 0;
        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;
        virtual uint32_t getNativeHandle() const = 0;
    };

    // =====================================================================
    // Abstract Framebuffer
    // =====================================================================

    struct FramebufferAttachment {
        TextureFormat Format = TextureFormat::RGBA8;
    };

    struct FramebufferDesc {
        uint32_t Width = 1, Height = 1;
        std::vector<FramebufferAttachment> ColorAttachments;
        bool HasDepthStencil = true;
    };

    class RHIFramebuffer {
    public:
        virtual ~RHIFramebuffer() = default;
        virtual void bind() = 0;
        virtual void unbind() = 0;
        virtual void resize(uint32_t width, uint32_t height) = 0;
        virtual uint32_t getColorAttachment(uint32_t index = 0) const = 0;
        virtual uint32_t getDepthAttachment() const = 0;
        virtual uint32_t getWidth() const = 0;
        virtual uint32_t getHeight() const = 0;
    };

    // =====================================================================
    // Pipeline State
    // =====================================================================

    struct PipelineDesc {
        Ref<RHIShader> Shader;
        VertexLayout Layout;
        BlendState Blend;
        DepthState Depth;
        RasterizerState Rasterizer;
        PrimitiveTopology Topology = PrimitiveTopology::Triangles;
    };

    class RHIPipeline {
    public:
        virtual ~RHIPipeline() = default;
        virtual void bind() = 0;
        virtual void unbind() = 0;
        virtual const PipelineDesc& getDesc() const = 0;
    };

    // =====================================================================
    // Device — factory for all RHI objects
    // =====================================================================

    class RHIDevice {
    public:
        virtual ~RHIDevice() = default;

        virtual Ref<RHIBuffer> createBuffer(BufferUsage usage, BufferAccess access,
                                             const void* data, size_t size) = 0;
        virtual Ref<RHIShader> createShader(const std::string& vertexSrc,
                                             const std::string& fragmentSrc) = 0;
        virtual Ref<RHITexture> createTexture(const TextureDesc& desc, const void* data = nullptr) = 0;
        virtual Ref<RHIFramebuffer> createFramebuffer(const FramebufferDesc& desc) = 0;
        virtual Ref<RHIPipeline> createPipeline(const PipelineDesc& desc) = 0;

        virtual void setViewport(const Viewport& vp) = 0;
        virtual void setScissor(const ScissorRect& rect) = 0;
        virtual void clear(float r, float g, float b, float a, float depth = 1.0f) = 0;
        virtual void draw(uint32_t vertexCount, uint32_t firstVertex = 0) = 0;
        virtual void drawIndexed(uint32_t indexCount, uint32_t firstIndex = 0) = 0;

        virtual std::string getBackendName() const = 0;

        static Ref<RHIDevice> Create(RHIBackend backend = RHIBackend::OpenGL);
    };

}} // namespace GameEngine::RHI
