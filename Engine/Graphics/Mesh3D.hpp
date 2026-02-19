#pragma once

#include "../Core/Base.hpp"
#include "VertexArray.hpp"
#include "Shader.hpp"
#include "Material.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace GameEngine {

    /**
     * @brief Vertex structure for 3D meshes
     * 
     * Contains position, normal, and texture coordinates
     * Can be extended with tangent, bitangent, bone weights, etc.
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
    };

    /**
     * @brief 3D Mesh class
     * 
     * Represents a renderable 3D mesh with vertices and indices
     * 
     * Features:
     * - Dynamic and static mesh support
     * - Vertex attribute layout (position, normal, texcoords)
     * - Index buffer for efficient rendering
     * - AABB calculation for culling
     * - Normal/tangent calculation
     * 
     * Usage:
     *   std::vector<Vertex3D> vertices = { ... };
     *   std::vector<uint32_t> indices = { ... };
     *   auto mesh = CreateRef<Mesh3D>(vertices, indices);
     *   mesh->Draw();
     */
    class Mesh3D {
    public:
        /**
         * @brief Create mesh from vertices and indices
         */
        Mesh3D(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
        
        /**
         * @brief Create empty dynamic mesh
         */
        Mesh3D();
        
        ~Mesh3D() = default;
        
        /**
         * @brief Get/Set mesh name
         */
        const std::string& GetName() const { return m_Name; }
        void SetName(const std::string& name) { m_Name = name; }

        /**
         * @brief Source path (set when loaded from an asset file)
         */
        const std::string& GetSourcePath() const { return m_SourcePath; }
        void SetSourcePath(const std::string& path) { m_SourcePath = path; }
        
        /**
         * @brief Upload mesh data to GPU
         */
        void UploadData(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
        
        /**
         * @brief Update vertex data (for dynamic meshes like cloth)
         */
        void UpdateVertices(const std::vector<Vertex3D>& vertices);
        
        /**
         * @brief Draw mesh
         */
        void Draw() const;
        
        /**
         * @brief Get vertex count
         */
        uint32_t GetVertexCount() const { return m_VertexCount; }
        
        /**
         * @brief Get index count
         */
        uint32_t GetIndexCount() const { return m_IndexCount; }
        
        /**
         * @brief Get vertices (for CPU-side operations)
         */
        const std::vector<Vertex3D>& GetVertices() const { return m_Vertices; }
        
        /**
         * @brief Get indices
         */
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
        
        /**
         * @brief Calculate normals from geometry
         */
        void CalculateNormals();
        
        /**
         * @brief Calculate tangents for normal mapping
         */
        void CalculateTangents();
        
        /**
         * @brief Get axis-aligned bounding box
         */
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
        std::string m_SourcePath;    // Set when loaded from file (for serialization)
        std::vector<Vertex3D> m_Vertices;
        std::vector<uint32_t> m_Indices;
        
        Ref<VertexArray> m_VertexArray;
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;
        
        uint32_t m_VertexCount;
        uint32_t m_IndexCount;
        
        AABB m_AABB;
    };
}