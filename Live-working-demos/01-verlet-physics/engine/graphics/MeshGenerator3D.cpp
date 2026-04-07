
#include "MeshGenerator3D.hpp"

namespace engine::graphics {

void MeshGenerator3D::generateClothMesh(
    int width,
    int height,
    const std::vector<std::shared_ptr<engine::physics::Particle3D>>& particles,
    std::vector<Vertex3D>& outVertices,
    std::vector<unsigned int>& outIndices
) {
    outVertices.clear();
    outIndices.clear();

    // Step 1: Copy particle positions into vertex buffer
    for (int j = 0; j <= height; ++j) {
        for (int i = 0; i <= width; ++i) {
            Vertex3D v;
            v.position = particles[j * (width + 1) + i]->getPosition();
            v.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Temporary upward normal
            v.texCoord = glm::vec2(
                static_cast<float>(i) / static_cast<float>(width),
                static_cast<float>(j) / static_cast<float>(height)
            ); // Properly spread UVs from (0,0) to (1,1)

            outVertices.push_back(v);
        }
    }

    // Step 2: Build index buffer
    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int i0 = j * (width + 1) + i;
            int i1 = i0 + 1;
            int i2 = i0 + (width + 1);
            int i3 = i2 + 1;

            // First triangle
            outIndices.push_back(i0);
            outIndices.push_back(i2);
            outIndices.push_back(i1);

            // Second triangle
            outIndices.push_back(i1);
            outIndices.push_back(i2);
            outIndices.push_back(i3);
        }
    }
}

} // namespace engine::graphics
