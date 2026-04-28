//
// Created by barth on 19/09/2022.
//
// Vulkan port: GPU buffers are created via VulkanBase helpers.
// The interleaved vertex layout is: [pos.x, pos.y, pos.z, norm.x, norm.y, norm.z, col.r, col.g, col.b]
// per vertex (9 floats, 36 bytes stride).
//

#include "Mesh.h"
#include <iostream>
#include <cstring>

std::shared_ptr<Material> Mesh::defaultMaterial = std::make_shared<DefaultMaterial>();
vkdemo::VulkanBase* Mesh::s_VulkanBase = nullptr;

Mesh::Mesh(const char *name) : Transformable(), Renderable(), _name(name) {
    _id = UUID::generate_uuid_v4();
    _material = Mesh::defaultMaterial;
}

void Mesh::setVertexData(VertexData &vertexData) {
    _vertexData = vertexData;
    _gpuDirty = true;
}

/// Build an interleaved vertex buffer and upload to GPU.
/// Layout per vertex: pos(3) + normal(3) + color(3) = 9 floats.
void Mesh::sendVertexDataToGPU() {
    if (!s_VulkanBase) return;

    uint32_t numVerts = static_cast<uint32_t>(_vertexData.positions.size() / 3);
    if (numVerts == 0) return;

    // Build interleaved data
    const size_t floatsPerVert = 9; // pos(3) + normal(3) + color(3)
    std::vector<float> interleaved(numVerts * floatsPerVert);
    for (uint32_t i = 0; i < numVerts; i++) {
        size_t base = i * floatsPerVert;
        // Position
        interleaved[base + 0] = _vertexData.positions[i * 3 + 0];
        interleaved[base + 1] = _vertexData.positions[i * 3 + 1];
        interleaved[base + 2] = _vertexData.positions[i * 3 + 2];
        // Normal
        if (i * 3 + 2 < _vertexData.normals.size()) {
            interleaved[base + 3] = _vertexData.normals[i * 3 + 0];
            interleaved[base + 4] = _vertexData.normals[i * 3 + 1];
            interleaved[base + 5] = _vertexData.normals[i * 3 + 2];
        } else {
            interleaved[base + 3] = 0.0f;
            interleaved[base + 4] = 1.0f;
            interleaved[base + 5] = 0.0f;
        }
        // Color
        if (i * 3 + 2 < _vertexData.colors.size()) {
            interleaved[base + 6] = _vertexData.colors[i * 3 + 0];
            interleaved[base + 7] = _vertexData.colors[i * 3 + 1];
            interleaved[base + 8] = _vertexData.colors[i * 3 + 2];
        } else {
            interleaved[base + 6] = 1.0f;
            interleaved[base + 7] = 1.0f;
            interleaved[base + 8] = 1.0f;
        }
    }

    VkDeviceSize vbSize = interleaved.size() * sizeof(float);
    VkDeviceSize ibSize = _vertexData.indices.size() * sizeof(int32_t);

    // Destroy old buffers
    if (_vertexBuffer.Buffer != VK_NULL_HANDLE)
        s_VulkanBase->DestroyBuffer(_vertexBuffer);
    if (_indexBuffer.Buffer != VK_NULL_HANDLE)
        s_VulkanBase->DestroyBuffer(_indexBuffer);

    // Create new buffers using host-visible memory for frequent updates
    _vertexBuffer = s_VulkanBase->CreateBuffer(
        vbSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    s_VulkanBase->CopyToBuffer(_vertexBuffer, interleaved.data(), vbSize);

    // Convert int32_t indices to uint32_t for Vulkan
    std::vector<uint32_t> indices32(_vertexData.indices.begin(), _vertexData.indices.end());
    _indexBuffer = s_VulkanBase->CreateBuffer(
        ibSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    s_VulkanBase->CopyToBuffer(_indexBuffer, indices32.data(), ibSize);

    _gpuDirty = false;
}

void Mesh::setMaterial(std::shared_ptr<Material> material) {
    _material = material;
}

void Mesh::render(glm::mat4 projectionViewMatrix, Shader *shaderOverride) {
    // In the Vulkan port, actual draw command recording is done by
    // Scene::render(), which iterates meshes and records vkCmdDrawIndexed.
    // This method is retained for API compatibility and updates the AABB.
    if(!_enabled) return;

    const glm::mat4 world = transform()->computeWorldMatrix();
    _aabb.updateWithVertexData(vertexData(), world);

    if (shaderOverride == nullptr) _material->bind();
    else shaderOverride->bind();

    if (shaderOverride == nullptr) _material->unbind();
    else shaderOverride->unbind();
}

std::shared_ptr<Mesh> Mesh::FromVertexData(const char *name, VertexData &vertexData) {
    auto mesh = std::make_shared<Mesh>(name);
    mesh->setVertexData(vertexData);
    return mesh;
}

void Mesh::bakeTransformIntoVertexData() {
    auto worldMatrix = transform()->computeWorldMatrix();
    auto normalMatrix = transform()->computeNormalMatrix();
    for (int i = 0; i < _vertexData.positions.size(); i += 3) {
        glm::vec4 position = glm::vec4(_vertexData.positions[i], _vertexData.positions[i + 1],
                                       _vertexData.positions[i + 2], 1.0f);
        glm::vec4 transformedPosition = worldMatrix * position;
        _vertexData.positions[i] = transformedPosition.x;
        _vertexData.positions[i + 1] = transformedPosition.y;
        _vertexData.positions[i + 2] = transformedPosition.z;

        glm::vec4 normal = glm::vec4(_vertexData.normals[i], _vertexData.normals[i + 1],
                                     _vertexData.normals[i + 2], 1.0f);
        glm::vec4 transformedNormal = normalMatrix * normal;
        glm::vec3 normalizedTransformedNormal = glm::normalize(glm::vec3(transformedNormal.x, transformedNormal.y, transformedNormal.z));
        _vertexData.normals[i] = normalizedTransformedNormal.x;
        _vertexData.normals[i + 1] = normalizedTransformedNormal.y;
        _vertexData.normals[i + 2] = normalizedTransformedNormal.z;
    }

    sendVertexDataToGPU();

    transform()->reset();
}

void Mesh::bakeRotationIntoVertexData() {
    auto rotationMatrix = transform()->computeRotationMatrix();
    auto normalMatrix = transform()->computeNormalMatrix();
    for (int i = 0; i < _vertexData.positions.size(); i += 3) {
        glm::vec4 position = glm::vec4(_vertexData.positions[i], _vertexData.positions[i + 1],
                                       _vertexData.positions[i + 2], 1.0f);
        glm::vec4 transformedPosition = rotationMatrix * position;
        _vertexData.positions[i] = transformedPosition.x;
        _vertexData.positions[i + 1] = transformedPosition.y;
        _vertexData.positions[i + 2] = transformedPosition.z;

        glm::vec4 normal = glm::vec4(_vertexData.normals[i], _vertexData.normals[i + 1],
                                     _vertexData.normals[i + 2], 1.0f);
        glm::vec4 transformedNormal = normalMatrix * normal;
        glm::vec3 normalizedTransformedNormal = glm::normalize(glm::vec3(transformedNormal.x, transformedNormal.y, transformedNormal.z));
        _vertexData.normals[i] = normalizedTransformedNormal.x;
        _vertexData.normals[i + 1] = normalizedTransformedNormal.y;
        _vertexData.normals[i + 2] = normalizedTransformedNormal.z;
    }

    sendVertexDataToGPU();

    transform()->setRotation(0, 0, 0);
}

void Mesh::bakeScalingIntoVertexData() {
    auto scalingMatrix = glm::scale(glm::mat4(1.0f), transform()->scaling());
    auto normalMatrix = transform()->computeNormalMatrix();
    for (int i = 0; i < _vertexData.positions.size(); i += 3) {
        glm::vec4 position = glm::vec4(_vertexData.positions[i], _vertexData.positions[i + 1],
                                       _vertexData.positions[i + 2], 1.0f);
        glm::vec4 transformedPosition = scalingMatrix * position;
        _vertexData.positions[i] = transformedPosition.x;
        _vertexData.positions[i + 1] = transformedPosition.y;
        _vertexData.positions[i + 2] = transformedPosition.z;

        glm::vec4 normal = glm::vec4(_vertexData.normals[i], _vertexData.normals[i + 1],
                                     _vertexData.normals[i + 2], 1.0f);
        glm::vec4 transformedNormal = normalMatrix * normal;
        glm::vec3 normalizedTransformedNormal = glm::normalize(glm::vec3(transformedNormal.x, transformedNormal.y, transformedNormal.z));
        _vertexData.normals[i] = normalizedTransformedNormal.x;
        _vertexData.normals[i + 1] = normalizedTransformedNormal.y;
        _vertexData.normals[i + 2] = normalizedTransformedNormal.z;
    }

    sendVertexDataToGPU();

    transform()->setScale(1);
}
