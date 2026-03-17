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
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        float cylinderHeight = height - 2.0f * radius;
        if (cylinderHeight < 0) cylinderHeight = 0;
        float halfHeight = cylinderHeight * 0.5f;
        
        // Top hemisphere
        for (int ring = 0; ring <= rings; ring++) {
            float phi = glm::half_pi<float>() * float(ring) / float(rings);
            float y = radius * cos(phi) + halfHeight;
            float r = radius * sin(phi);
            
            for (int seg = 0; seg <= segments; seg++) {
                float theta = 2.0f * glm::pi<float>() * float(seg) / float(segments);
                
                Vertex3D vertex;
                vertex.Position = glm::vec3(r * cos(theta), y, r * sin(theta));
                vertex.Normal = glm::normalize(glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));
                vertex.TexCoords = glm::vec2(float(seg) / float(segments), 1.0f - float(ring) / float(2 * rings + 1));
                
                vertices.push_back(vertex);
            }
        }
        
        // Cylinder part
        for (int ring = 0; ring <= 1; ring++) {
            float y = halfHeight - cylinderHeight * float(ring);
            
            for (int seg = 0; seg <= segments; seg++) {
                float theta = 2.0f * glm::pi<float>() * float(seg) / float(segments);
                
                Vertex3D vertex;
                vertex.Position = glm::vec3(radius * cos(theta), y, radius * sin(theta));
                vertex.Normal = glm::normalize(glm::vec3(cos(theta), 0, sin(theta)));
                vertex.TexCoords = glm::vec2(float(seg) / float(segments), 0.5f - float(ring) * 0.1f);
                
                vertices.push_back(vertex);
            }
        }
        
        // Bottom hemisphere
        for (int ring = 0; ring <= rings; ring++) {
            float phi = glm::half_pi<float>() + glm::half_pi<float>() * float(ring) / float(rings);
            float y = radius * cos(phi) - halfHeight;
            float r = radius * sin(phi);
            
            for (int seg = 0; seg <= segments; seg++) {
                float theta = 2.0f * glm::pi<float>() * float(seg) / float(segments);
                
                Vertex3D vertex;
                vertex.Position = glm::vec3(r * cos(theta), y, r * sin(theta));
                vertex.Normal = glm::normalize(glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)));
                vertex.TexCoords = glm::vec2(float(seg) / float(segments), float(ring) / float(2 * rings + 1));
                
                vertices.push_back(vertex);
            }
        }
        
        // Generate indices
        int rowSize = segments + 1;
        int totalRows = rings + 2 + rings + 1;
        
        for (int row = 0; row < totalRows - 1; row++) {
            for (int seg = 0; seg < segments; seg++) {
                int current = row * rowSize + seg;
                int next = (row + 1) * rowSize + seg;
                
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
    
    Ref<Mesh3D> MeshGenerator3D::CreateCone(float radius, float height, int segments) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        float halfHeight = height * 0.5f;
        
        // Tip vertex
        vertices.push_back(Vertex3D(glm::vec3(0, halfHeight, 0), glm::vec3(0, 1, 0), glm::vec2(0.5f, 1.0f)));
        
        // Side vertices
        float slopeAngle = atan2(radius, height);
        float normalY = cos(slopeAngle);
        float normalXZ = sin(slopeAngle);
        
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), -halfHeight, radius * sin(theta));
            vertex.Normal = glm::normalize(glm::vec3(normalXZ * cos(theta), normalY, normalXZ * sin(theta)));
            vertex.TexCoords = glm::vec2(float(i) / float(segments), 0.0f);
            
            vertices.push_back(vertex);
        }
        
        // Bottom cap center
        vertices.push_back(Vertex3D(glm::vec3(0, -halfHeight, 0), glm::vec3(0, -1, 0), glm::vec2(0.5f, 0.5f)));
        
        // Bottom cap edge
        for (int i = 0; i <= segments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(segments);
            
            Vertex3D vertex;
            vertex.Position = glm::vec3(radius * cos(theta), -halfHeight, radius * sin(theta));
            vertex.Normal = glm::vec3(0, -1, 0);
            vertex.TexCoords = glm::vec2(0.5f + 0.5f * cos(theta), 0.5f + 0.5f * sin(theta));
            
            vertices.push_back(vertex);
        }
        
        // Side triangles
        for (int i = 0; i < segments; i++) {
            indices.push_back(0);
            indices.push_back(i + 1);
            indices.push_back(i + 2);
        }
        
        // Bottom cap
        int bottomCenter = segments + 2;
        int bottomStart = bottomCenter + 1;
        for (int i = 0; i < segments; i++) {
            indices.push_back(bottomCenter);
            indices.push_back(bottomStart + i + 1);
            indices.push_back(bottomStart + i);
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }
    
    Ref<Mesh3D> MeshGenerator3D::CreateTorus(float majorRadius, float minorRadius, int majorSegments, int minorSegments) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        for (int i = 0; i <= majorSegments; i++) {
            float theta = 2.0f * glm::pi<float>() * float(i) / float(majorSegments);
            float cosTheta = cos(theta);
            float sinTheta = sin(theta);
            
            for (int j = 0; j <= minorSegments; j++) {
                float phi = 2.0f * glm::pi<float>() * float(j) / float(minorSegments);
                float cosPhi = cos(phi);
                float sinPhi = sin(phi);
                
                Vertex3D vertex;
                
                float x = (majorRadius + minorRadius * cosPhi) * cosTheta;
                float y = minorRadius * sinPhi;
                float z = (majorRadius + minorRadius * cosPhi) * sinTheta;
                
                vertex.Position = glm::vec3(x, y, z);
                vertex.Normal = glm::normalize(glm::vec3(cosPhi * cosTheta, sinPhi, cosPhi * sinTheta));
                vertex.TexCoords = glm::vec2(float(i) / float(majorSegments), float(j) / float(minorSegments));
                
                vertices.push_back(vertex);
            }
        }
        
        // Generate indices
        for (int i = 0; i < majorSegments; i++) {
            for (int j = 0; j < minorSegments; j++) {
                int current = i * (minorSegments + 1) + j;
                int next = (i + 1) * (minorSegments + 1) + j;
                
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
    
    Ref<Mesh3D> MeshGenerator3D::CreateQuad(float width, float height) {
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        
        std::vector<Vertex3D> vertices = {
            { glm::vec3(-halfWidth, -halfHeight, 0), glm::vec3(0, 0, 1), glm::vec2(0, 0) },
            { glm::vec3( halfWidth, -halfHeight, 0), glm::vec3(0, 0, 1), glm::vec2(1, 0) },
            { glm::vec3( halfWidth,  halfHeight, 0), glm::vec3(0, 0, 1), glm::vec2(1, 1) },
            { glm::vec3(-halfWidth,  halfHeight, 0), glm::vec3(0, 0, 1), glm::vec2(0, 1) }
        };
        
        std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
        
        return CreateRef<Mesh3D>(vertices, indices);
    }
    
    Ref<Mesh3D> MeshGenerator3D::CreateUVSphere(float radius, int latitudes, int longitudes) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        // Add top pole vertex
        vertices.push_back(Vertex3D(glm::vec3(0, radius, 0), glm::vec3(0, 1, 0), glm::vec2(0.5f, 1.0f)));
        
        // Add middle vertices
        for (int lat = 1; lat < latitudes; lat++) {
            float theta = glm::pi<float>() * float(lat) / float(latitudes);
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            
            for (int lon = 0; lon <= longitudes; lon++) {
                float phi = 2.0f * glm::pi<float>() * float(lon) / float(longitudes);
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);
                
                Vertex3D vertex;
                vertex.Position = glm::vec3(
                    radius * sinTheta * cosPhi,
                    radius * cosTheta,
                    radius * sinTheta * sinPhi
                );
                vertex.Normal = glm::normalize(vertex.Position);
                vertex.TexCoords = glm::vec2(
                    float(lon) / float(longitudes),
                    1.0f - float(lat) / float(latitudes)
                );
                
                vertices.push_back(vertex);
            }
        }
        
        // Add bottom pole vertex
        vertices.push_back(Vertex3D(glm::vec3(0, -radius, 0), glm::vec3(0, -1, 0), glm::vec2(0.5f, 0.0f)));
        
        // Generate indices for top cap
        for (int lon = 0; lon < longitudes; lon++) {
            indices.push_back(0);
            indices.push_back(lon + 1);
            indices.push_back(lon + 2);
        }
        
        // Generate indices for middle quads
        int rowSize = longitudes + 1;
        for (int lat = 0; lat < latitudes - 2; lat++) {
            for (int lon = 0; lon < longitudes; lon++) {
                int current = 1 + lat * rowSize + lon;
                int next = current + rowSize;
                
                indices.push_back(current);
                indices.push_back(next);
                indices.push_back(current + 1);
                
                indices.push_back(current + 1);
                indices.push_back(next);
                indices.push_back(next + 1);
            }
        }
        
        // Generate indices for bottom cap
        int bottomPole = static_cast<int>(vertices.size()) - 1;
        int lastRowStart = 1 + (latitudes - 2) * rowSize;
        for (int lon = 0; lon < longitudes; lon++) {
            indices.push_back(bottomPole);
            indices.push_back(lastRowStart + lon + 1);
            indices.push_back(lastRowStart + lon);
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }
    
    Ref<Mesh3D> MeshGenerator3D::CreateIcoSphere(float radius, int subdivisions) {
        std::vector<Vertex3D> vertices;
        std::vector<uint32_t> indices;
        
        // Golden ratio
        float t = (1.0f + sqrt(5.0f)) / 2.0f;
        
        // Create initial icosahedron vertices (normalized)
        std::vector<glm::vec3> positions = {
            glm::normalize(glm::vec3(-1,  t,  0)),
            glm::normalize(glm::vec3( 1,  t,  0)),
            glm::normalize(glm::vec3(-1, -t,  0)),
            glm::normalize(glm::vec3( 1, -t,  0)),
            glm::normalize(glm::vec3( 0, -1,  t)),
            glm::normalize(glm::vec3( 0,  1,  t)),
            glm::normalize(glm::vec3( 0, -1, -t)),
            glm::normalize(glm::vec3( 0,  1, -t)),
            glm::normalize(glm::vec3( t,  0, -1)),
            glm::normalize(glm::vec3( t,  0,  1)),
            glm::normalize(glm::vec3(-t,  0, -1)),
            glm::normalize(glm::vec3(-t,  0,  1))
        };
        
        // Initial icosahedron faces
        std::vector<std::tuple<int, int, int>> faces = {
            {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
            {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
            {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
            {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
        };
        
        // Subdivide
        std::unordered_map<uint64_t, int> midpointCache;
        auto getMidpoint = [&](int i1, int i2) -> int {
            uint64_t key = (static_cast<uint64_t>(std::min(i1, i2)) << 32) + std::max(i1, i2);
            auto it = midpointCache.find(key);
            if (it != midpointCache.end()) {
                return it->second;
            }
            
            glm::vec3 mid = glm::normalize((positions[i1] + positions[i2]) * 0.5f);
            int index = static_cast<int>(positions.size());
            positions.push_back(mid);
            midpointCache[key] = index;
            return index;
        };
        
        for (int s = 0; s < subdivisions; s++) {
            std::vector<std::tuple<int, int, int>> newFaces;
            
            for (const auto& [i0, i1, i2] : faces) {
                int a = getMidpoint(i0, i1);
                int b = getMidpoint(i1, i2);
                int c = getMidpoint(i2, i0);
                
                newFaces.push_back({i0, a, c});
                newFaces.push_back({i1, b, a});
                newFaces.push_back({i2, c, b});
                newFaces.push_back({a, b, c});
            }
            
            faces = newFaces;
        }
        
        // Create vertices
        for (const auto& pos : positions) {
            Vertex3D vertex;
            vertex.Position = pos * radius;
            vertex.Normal = pos;
            
            // Calculate UV from spherical coordinates
            float u = 0.5f + atan2(pos.z, pos.x) / (2.0f * glm::pi<float>());
            float v = 0.5f - asin(pos.y) / glm::pi<float>();
            vertex.TexCoords = glm::vec2(u, v);
            
            vertices.push_back(vertex);
        }
        
        // Create indices
        for (const auto& [i0, i1, i2] : faces) {
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);
        }
        
        return CreateRef<Mesh3D>(vertices, indices);
    }
}