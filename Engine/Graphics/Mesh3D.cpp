#include "Mesh3D.hpp"
#include "../Core/Logger.hpp"
#include <limits>
#include <cmath>

namespace GameEngine {

    Mesh3D::Mesh3D(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices)
        : m_Vertices(vertices)
        , m_Indices(indices)
        , m_VertexCount(static_cast<uint32_t>(vertices.size()))
        , m_IndexCount(static_cast<uint32_t>(indices.size())) {

        SetupMesh();
        CalculateAABB();
    }

    Mesh3D::Mesh3D()
        : m_VertexCount(0)
        , m_IndexCount(0) {
    }

    void Mesh3D::UploadData(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) {
        m_Vertices = vertices;
        m_Indices = indices;
        m_VertexCount = static_cast<uint32_t>(vertices.size());
        m_IndexCount = static_cast<uint32_t>(indices.size());

        SetupMesh();
        CalculateAABB();
    }

    void Mesh3D::UpdateVertices(const std::vector<Vertex3D>& vertices) {
        GE_CORE_ASSERT(vertices.size() == m_Vertices.size(), "Vertex count mismatch!");

        m_Vertices = vertices;

        // Update vertex buffer data
        if (m_VertexBuffer) {
            m_VertexBuffer->SetData(vertices.data(),
                                     static_cast<uint32_t>(vertices.size() * sizeof(Vertex3D)));
        }

        CalculateAABB();
    }

    void Mesh3D::SetupMesh() {
        if (m_Vertices.empty()) return;

        // Create vertex buffer (static, device-local via staging)
        m_VertexBuffer = CreateRef<VertexBuffer>(
            m_Vertices.data(),
            static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex3D))
        );

        m_VertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoords" },
            { ShaderDataType::Float3, "a_Tangent" },
            { ShaderDataType::Float3, "a_Bitangent" }
        });

        // Create index buffer
        if (!m_Indices.empty()) {
            m_IndexBuffer = CreateRef<IndexBuffer>(m_Indices.data(), m_IndexCount);
        }
    }

    void Mesh3D::Draw(VkCommandBuffer cmd) const {
        if (!m_VertexBuffer) return;

        Bind(cmd);

        if (m_IndexBuffer && m_IndexCount > 0) {
            vkCmdDrawIndexed(cmd, m_IndexCount, 1, 0, 0, 0);
        } else if (m_VertexCount > 0) {
            vkCmdDraw(cmd, m_VertexCount, 1, 0, 0);
        }
    }

    void Mesh3D::Bind(VkCommandBuffer cmd) const {
        if (!m_VertexBuffer) return;

        m_VertexBuffer->Bind(cmd);
        if (m_IndexBuffer) {
            m_IndexBuffer->Bind(cmd);
        }
    }

    void Mesh3D::DrawInstanced(VkCommandBuffer cmd, uint32_t instanceCount) const {
        if (!m_VertexBuffer) return;

        Bind(cmd);

        if (m_IndexBuffer && m_IndexCount > 0) {
            vkCmdDrawIndexed(cmd, m_IndexCount, instanceCount, 0, 0, 0);
        } else if (m_VertexCount > 0) {
            vkCmdDraw(cmd, m_VertexCount, instanceCount, 0, 0);
        }
    }

    void Mesh3D::CalculateNormals() {
        for (auto& vertex : m_Vertices) {
            vertex.Normal = glm::vec3(0.0f);
        }

        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            uint32_t i0 = m_Indices[i];
            uint32_t i1 = m_Indices[i + 1];
            uint32_t i2 = m_Indices[i + 2];

            glm::vec3& v0 = m_Vertices[i0].Position;
            glm::vec3& v1 = m_Vertices[i1].Position;
            glm::vec3& v2 = m_Vertices[i2].Position;

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 normal = glm::cross(edge1, edge2);

            m_Vertices[i0].Normal += normal;
            m_Vertices[i1].Normal += normal;
            m_Vertices[i2].Normal += normal;
        }

        for (auto& vertex : m_Vertices) {
            if (glm::length(vertex.Normal) > 0.0001f) {
                vertex.Normal = glm::normalize(vertex.Normal);
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
    }

    void Mesh3D::CalculateTangents() {
        for (auto& vertex : m_Vertices) {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }

        for (size_t i = 0; i < m_Indices.size(); i += 3) {
            uint32_t i0 = m_Indices[i];
            uint32_t i1 = m_Indices[i + 1];
            uint32_t i2 = m_Indices[i + 2];

            Vertex3D& v0 = m_Vertices[i0];
            Vertex3D& v1 = m_Vertices[i1];
            Vertex3D& v2 = m_Vertices[i2];

            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;

            glm::vec2 deltaUV1 = v1.TexCoords - v0.TexCoords;
            glm::vec2 deltaUV2 = v2.TexCoords - v0.TexCoords;

            float denom = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
            if (std::abs(denom) < 1e-8f) continue;
            float f = 1.0f / denom;

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            glm::vec3 bitangent;
            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

            v0.Tangent += tangent;
            v1.Tangent += tangent;
            v2.Tangent += tangent;

            v0.Bitangent += bitangent;
            v1.Bitangent += bitangent;
            v2.Bitangent += bitangent;
        }

        for (auto& vertex : m_Vertices) {
            float tangentLen = glm::length(vertex.Tangent);
            if (tangentLen < 1e-6f) {
                glm::vec3 up = (std::abs(vertex.Normal.y) < 0.999f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
                vertex.Tangent = glm::normalize(glm::cross(up, vertex.Normal));
                vertex.Bitangent = glm::cross(vertex.Normal, vertex.Tangent);
                continue;
            }

            vertex.Tangent = glm::normalize(vertex.Tangent - vertex.Normal * glm::dot(vertex.Normal, vertex.Tangent));

            if (glm::dot(glm::cross(vertex.Normal, vertex.Tangent), vertex.Bitangent) < 0.0f) {
                vertex.Tangent *= -1.0f;
            }

            float bitangentLen = glm::length(vertex.Bitangent);
            if (bitangentLen > 1e-6f) {
                vertex.Bitangent = vertex.Bitangent / bitangentLen;
            } else {
                vertex.Bitangent = glm::cross(vertex.Normal, vertex.Tangent);
            }
        }
    }

    void Mesh3D::CalculateAABB() {
        if (m_Vertices.empty()) {
            m_AABB.Min = glm::vec3(0.0f);
            m_AABB.Max = glm::vec3(0.0f);
            return;
        }

        m_AABB.Min = glm::vec3(std::numeric_limits<float>::max());
        m_AABB.Max = glm::vec3(std::numeric_limits<float>::lowest());

        for (const auto& vertex : m_Vertices) {
            m_AABB.Min = glm::min(m_AABB.Min, vertex.Position);
            m_AABB.Max = glm::max(m_AABB.Max, vertex.Position);
        }
    }
}
