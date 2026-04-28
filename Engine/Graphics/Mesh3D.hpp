#pragma once

#include "../Core/Base.hpp"
#include "VertexArray.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace GameEngine {

    /**
     * @brief Vertex structure for 3D meshes
     */
    struct Vertex3D {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;

        Vertex3D()
            : Position(0.0f), Normal(0.0f, 1.0f, 0.0f), TexCoords(0.0f),
              Tangent(0.0f), Bitangent(0.0f) {}

        Vertex3D(const glm::vec3& pos, const glm::vec3& normal, const glm::vec2& texCoords)
            : Position(pos), Normal(normal), TexCoords(texCoords),
              Tangent(0.0f), Bitangent(0.0f) {}

        /**
         * @brief Get Vulkan vertex input binding description
         */
        static VkVertexInputBindingDescription GetBindingDescription() {
            VkVertexInputBindingDescription desc{};
            desc.binding = 0;
            desc.stride = sizeof(Vertex3D);
            desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return desc;
        }

        /**
         * @brief Get Vulkan vertex input attribute descriptions
         */
        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() {
            std::vector<VkVertexInputAttributeDescription> attrs(5);

            // Position
            attrs[0].binding = 0;
            attrs[0].location = 0;
            attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[0].offset = offsetof(Vertex3D, Position);

            // Normal
            attrs[1].binding = 0;
            attrs[1].location = 1;
            attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[1].offset = offsetof(Vertex3D, Normal);

            // TexCoords
            attrs[2].binding = 0;
            attrs[2].location = 2;
            attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
            attrs[2].offset = offsetof(Vertex3D, TexCoords);

            // Tangent
            attrs[3].binding = 0;
            attrs[3].location = 3;
            attrs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[3].offset = offsetof(Vertex3D, Tangent);

            // Bitangent
            attrs[4].binding = 0;
            attrs[4].location = 4;
            attrs[4].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrs[4].offset = offsetof(Vertex3D, Bitangent);

            return attrs;
        }
    };

    /**
     * @brief 3D Mesh class (Vulkan)
     *
     * Stores vertices and indices in device-local VkBuffers via VMA.
     * Draw() records vkCmdBindVertexBuffers + vkCmdDrawIndexed to a
     * command buffer.
     */
    class Mesh3D {
    public:
        Mesh3D(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
        Mesh3D();
        ~Mesh3D() = default;

        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        const std::string& GetSourcePath() const { return m_SourcePath; }
        void SetSourcePath(const std::string& path) { m_SourcePath = path; }

        void UploadData(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
        void UpdateVertices(const std::vector<Vertex3D>& vertices);

        /**
         * @brief Record draw commands to a command buffer
         */
        void Draw(VkCommandBuffer cmd) const;

        /**
         * @brief Bind vertex and index buffers to command buffer (without drawing)
         */
        void Bind(VkCommandBuffer cmd) const;

        /**
         * @brief Draw with instancing
         */
        void DrawInstanced(VkCommandBuffer cmd, uint32_t instanceCount) const;

        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }

        const std::vector<Vertex3D>& GetVertices() const { return m_Vertices; }
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

        void CalculateNormals();
        void CalculateTangents();

        struct AABB {
            glm::vec3 Min;
            glm::vec3 Max;

            glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
            glm::vec3 GetExtents() const { return (Max - Min) * 0.5f; }
        };

        AABB GetAABB() const { return m_AABB; }

    private:
        void SetupMesh();
        void CalculateAABB();

    private:
        std::string m_Name;
        std::string m_SourcePath;
        std::vector<Vertex3D> m_Vertices;
        std::vector<uint32_t> m_Indices;

        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;

        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        AABB m_AABB;
    };
}
