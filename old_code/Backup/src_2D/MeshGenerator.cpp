// MeshGenerator.cpp

#include "MeshGenerator.hpp"
#include <cmath>

namespace MeshGenerator {

    MeshData generateSphere(int sectors, int stacks) {
        MeshData mesh;

        for (int i = 0; i <= stacks; ++i) {
            float stackAngle = glm::pi<float>() / 2 - i * glm::pi<float>() / stacks;
            float xy = cosf(stackAngle);
            float z = sinf(stackAngle);

            for (int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * 2.0f * glm::pi<float>() / sectors;
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                mesh.vertices.emplace_back(x, y, z);
            }
        }

        for (int i = 0; i < stacks; ++i) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;

            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    mesh.indices.push_back(k1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k1 + 1);
                }
                if (i != (stacks - 1)) {
                    mesh.indices.push_back(k1 + 1);
                    mesh.indices.push_back(k2);
                    mesh.indices.push_back(k2 + 1);
                }
            }
        }

        return mesh;
    }

    MeshData generateGrid(int width, int height, float spacing) {
        MeshData mesh;
        for (int y = 0; y <= height; ++y) {
            for (int x = 0; x <= width; ++x) {
                mesh.vertices.emplace_back(
                    x * spacing,
                    0.0f,
                    y * spacing
                );
            }
        }

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int i = y * (width + 1) + x;
                mesh.indices.push_back(i);
                mesh.indices.push_back(i + 1);
                mesh.indices.push_back(i + width + 1);

                mesh.indices.push_back(i + 1);
                mesh.indices.push_back(i + width + 2);
                mesh.indices.push_back(i + width + 1);
            }
        }

        return mesh;
    }

}

