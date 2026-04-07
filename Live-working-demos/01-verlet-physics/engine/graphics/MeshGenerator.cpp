#include "engine/graphics/MeshGenerator.hpp"
#include <glm/gtc/constants.hpp>
#include <cmath>

namespace engine::graphics {

Mesh MeshGenerator::Grid(int width, int height, float spacing) {
    Mesh mesh;

    for (int z = 0; z < height; ++z) {
        for (int x = 0; x < width; ++x) {
            mesh.vertices.emplace_back(x * spacing, 0.0f, z * spacing);
        }
    }

    for (int z = 0; z < height - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int topLeft = z * width + x;
            int topRight = topLeft + 1;
            int bottomLeft = topLeft + width;
            int bottomRight = bottomLeft + 1;

            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    mesh.upload();
    return mesh;
}

Mesh MeshGenerator::Sphere(float radius, int stacks, int slices) {
    Mesh mesh;

    for (int i = 0; i <= stacks; ++i) {
        float v = (float)i / stacks;
        float phi = v * glm::pi<float>();

        for (int j = 0; j <= slices; ++j) {
            float u = (float)j / slices;
            float theta = u * glm::two_pi<float>();

            float x = radius * std::sin(phi) * std::cos(theta);
            float y = radius * std::cos(phi);
            float z = radius * std::sin(phi) * std::sin(theta);

            mesh.vertices.emplace_back(x, y, z);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            int p1 = i * (slices + 1) + j;
            int p2 = p1 + slices + 1;

            mesh.indices.push_back(p1);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p1 + 1);

            mesh.indices.push_back(p1 + 1);
            mesh.indices.push_back(p2);
            mesh.indices.push_back(p2 + 1);
        }
    }

    mesh.upload();
    return mesh;
}

Mesh MeshGenerator::Plane(float size) {
    Mesh mesh;

    mesh.vertices = {
        {-size / 2, 0, -size / 2},
        { size / 2, 0, -size / 2},
        { size / 2, 0,  size / 2},
        {-size / 2, 0,  size / 2}
    };

    mesh.indices = {
        0, 1, 2,
        2, 3, 0
    };

    mesh.upload();
    return mesh;
}

} // namespace engine::graphics