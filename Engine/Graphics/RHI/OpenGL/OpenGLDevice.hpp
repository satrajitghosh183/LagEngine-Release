#pragma once

// =============================================================================
// OpenGLDevice — STUB (Vulkan-only build)
//
// The engine has migrated entirely to Vulkan. This file is intentionally left
// as an empty shell so that any stale #include references compile without
// errors while producing no code. No OpenGL symbols are referenced here.
// =============================================================================

#include "../RHIDevice.hpp"

namespace GameEngine { namespace RHI {

    // =========================================================================
    // Stub implementations — all no-ops / return nulls.
    // These classes exist solely to satisfy the abstract RHIDevice interface.
    // The Vulkan backend (VulkanDevice / VulkanBuffer / etc.) is the real impl.
    // =========================================================================

    class OpenGLBuffer : public RHIBuffer {
    public:
        OpenGLBuffer(BufferUsage, BufferAccess, const void*, size_t size) : m_Size(size) {}
        ~OpenGLBuffer() override = default;

        void bind() override {}
        void unbind() override {}
        void setData(const void*, size_t, size_t = 0) override {}
        void* map() override { return nullptr; }
        void unmap() override {}
        size_t getSize() const override { return m_Size; }
        BufferUsage getUsage() const override { return m_Usage; }

    private:
        size_t m_Size = 0;
        BufferUsage m_Usage = BufferUsage::Vertex;
    };

    class OpenGLShader : public RHIShader {
    public:
        OpenGLShader(const std::string&, const std::string&) {}
        ~OpenGLShader() override = default;

        void bind() override {}
        void unbind() override {}
        void setInt(const std::string&, int) override {}
        void setFloat(const std::string&, float) override {}
        void setVec2(const std::string&, const glm::vec2&) override {}
        void setVec3(const std::string&, const glm::vec3&) override {}
        void setVec4(const std::string&, const glm::vec4&) override {}
        void setMat3(const std::string&, const glm::mat3&) override {}
        void setMat4(const std::string&, const glm::mat4&) override {}
        bool isValid() const override { return false; }
    };

    class OpenGLTexture : public RHITexture {
    public:
        OpenGLTexture(const TextureDesc& desc, const void*) : m_Width(desc.Width), m_Height(desc.Height) {}
        ~OpenGLTexture() override = default;

        void bind(uint32_t = 0) override {}
        void unbind() override {}
        void setData(const void*, uint32_t w, uint32_t h) override { m_Width = w; m_Height = h; }
        uint32_t getWidth() const override { return m_Width; }
        uint32_t getHeight() const override { return m_Height; }
        uint32_t getNativeHandle() const override { return 0; }

    private:
        uint32_t m_Width = 0, m_Height = 0;
    };

    class OpenGLFramebuffer : public RHIFramebuffer {
    public:
        explicit OpenGLFramebuffer(const FramebufferDesc& desc) : m_Desc(desc) {}
        ~OpenGLFramebuffer() override = default;

        void bind() override {}
        void unbind() override {}
        void resize(uint32_t w, uint32_t h) override { m_Desc.Width = w; m_Desc.Height = h; }
        uint32_t getColorAttachment(uint32_t = 0) const override { return 0; }
        uint32_t getDepthAttachment() const override { return 0; }
        uint32_t getWidth() const override { return m_Desc.Width; }
        uint32_t getHeight() const override { return m_Desc.Height; }

    private:
        FramebufferDesc m_Desc;
    };

    class OpenGLPipeline : public RHIPipeline {
    public:
        explicit OpenGLPipeline(const PipelineDesc& desc) : m_Desc(desc) {}
        void bind() override {}
        void unbind() override {}
        const PipelineDesc& getDesc() const override { return m_Desc; }

    private:
        PipelineDesc m_Desc;
    };

    class OpenGLDevice : public RHIDevice {
    public:
        OpenGLDevice() = default;
        ~OpenGLDevice() override = default;

        Ref<RHIBuffer> createBuffer(BufferUsage usage, BufferAccess access,
                                     const void* data, size_t size) override {
            return CreateRef<OpenGLBuffer>(usage, access, data, size);
        }
        Ref<RHIShader> createShader(const std::string& v, const std::string& f) override {
            return CreateRef<OpenGLShader>(v, f);
        }
        Ref<RHITexture> createTexture(const TextureDesc& desc, const void* data = nullptr) override {
            return CreateRef<OpenGLTexture>(desc, data);
        }
        Ref<RHIFramebuffer> createFramebuffer(const FramebufferDesc& desc) override {
            return CreateRef<OpenGLFramebuffer>(desc);
        }
        Ref<RHIPipeline> createPipeline(const PipelineDesc& desc) override {
            return CreateRef<OpenGLPipeline>(desc);
        }

        void setViewport(const Viewport&) override {}
        void setScissor(const ScissorRect&) override {}
        void clear(float, float, float, float, float = 1.0f) override {}
        void draw(uint32_t, uint32_t = 0) override {}
        void drawIndexed(uint32_t, uint32_t = 0) override {}

        std::string getBackendName() const override { return "Stub (Vulkan-only build)"; }
    };

}} // namespace GameEngine::RHI
