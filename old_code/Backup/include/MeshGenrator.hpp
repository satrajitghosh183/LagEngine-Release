// MeshGenerator.hpp

#pragma once
#include <vector>
#include <glm/glm.hpp>

namespace MeshGenerator {

    struct MeshData {
        std::vector<glm::vec3> vertices;
        std::vector<unsigned int> indices;
    };

    // Sphere generator (centered at origin, radius 1.0)
    MeshData generateSphere(int sectors = 16, int stacks = 16);

    // Grid (cloth-like) generator
    MeshData generateGrid(int width, int height, float spacing = 1.0f);

}
