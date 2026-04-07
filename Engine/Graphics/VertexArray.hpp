#pragma once

#include "../Core/Base.hpp"
#include <vector>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Vertex buffer element data types
     */
    enum class ShaderDataType {
        None = 0,
        Float, Float2, Float3, Float4,
        Mat3, Mat4,
        Int, Int2, Int3, Int4,
        Bool
    };

    /**
     * @brief Get size of shader data type in bytes
     */
    static uint32_t ShaderDataTypeSize(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:    return 4;
            case ShaderDataType::Float2:   return 4 * 2;
            case ShaderDataType::Float3:   return 4 * 3;
            case ShaderDataType::Float4:   return 4 * 4;
            case ShaderDataType::Mat3:     return 4 * 3 * 3;
            case ShaderDataType::Mat4:     return 4 * 4 * 4;
            case ShaderDataType::Int:      return 4;
            case ShaderDataType::Int2:     return 4 * 2;
            case ShaderDataType::Int3:     return 4 * 3;
            case ShaderDataType::Int4:     return 4 * 4;
            case ShaderDataType::Bool:     return 1;
            default: return 0;
        }
    }

    /**
     * @brief Vertex buffer element
     * 
     * Describes a single attribute in the vertex layout
     * (e.g., position, normal, texture coordinates)
     */
    struct BufferElement {
        std::string Name;
        ShaderDataType Type;
        uint32_t Size;
        uint32_t Offset;
        bool Normalized;
        
        BufferElement() = default;
        
        BufferElement(ShaderDataType type, const std::string& name, bool normalized = false)
            : Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}
        
        uint32_t GetComponentCount() const {
            switch (Type) {
                case ShaderDataType::Float:   return 1;
                case ShaderDataType::Float2:  return 2;
                case ShaderDataType::Float3:  return 3;
                case ShaderDataType::Float4:  return 4;
                case ShaderDataType::Mat3:    return 3 * 3;
                case ShaderDataType::Mat4:    return 4 * 4;
                case ShaderDataType::Int:     return 1;
                case ShaderDataType::Int2:    return 2;
                case ShaderDataType::Int3:    return 3;
                case ShaderDataType::Int4:    return 4;
                case ShaderDataType::Bool:    return 1;
                default: return 0;
            }
        }
    };

    /**
     * @brief Vertex buffer layout
     * 
     * Describes the structure of vertex data
     */
    class BufferLayout {
    public:
        BufferLayout() {}
        
        BufferLayout(const std::initializer_list<BufferElement>& elements)
            : m_Elements(elements) {
            CalculateOffsetsAndStride();
        }
        
        uint32_t GetStride() const { return m_Stride; }
        const std::vector<BufferElement>& GetElements() const { return m_Elements; }
        
        std::vector<BufferElement>::iterator begin() { return m_Elements.begin(); }
        std::vector<BufferElement>::iterator end() { return m_Elements.end(); }
        std::vector<BufferElement>::const_iterator begin() const { return m_Elements.begin(); }
        std::vector<BufferElement>::const_iterator end() const { return m_Elements.end(); }
        
    private:
        void CalculateOffsetsAndStride() {
            uint32_t offset = 0;
            m_Stride = 0;
            for (auto& element : m_Elements) {
                element.Offset = offset;
                offset += element.Size;
                m_Stride += element.Size;
            }
        }
        
    private:
        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride = 0;
    };

    /**
     * @brief Vertex buffer (VBO)
     */
    class VertexBuffer {
    public:
        VertexBuffer(float* vertices, uint32_t size);
        VertexBuffer(uint32_t size); // Dynamic buffer
        ~VertexBuffer();
        
        void Bind() const;
        void Unbind() const;
        
        void SetData(const void* data, uint32_t size);
        
        const BufferLayout& GetLayout() const { return m_Layout; }
        void SetLayout(const BufferLayout& layout) { m_Layout = layout; }
        
    private:
        uint32_t m_RendererID;
        BufferLayout m_Layout;
    };

    /**
     * @brief Index buffer (EBO)
     */
    class IndexBuffer {
    public:
        IndexBuffer(uint32_t* indices, uint32_t count);
        ~IndexBuffer();
        
        void Bind() const;
        void Unbind() const;
        
        uint32_t GetCount() const { return m_Count; }
        
    private:
        uint32_t m_RendererID;
        uint32_t m_Count;
    };

    /**
     * @brief Vertex array (VAO)
     * 
     * Encapsulates vertex buffer layout and index buffer
     */
    class VertexArray {
    public:
        VertexArray();
        ~VertexArray();
        
        void Bind() const;
        void Unbind() const;
        
        void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
        void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);
        
        const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
        const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }
        
    private:
        uint32_t m_RendererID;
        uint32_t m_VertexBufferIndex = 0;
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffer;
    };
}