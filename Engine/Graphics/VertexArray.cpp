#include "VertexArray.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>

namespace GameEngine {

    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
                return GL_FLOAT;
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return GL_INT;
            case ShaderDataType::Bool:
                return GL_BOOL;
            default:
                GE_CORE_ASSERT(false, "Unknown ShaderDataType!");
                return 0;
        }
    }

    // ========== VertexBuffer ==========

    VertexBuffer::VertexBuffer(float* vertices, uint32_t size) {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
    }

    VertexBuffer::VertexBuffer(uint32_t size) {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    VertexBuffer::~VertexBuffer() {
        glDeleteBuffers(1, &m_RendererID);
    }

    void VertexBuffer::Bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    }

    void VertexBuffer::Unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VertexBuffer::SetData(const void* data, uint32_t size) {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    // ========== IndexBuffer ==========

    IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t count)
        : m_Count(count) {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }

    IndexBuffer::~IndexBuffer() {
        glDeleteBuffers(1, &m_RendererID);
    }

    void IndexBuffer::Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    }

    void IndexBuffer::Unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    // ========== VertexArray ==========

    VertexArray::VertexArray() {
        glGenVertexArrays(1, &m_RendererID);
    }

    VertexArray::~VertexArray() {
        glDeleteVertexArrays(1, &m_RendererID);
    }

    void VertexArray::Bind() const {
        glBindVertexArray(m_RendererID);
    }

    void VertexArray::Unbind() const {
        glBindVertexArray(0);
    }

    void VertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
        GE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");
        
        glBindVertexArray(m_RendererID);
        vertexBuffer->Bind();
        
        const auto& layout = vertexBuffer->GetLayout();
        for (const auto& element : layout) {
            glEnableVertexAttribArray(m_VertexBufferIndex);
            glVertexAttribPointer(
                m_VertexBufferIndex,
                element.GetComponentCount(),
                ShaderDataTypeToOpenGLBaseType(element.Type),
                element.Normalized ? GL_TRUE : GL_FALSE,
                layout.GetStride(),
                (const void*)(intptr_t)element.Offset
            );
            m_VertexBufferIndex++;
        }
        
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void VertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) {
        glBindVertexArray(m_RendererID);
        indexBuffer->Bind();
        
        m_IndexBuffer = indexBuffer;
    }
}