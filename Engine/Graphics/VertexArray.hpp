#pragma once

#include "../Core/Base.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
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

    static VkFormat ShaderDataTypeToVkFormat(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:    return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Float2:   return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Float3:   return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Float4:   return VK_FORMAT_R32G32B32A32_SFLOAT;
            case ShaderDataType::Int:      return VK_FORMAT_R32_SINT;
            case ShaderDataType::Int2:     return VK_FORMAT_R32G32_SINT;
            case ShaderDataType::Int3:     return VK_FORMAT_R32G32B32_SINT;
            case ShaderDataType::Int4:     return VK_FORMAT_R32G32B32A32_SINT;
            default: return VK_FORMAT_UNDEFINED;
        }
    }

    /**
     * @brief Vertex buffer element
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

        /**
         * @brief Generate Vulkan vertex input binding description
         */
        VkVertexInputBindingDescription GetBindingDescription(uint32_t binding = 0) const {
            VkVertexInputBindingDescription desc{};
            desc.binding = binding;
            desc.stride = m_Stride;
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return desc;
        }

        /**
         * @brief Generate Vulkan vertex input attribute descriptions
         */
        std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(uint32_t binding = 0) const {
            std::vector<VkVertexInputAttributeDescription> attrs;
            attrs.reserve(m_Elements.size());
            for (uint32_t i = 0; i < m_Elements.size(); i++) {
                VkVertexInputAttributeDescription attr{};
                attr.binding = binding;
                attr.location = i;
                attr.format = ShaderDataTypeToVkFormat(m_Elements[i].Type);
                attr.offset = m_Elements[i].Offset;
                attrs.push_back(attr);
            }
            return attrs;
        }

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

        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride = 0;
    };

    /**
     * @brief Vulkan vertex buffer (VkBuffer + VMA allocation)
     */
    class VertexBuffer {
    public:
        VertexBuffer(const void* data, uint32_t size);
        VertexBuffer(float* vertices, uint32_t size);
        VertexBuffer(uint32_t size); // Dynamic buffer
        ~VertexBuffer();

        VertexBuffer(const VertexBuffer&) = delete;
        VertexBuffer& operator=(const VertexBuffer&) = delete;

        void SetData(const void* data, uint32_t size);
        void Bind(VkCommandBuffer cmd) const;

        const BufferLayout& GetLayout() const { return m_Layout; }
        void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

        VkBuffer GetBuffer() const { return m_Buffer; }

    private:
        void CreateBuffer(uint32_t size, bool dynamic);
        void UploadData(const void* data, uint32_t size);

        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        uint32_t m_Size = 0;
        bool m_Dynamic = false;
        BufferLayout m_Layout;
    };

    /**
     * @brief Vulkan index buffer (VkBuffer + VMA allocation)
     */
    class IndexBuffer {
    public:
        IndexBuffer(const uint32_t* indices, uint32_t count);
        ~IndexBuffer();

        IndexBuffer(const IndexBuffer&) = delete;
        IndexBuffer& operator=(const IndexBuffer&) = delete;

        void Bind(VkCommandBuffer cmd) const;

        uint32_t GetCount() const { return m_Count; }
        VkBuffer GetBuffer() const { return m_Buffer; }

    private:
        VkBuffer m_Buffer = VK_NULL_HANDLE;
        VmaAllocation m_Allocation = VK_NULL_HANDLE;
        uint32_t m_Count = 0;
    };

    /**
     * @brief Vulkan vertex array abstraction
     *
     * Groups vertex buffers and an index buffer together.
     * In Vulkan there's no VAO concept — this just stores references
     * and provides Bind() to record vkCmdBind* commands.
     */
    class VertexArray {
    public:
        VertexArray() = default;
        ~VertexArray() = default;

        void Bind(VkCommandBuffer cmd) const;

        void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer);
        void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer);

        const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const { return m_VertexBuffers; }
        const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

    private:
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffer;
    };
}
