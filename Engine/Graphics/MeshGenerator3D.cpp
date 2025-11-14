#include "MeshGenerator3D.hpp"
#include <glm/gtc/constants.hpp>

namespace GameEngine {

    Ref<Mesh3D> MeshGenerator3D::CreateCube(float size) {
        float halfSize = size * 0.5f;
        
        std::vector<Vertex3D> vertices = {
            // Front face
            { glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(0, 0, 1), glm::vec2(0, 0) },
            { glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3(0, 0, 1), glm::vec2(1, 0) },
            { glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3(0, 0, 1), glm::vec2(1, 1) },
            { glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(0, 0, 1), glm::vec2(0, 1) },
            
            // Back face
            { glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(0, 0) },
            { glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(1, 0) },
            { glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(1, 1) },
            { glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(0, 0, -1), glm::vec2(0, 1) },
            
            // Top face
            { glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(0, 1, 0), glm::vec2(0, 0) },
            { glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3(0, 1, 0), glm::vec2(1, 0) },
            { glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(0, 1, 0), glm::vec2(1, 1) },
            { glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(0, 1, 0), glm::vec2(0, 1) },
            
            // Bottom face
            { glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(0, -1, 0), glm::vec2(0, 0) },
            { glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(0, -1, 0), glm::vec2(1, 0) },
            { glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3(0, -1, 0), glm::vec2(1, 1) },
            { glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(0, -1, 0), glm::vec2(0, 1) },
            
            // Right face
            { glm::vec3( halfSize, -halfSize,  halfSize), glm::vec3(1, 0, 0), glm::vec2(0, 0) },
            { glm::vec3( halfSize, -halfSize, -halfSize), glm::vec3(1, 0, 0), glm::vec2(1, 0) },
            { glm::vec3( halfSize,  halfSize, -halfSize), glm::vec3(1, 0, 0), glm::vec2(1, 1) },
            { glm::vec3( halfSize,  halfSize,  halfSize), glm::vec3(1, 0, 0), glm::vec2(0, 1) },
            
            // Left face
            { glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(-1, 0, 0), glm::vec2(0, 0) },
            { glm::vec3(-halfSize, -halfSize,  halfSize), glm::vec3(-1, 0, 0), glm::vec2(1, 0) },
            { glm::vec3(-halfSize,  halfSize,  halfSize), glm::vec3(-1, 0, 0), glm::vec2(1, 1) },
            { glm::vec3(-halfSize,  halfSize, -halfSize), glm::vec3(-1, 0, 0), glm::vec2(0, 1) }
        };
        
        std::vector<uint32_t> indices = {
            0, 1, 2,  2, 3, 0,      // Front
            4, 5, 6,  6, 7, 4,      // Back
            8, 9, 10, 10, 11, 8,    // Top
            12, 13, 14, 14, 15, 12, // Bottom
            16, 17, 18, 18, 19, 16, // Right
            20, 21, 22, 22, 23, 20  // Left
        };
        
        return CreateRef<Mesh3D>(vertices, indices);
    }

    Ref<Mesh3D> MeshGenerator3D::CreateSphere(float radius, int segments, int rings) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        for (int ring = 0; ring <= rings; ring++) {
            float phi = glm::pi<float>() * float(ring) / float(rings);
            
            for (int segment = 0; segment <= segments; segment++) {
                float theta = 2.0f * glm::pi<float>() * float(segment) / float(segments);
                
                Vertex3D vertex;
                vertex.Position.x = radius * sin(phi) * cos(theta);
                vertex.Position.y = radius * cos(phi);
                vertex.Position.z = radius * sin(phi) * sin(theta);
                
                vertex.Normal = glm::normalize(vertex.Position);
                
                vertex.TexCoords.x = float(segment) / float(segments);
                vertex.TexCoords.y = float(ring) / float(rings);
                
                vertices.push_back(vertex);
            }
        }
        
        for (int ring = 0; ring < rings; ring++) {
            for (int segment = 0; segment < segments; segment++) {
                int current = ring * (segments + 1) + segment;
                int next = current + segments + 1;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }

    Ref<Mesh3D> MeshGenerator3D::CreatePlane(float width, float height, int subdivisionsX, int subdivisionsY) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        
        for (int y = 0; y <= subdivisionsY; y++) {
            for (int x = 0; x <= subdivisionsX; x++) {
                Vertex3D vertex;
                
                float u = float(x) / float(subdivisionsX);
                float v = float(y) / float(subdivisionsY);
                
                vertex.Position = glm::vec3(
                    -halfWidth + width * u,
                    0.0f,
                    -halfHeight + height * v
                );
                
                vertex.Normal = glm::vec3(0, 1, 0);
                vertex.TexCoords = glm::vec2(u, v);
                
                vertices.push_back(vertex);
            }
        }
        
        for (int y = 0; y < subdivisionsY; y++) {
            for (int x = 0; x < subdivisionsX; x++) {
                int topLeft = y * (subdivisionsX + 1) + x;
                int topRight = topLeft + 1;
                int bottomLeft = (y + 1) * (subdivisionsX + 1) + x;
                int bottomRight = bottomLeft + 1;
                
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);
                
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }

    Ref<Mesh3D> MeshGenerator3D::CreateCylinder(float radius, float height, int segments) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        float halfHeight = height * 0.5f;
        
        // Top cap center
        vertices.push_back(Vertex3D(glm::vec3(0, halfHeight, 0), glm::vec3(0, 1, 0), glm::vec2(0.5f, 0.5f)));
        
        // Top cap edge
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), halfHeight, radius * sin(theta));
            vertex.Normal = glm::vec3(0, 1, 0);
            vertex.TexCoords = glm::vec2(0.5f + 0.5f * cos(theta), 0.5f + 0.5f * sin(theta));
            
            vertices.push_back(vertex);
        }
        
        // Side vertices (top)
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), halfHeight, radius * sin(theta));
            vertex.Normal = glm::normalize(glm::vec3(cos(theta), 0, sin(theta)));
            vertex.TexCoords = glm::vec2(float(i) / float(segments), 1.0f);
            
            vertices.push_back(vertex);
        }
        
        // Side vertices (bottom)
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), -halfHeight, radius * sin(theta));
            vertex.Normal = glm::normalize(glm::vec3(cos(theta), 0, sin(theta)));
            vertex.TexCoords = glm::vec2(float(i) / float(segments), 0.0f);
            
            vertices.push_back(vertex);
        }
        
        // Bottom cap edge
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), -halfHeight, radius * sin(theta));
            vertex.Normal = glm::vec3(0, -1, 0);
            vertex.TexCoords = glm::vec2(0.5f + 0.5f * cos(theta), 0.5f + 0.5f * sin(theta));
            
            vertices.push_back(vertex);
        }
        
        // Bottom cap center
        vertices.push_back(Vertex3D(glm::vec3(0, -halfHeight, 0), glm::vec3(0, -1, 0), glm::vec2(0.5f, 0.5f)));
        
        // Top cap indices
        for (int i = 1; i <= segments; i++) {
            indices.push_back(0);
            indices.push_back(i);
            indices.push_back(i + 1);
        }
        
        // Side indices
        int sideStart = segments + 2;
        for (int i = 0; i < segments; i++) {
            int topLeft = sideStart + i;
            int topRight = sideStart + i + 1;
            int bottomLeft = sideStart + segments + 1 + i;
            int bottomRight = sideStart + segments + 1 + i + 1;
            
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
        
        // Bottom cap indices
        int bottomStart = sideStart + 2 * (segments + 1);
        int bottomCenter = bottomStart + segments + 1;
        
        for (int i = 0; i < segments; i++) {
            indices.push_back(bottomCenter);
            indices.push_back(bottomStart + i + 1);
            indices.push_back(bottomStart + i);
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }

    Ref<Mesh3D> MeshGenerator3D::CreateCapsule(float radius, float height, int segments, int rings) {
        // Simplified capsule: cylinder with hemisphere caps
        // TODO: Implement proper capsule mesh generation
        return CreateCylinder(radius, height, segments);
    }
}