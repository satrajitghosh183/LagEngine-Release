#include "Mesh3D.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <limits>

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
        
        // Create empty vertex array for dynamic meshes
        m_VertexArray = CreateRef<VertexArray>();
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
        
        // Update vertex buffer
        m_VertexBuffer->Bind();
        m_VertexBuffer->SetData(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(Vertex3D)));
        
        CalculateAABB();
    }

    void Mesh3D::SetupMesh() {
        // Create vertex array
        m_VertexArray = CreateRef<VertexArray>();
        
        // Create vertex buffer
        m_VertexBuffer = CreateRef<VertexBuffer>(
            reinterpret_cast<float*>(m_Vertices.data()),
            static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex3D))
        );
        
        // Set vertex layout
        m_VertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float3, "a_Normal" },
            { ShaderDataType::Float2, "a_TexCoords" },
            { ShaderDataType::Float3, "a_Tangent" },
            { ShaderDataType::Float3, "a_Bitangent" }
        });
        
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);
        
        // Create index buffer
        m_IndexBuffer = CreateRef<IndexBuffer>(m_Indices.data(), m_IndexCount);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);
    }

    void Mesh3D::Draw() const {
        m_VertexArray->Bind();
        glDrawElements(GL_TRIANGLES, m_IndexCount, GL_UNSIGNED_INT, nullptr);
        m_VertexArray->Unbind();
    }

    void Mesh3D::CalculateNormals() {
        // Reset normals
        for (auto& vertex : m_Vertices) {
            vertex.Normal = glm::vec3(0.0f);
        }
        
        // Accumulate face normals
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
        
        // Normalize
        for (auto& vertex : m_Vertices) {
            if (glm::length(vertex.Normal) > 0.0001f) {
                vertex.Normal = glm::normalize(vertex.Normal);
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
    }

    void Mesh3D::CalculateTangents() {
        // Reset tangents and bitangents
        for (auto& vertex : m_Vertices) {
            vertex.Tangent = glm::vec3(0.0f);
            vertex.Bitangent = glm::vec3(0.0f);
        }
        
        // Calculate tangents per face
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
            
            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            
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
        
        // Normalize and orthogonalize
        for (auto& vertex : m_Vertices) {
            // Gram-Schmidt orthogonalize
            vertex.Tangent = glm::normalize(vertex.Tangent - vertex.Normal * glm::dot(vertex.Normal, vertex.Tangent));
            
            // Calculate handedness
            if (glm::dot(glm::cross(vertex.Normal, vertex.Tangent), vertex.Bitangent) < 0.0f) {
                vertex.Tangent *= -1.0f;
            }
            
            vertex.Bitangent = glm::normalize(vertex.Bitangent);
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