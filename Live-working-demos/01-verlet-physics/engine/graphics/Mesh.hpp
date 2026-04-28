// Mesh.hpp -- Vulkan-compatible mesh data container
//
// Stores vertex/normal/index data on the CPU side.  The Vulkan entry
// points upload this data into GPU buffers via VulkanBase helpers and
// record draw commands on a VkCommandBuffer.
#pragma once

#include <vector>
#include <cstring>
#include <glm/glm.hpp>

#include <vulkan/vulkan.h>
#include <VulkanBase.hpp>

namespace engine::graphics {

    class Mesh {
    public:
        // ---- CPU-side geometry data ----
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        bool useIndices = false;

        // ---- Vulkan GPU resources (managed by caller or upload()) ----
        vkdemo::GPUBuffer vertexBuffer{};
        vkdemo::GPUBuffer normalBuffer{};
        vkdemo::GPUBuffer indexBuffer{};

        /// Upload CPU data into host-visible Vulkan buffers.
        /// Requires a valid VulkanBase reference.
        /// @param vk  VulkanBase instance used to create buffers.
        /// @param dynamic  Hint (unused -- host-visible buffers are always
        ///                 re-writable in the current scheme).
        void upload(vkdemo::VulkanBase& vk, bool dynamic = false) {
            if (!vertices.empty()) {
                VkDeviceSize size = vertices.size() * sizeof(glm::vec3);
                vertexBuffer = vk.CreateBuffer(size,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                vk.CopyToBuffer(vertexBuffer, vertices.data(), size);
            }

            if (!normals.empty()) {
                VkDeviceSize size = normals.size() * sizeof(glm::vec3);
                normalBuffer = vk.CreateBuffer(size,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                vk.CopyToBuffer(normalBuffer, normals.data(), size);
            }

            if (!indices.empty()) {
                VkDeviceSize size = indices.size() * sizeof(unsigned int);
                indexBuffer = vk.CreateBuffer(size,
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                vk.CopyToBuffer(indexBuffer, indices.data(), size);
                useIndices = true;
            }
        }

        /// Legacy no-arg overload kept for code that calls upload()
        /// without a VulkanBase reference. Does nothing; the caller
        /// is expected to manage buffer uploads manually.
        void upload(bool /*dynamic*/ = false) {
            useIndices = !indices.empty();
        }

        /// Re-upload vertex positions to the existing GPU buffer.
        void updateVertices(vkdemo::VulkanBase& vk) {
            if (vertexBuffer.Buffer && !vertices.empty()) {
                vk.CopyToBuffer(vertexBuffer, vertices.data(),
                                vertices.size() * sizeof(glm::vec3));
            }
        }

        /// Legacy no-arg overload (no-op without VulkanBase).
        void updateVertices() const {}

        /// Re-upload normals to the existing GPU buffer.
        void updateNormals(vkdemo::VulkanBase& vk) {
            if (normalBuffer.Buffer && !normals.empty()) {
                vk.CopyToBuffer(normalBuffer, normals.data(),
                                normals.size() * sizeof(glm::vec3));
            }
        }

        /// Legacy no-arg overload (no-op without VulkanBase).
        void updateNormals() const {}

        /// Record draw commands into a Vulkan command buffer.
        void draw(VkCommandBuffer cmd) const {
            if (vertexBuffer.Buffer == VK_NULL_HANDLE) return;

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer.Buffer, &offset);

            if (useIndices && indexBuffer.Buffer != VK_NULL_HANDLE) {
                vkCmdBindIndexBuffer(cmd, indexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(cmd, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            } else {
                vkCmdDraw(cmd, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
            }
        }

        /// Destroy GPU resources.
        void destroy(vkdemo::VulkanBase& vk) {
            vk.DestroyBuffer(vertexBuffer);
            vk.DestroyBuffer(normalBuffer);
            vk.DestroyBuffer(indexBuffer);
        }

        /// Legacy no-arg overload (no-op without VulkanBase).
        void destroy() {}
    };

}
