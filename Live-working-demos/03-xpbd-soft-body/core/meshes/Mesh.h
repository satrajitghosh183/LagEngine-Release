//
// Created by barth on 19/09/2022.
//
// Vulkan port: Mesh stores vertex data in host memory. GPU buffers
// are created/updated on demand via a VulkanBase pointer.
//

#ifndef FEATHERGL_MESH_H
#define FEATHERGL_MESH_H

#include <vector>
#include <memory>
#include <cstdint>
#include "Transformable.h"
#include "VertexData.h"
#include "DefaultMaterial.h"
#include "../utils/Uuid.h"
#include "Renderable.h"
#include "AABB.h"
#include "PickResult.h"
#include <VulkanBase.hpp>

class Mesh : public Transformable, public Renderable {
public:
    explicit Mesh(const char *name);

    bool operator==(const Mesh &other) const {
        return _id == other._id;
    }

    const std::string &name() const {
        return _name;
    }

    Transform *transform() override {
        return &_transform;
    }

    AABB *aabb() {
        return &_aabb;
    }

    static std::shared_ptr<Mesh> FromVertexData(const char *name, VertexData &vertexData);

    void setMaterial(std::shared_ptr<Material> material);

    std::shared_ptr<Material> material() const {
        return _material;
    }

    void bakeTransformIntoVertexData();

    void bakeRotationIntoVertexData();

    void bakeScalingIntoVertexData();

    void setVertexData(VertexData &vertexData);

    /// Upload current vertex/index data to the GPU (recreates buffers)
    void sendVertexDataToGPU();

    VertexData &vertexData() {
        return _vertexData;
    }

    PickResult pickWithRay(glm::vec3 rayOrigin, glm::vec3 rayDirection) {
        if (!_pickingEnabled) return PickResult{};
        glm::mat4 worldMatrix = transform()->computeWorldMatrix();
        return Utils::PickWithRay(vertexData().indices, vertexData().positions, rayOrigin, rayDirection, worldMatrix);
    }

    /// Record Vulkan draw commands into the given command buffer.
    /// projectionViewMatrix is used for push constant MVP.
    void render(glm::mat4 projectionViewMatrix, Shader* shaderOverride = nullptr);

    void setPickingEnabled(bool enabled) {
        _pickingEnabled = enabled;
    }

    bool isPickingEnabled() const {
        return _pickingEnabled;
    }

    void setEnabled(bool enabled) {
        _enabled = enabled;
    }

    bool isEnabled() {
        return _enabled;
    }

    // Vulkan GPU resources
    bool hasGPUBuffers() const { return _vertexBuffer.Buffer != VK_NULL_HANDLE; }
    vkdemo::GPUBuffer& vertexBuffer() { return _vertexBuffer; }
    vkdemo::GPUBuffer& indexBuffer() { return _indexBuffer; }
    uint32_t indexCount() const { return static_cast<uint32_t>(_vertexData.indices.size()); }
    uint32_t vertexCount() const { return static_cast<uint32_t>(_vertexData.positions.size() / 3); }

    /// Set the VulkanBase instance used for GPU buffer management
    static void SetVulkanBase(vkdemo::VulkanBase* vkBase) { s_VulkanBase = vkBase; }

    /// Mark that GPU buffers need recreation on next render
    void markDirty() { _gpuDirty = true; }
    bool isDirty() const { return _gpuDirty; }
    void clearDirty() { _gpuDirty = false; }

    static std::shared_ptr<Material> defaultMaterial;

private:
    std::string _name;
    std::string _id;
    VertexData _vertexData;

    Transform _transform;
    AABB _aabb;

    // Vulkan GPU buffers
    vkdemo::GPUBuffer _vertexBuffer{};
    vkdemo::GPUBuffer _indexBuffer{};
    bool _gpuDirty = true;

    bool _pickingEnabled = true;
    bool _enabled = true;

    static vkdemo::VulkanBase* s_VulkanBase;

protected:
    std::shared_ptr<Material> _material;
};

#endif //FEATHERGL_MESH_H
