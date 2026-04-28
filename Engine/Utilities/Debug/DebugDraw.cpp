#include "DebugDraw.hpp"
#include "../../Core/Logger.hpp"
#include "../../Core/Application.hpp"
#include "../../Scene/SceneManager.hpp"
#include "../../Graphics/RenderCommand.hpp"
#include "../../Physics/PhysicsWorld.hpp"
#include "../../Physics/Shapes/SphereShape.hpp"
#include "../../Physics/Shapes/BoxShape.hpp"
#include "../../Physics/Shapes/CapsuleShape.hpp"
#include "../../Physics/Shapes/PlaneShape.hpp"
#include <glm/gtc/matrix_transform.hpp>

// =============================================================================
// DebugDraw — Vulkan implementation
//
// Lines are accumulated in s_Lines (CPU), then uploaded into a dynamic
// host-visible VkBuffer once per Render() call.  A single vkCmdDraw with
// VK_PRIMITIVE_TOPOLOGY_LINE_LIST draws all lines.
//
// The pipeline is created with the SPIR-V shaders:
//   Assets/Shaders/Debug/debug.vert.spv
//   Assets/Shaders/Debug/debug.frag.spv
//
// Vertex layout (per vertex, interleaved):
//   location 0 : vec3  position
//   location 1 : vec3  color
//
// View-projection is a push constant (VK_SHADER_STAGE_VERTEX_BIT, 64 bytes).
// =============================================================================

namespace GameEngine {

    // =========================================================================
    // Static member definitions
    // =========================================================================

    std::vector<DebugDraw::DebugLine> DebugDraw::s_Lines;
    std::mutex                         DebugDraw::s_LinesMutex;

    VkPipeline            DebugDraw::s_Pipeline        = VK_NULL_HANDLE;
    VkPipelineLayout      DebugDraw::s_PipelineLayout  = VK_NULL_HANDLE;
    VkBuffer              DebugDraw::s_VertexBuffer     = VK_NULL_HANDLE;
    VmaAllocation         DebugDraw::s_VertexAllocation = VK_NULL_HANDLE;
    void*                 DebugDraw::s_VertexMapped     = nullptr;
    uint32_t              DebugDraw::s_VertexCapacity   = 0;
    bool                  DebugDraw::s_DepthTest        = true;
    bool                  DebugDraw::s_Initialized      = false;

    // =========================================================================
    // Internal helpers
    // =========================================================================

    void DebugDraw::EnsureBufferCapacity(uint32_t requiredVertexCount) {
        if (requiredVertexCount <= s_VertexCapacity) return;

        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        // Free old buffer
        if (s_VertexBuffer != VK_NULL_HANDLE) {
            vmaUnmapMemory(allocator, s_VertexAllocation);
            vmaDestroyBuffer(allocator, s_VertexBuffer, s_VertexAllocation);
            s_VertexBuffer     = VK_NULL_HANDLE;
            s_VertexAllocation = VK_NULL_HANDLE;
            s_VertexMapped     = nullptr;
        }

        uint32_t newCapacity = std::max(requiredVertexCount, 4096u);
        VkDeviceSize size = static_cast<VkDeviceSize>(newCapacity) * sizeof(LineVertex);

        VkBufferCreateInfo bufCI{};
        bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufCI.size  = size;
        bufCI.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

        VmaAllocationCreateInfo allocCI{};
        allocCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                        VMA_ALLOCATION_CREATE_MAPPED_BIT;
        allocCI.usage = VMA_MEMORY_USAGE_AUTO;

        VmaAllocationInfo info;
        vmaCreateBuffer(allocator, &bufCI, &allocCI,
                        &s_VertexBuffer, &s_VertexAllocation, &info);
        s_VertexMapped   = info.pMappedData;
        s_VertexCapacity = newCapacity;
    }

    void DebugDraw::UploadAndDraw(VkCommandBuffer cmd, const glm::mat4& viewProj) {
        std::vector<DebugLine> linesCopy;
        {
            std::lock_guard<std::mutex> lock(s_LinesMutex);
            if (s_Lines.empty()) return;
            linesCopy = s_Lines;
        }

        // Build interleaved vertex data (2 vertices per line)
        uint32_t vertexCount = static_cast<uint32_t>(linesCopy.size()) * 2u;
        EnsureBufferCapacity(vertexCount);

        auto* dst = static_cast<LineVertex*>(s_VertexMapped);
        uint32_t vi = 0;
        for (const auto& line : linesCopy) {
            dst[vi++] = { line.Start, line.Color };
            dst[vi++] = { line.End,   line.Color };
        }

        // Flush mapped range (VMA uses coherent memory by default with AUTO usage;
        // call flush explicitly to be safe with non-coherent devices)
        vmaFlushAllocation(VulkanDevice::Get().GetAllocator(),
                           s_VertexAllocation, 0, VK_WHOLE_SIZE);

        if (s_Pipeline == VK_NULL_HANDLE) return; // pipeline pending SPIR-V

        // Bind pipeline
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, s_Pipeline);

        // Push view-projection
        vkCmdPushConstants(cmd, s_PipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                           &viewProj);

        // Bind vertex buffer
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &s_VertexBuffer, &offset);

        // Draw lines
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
    }

    // =========================================================================
    // Public API — init / shutdown
    // =========================================================================

    void DebugDraw::Init(VkRenderPass renderPass) {
        if (s_Initialized) return;

        VkDevice device = VulkanDevice::Get().GetDevice();

        // Push constant: mat4 viewProjection (64 bytes, vertex stage)
        VkPushConstantRange pcRange{};
        pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcRange.offset     = 0;
        pcRange.size       = sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo layoutCI{};
        layoutCI.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutCI.pushConstantRangeCount = 1;
        layoutCI.pPushConstantRanges    = &pcRange;
        vkCreatePipelineLayout(device, &layoutCI, nullptr, &s_PipelineLayout);

        // TODO: load debug.vert.spv + debug.frag.spv and build VkPipeline
        //       with VK_PRIMITIVE_TOPOLOGY_LINE_LIST.
        GE_CORE_INFO("DebugDraw: Vulkan pipeline deferred pending SPIR-V compilation");

        (void)renderPass; // stored by the pipeline builder when implemented

        s_Initialized = true;
        GE_CORE_INFO("DebugDraw initialised (Vulkan)");
    }

    void DebugDraw::Shutdown() {
        if (!s_Initialized) return;

        VkDevice     device    = VulkanDevice::Get().GetDevice();
        VmaAllocator allocator = VulkanDevice::Get().GetAllocator();

        vkDeviceWaitIdle(device);

        if (s_Pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, s_Pipeline, nullptr);
            s_Pipeline = VK_NULL_HANDLE;
        }
        if (s_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, s_PipelineLayout, nullptr);
            s_PipelineLayout = VK_NULL_HANDLE;
        }
        if (s_VertexBuffer != VK_NULL_HANDLE) {
            vmaUnmapMemory(allocator, s_VertexAllocation);
            vmaDestroyBuffer(allocator, s_VertexBuffer, s_VertexAllocation);
            s_VertexBuffer     = VK_NULL_HANDLE;
            s_VertexAllocation = VK_NULL_HANDLE;
            s_VertexMapped     = nullptr;
            s_VertexCapacity   = 0;
        }

        {
            std::lock_guard<std::mutex> lock(s_LinesMutex);
            s_Lines.clear();
        }

        s_Initialized = false;
    }

    // =========================================================================
    // Public API — per-frame
    // =========================================================================

    void DebugDraw::Update(float deltaTime) {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.erase(
            std::remove_if(s_Lines.begin(), s_Lines.end(), [deltaTime](DebugLine& line) {
                if (line.RemainingTime > 0.0f) {
                    line.RemainingTime -= deltaTime;
                    return line.RemainingTime <= 0.0f;
                }
                return false;
            }),
            s_Lines.end());
    }

    void DebugDraw::Render(VkCommandBuffer cmd, const Camera3D& camera) {
        if (!s_Initialized) return;
        UploadAndDraw(cmd, camera.GetViewProjectionMatrix());
    }

    void DebugDraw::Render(VkCommandBuffer cmd, const glm::mat4& viewProjectionMatrix) {
        if (!s_Initialized) return;
        UploadAndDraw(cmd, viewProjectionMatrix);
    }

    void DebugDraw::FlushSingleFrame() {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.erase(
            std::remove_if(s_Lines.begin(), s_Lines.end(),
                [](const DebugLine& line) { return line.RemainingTime == 0.0f; }),
            s_Lines.end());
    }

    void DebugDraw::Clear() {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.clear();
    }

    void DebugDraw::SetDepthTest(bool enabled) {
        s_DepthTest = enabled;
    }

    // =========================================================================
    // Internal line accumulation
    // =========================================================================

    void DebugDraw::AddLine(const glm::vec3& start, const glm::vec3& end,
                            const glm::vec3& color, float duration) {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.push_back({ start, end, color, duration });
    }

    // =========================================================================
    // Public draw calls
    // =========================================================================

    void DebugDraw::DrawLine(const glm::vec3& start, const glm::vec3& end,
                             const glm::vec3& color, float duration) {
        AddLine(start, end, color, duration);
    }

    void DebugDraw::DrawRay(const glm::vec3& origin, const glm::vec3& direction,
                            const glm::vec3& color, float duration) {
        DrawLine(origin, origin + direction, color, duration);
    }

    void DebugDraw::DrawBox(const glm::vec3& center, const glm::vec3& size,
                            const glm::quat& rotation, const glm::vec3& color, float duration) {
        glm::vec3 h = size * 0.5f;

        glm::vec3 corners[8] = {
            glm::vec3(-h.x, -h.y, -h.z), glm::vec3( h.x, -h.y, -h.z),
            glm::vec3( h.x,  h.y, -h.z), glm::vec3(-h.x,  h.y, -h.z),
            glm::vec3(-h.x, -h.y,  h.z), glm::vec3( h.x, -h.y,  h.z),
            glm::vec3( h.x,  h.y,  h.z), glm::vec3(-h.x,  h.y,  h.z)
        };

        for (int i = 0; i < 8; i++)
            corners[i] = center + rotation * corners[i];

        DrawLine(corners[0], corners[1], color, duration);
        DrawLine(corners[1], corners[2], color, duration);
        DrawLine(corners[2], corners[3], color, duration);
        DrawLine(corners[3], corners[0], color, duration);

        DrawLine(corners[4], corners[5], color, duration);
        DrawLine(corners[5], corners[6], color, duration);
        DrawLine(corners[6], corners[7], color, duration);
        DrawLine(corners[7], corners[4], color, duration);

        DrawLine(corners[0], corners[4], color, duration);
        DrawLine(corners[1], corners[5], color, duration);
        DrawLine(corners[2], corners[6], color, duration);
        DrawLine(corners[3], corners[7], color, duration);
    }

    void DebugDraw::DrawSphere(const glm::vec3& center, float radius,
                               const glm::vec3& color, float duration, int segments) {
        float step = glm::two_pi<float>() / segments;

        for (int axis = 0; axis < 3; axis++) {
            for (int i = 0; i < segments; i++) {
                float a1 = i * step;
                float a2 = (i + 1) * step;
                glm::vec3 p1, p2;

                if (axis == 0) {
                    p1 = glm::vec3(0.0f, std::cos(a1), std::sin(a1)) * radius;
                    p2 = glm::vec3(0.0f, std::cos(a2), std::sin(a2)) * radius;
                } else if (axis == 1) {
                    p1 = glm::vec3(std::cos(a1), 0.0f, std::sin(a1)) * radius;
                    p2 = glm::vec3(std::cos(a2), 0.0f, std::sin(a2)) * radius;
                } else {
                    p1 = glm::vec3(std::cos(a1), std::sin(a1), 0.0f) * radius;
                    p2 = glm::vec3(std::cos(a2), std::sin(a2), 0.0f) * radius;
                }

                DrawLine(center + p1, center + p2, color, duration);
            }
        }
    }

    void DebugDraw::DrawCapsule(const glm::vec3& center, float radius, float height,
                                const glm::quat& rotation, const glm::vec3& color, float duration) {
        glm::vec3 up      = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        float     half    = (height - 2.0f * radius) * 0.5f;
        glm::vec3 top     = center + up * half;
        glm::vec3 bottom  = center - up * half;

        DrawSphere(top,    radius, color, duration, 16);
        DrawSphere(bottom, radius, color, duration, 16);

        glm::vec3 right   = rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 forward = rotation * glm::vec3(0.0f, 0.0f, 1.0f);

        DrawLine(top    + right   * radius, bottom + right   * radius, color, duration);
        DrawLine(top    - right   * radius, bottom - right   * radius, color, duration);
        DrawLine(top    + forward * radius, bottom + forward * radius, color, duration);
        DrawLine(top    - forward * radius, bottom - forward * radius, color, duration);
    }

    void DebugDraw::DrawGrid(float size, int divisions, const glm::vec3& color) {
        float step     = size / divisions;
        float halfSize = size * 0.5f;

        for (int i = 0; i <= divisions; i++) {
            float pos = -halfSize + i * step;
            DrawLine(glm::vec3(pos, 0.0f, -halfSize), glm::vec3(pos, 0.0f,  halfSize), color);
            DrawLine(glm::vec3(-halfSize, 0.0f, pos), glm::vec3( halfSize, 0.0f, pos), color);
        }
    }

    void DebugDraw::DrawAxes(const glm::vec3& origin, float length, float duration) {
        DrawLine(origin, origin + glm::vec3(length, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), duration);
        DrawLine(origin, origin + glm::vec3(0.0f, length, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), duration);
        DrawLine(origin, origin + glm::vec3(0.0f, 0.0f, length), glm::vec3(0.0f, 0.0f, 1.0f), duration);
    }

    void DebugDraw::DrawCross(const glm::vec3& position, float size,
                              const glm::vec3& color, float duration) {
        float h = size * 0.5f;
        DrawLine(position - glm::vec3(h, 0.0f, 0.0f), position + glm::vec3(h, 0.0f, 0.0f), color, duration);
        DrawLine(position - glm::vec3(0.0f, h, 0.0f), position + glm::vec3(0.0f, h, 0.0f), color, duration);
        DrawLine(position - glm::vec3(0.0f, 0.0f, h), position + glm::vec3(0.0f, 0.0f, h), color, duration);
    }

    void DebugDraw::DrawFrustum(const glm::mat4& viewProjection,
                                const glm::vec3& color, float duration) {
        glm::mat4 invVP = glm::inverse(viewProjection);
        glm::vec3 corners[8];
        int index = 0;

        for (int z = 0; z < 2; z++) {
            for (int y = 0; y < 2; y++) {
                for (int x = 0; x < 2; x++) {
                    glm::vec4 c = invVP * glm::vec4(
                        x * 2.0f - 1.0f,
                        y * 2.0f - 1.0f,
                        z * 2.0f - 1.0f,
                        1.0f);
                    corners[index++] = glm::vec3(c) / c.w;
                }
            }
        }

        // Near plane
        DrawLine(corners[0], corners[1], color, duration);
        DrawLine(corners[1], corners[3], color, duration);
        DrawLine(corners[3], corners[2], color, duration);
        DrawLine(corners[2], corners[0], color, duration);

        // Far plane
        DrawLine(corners[4], corners[5], color, duration);
        DrawLine(corners[5], corners[7], color, duration);
        DrawLine(corners[7], corners[6], color, duration);
        DrawLine(corners[6], corners[4], color, duration);

        // Connecting edges
        DrawLine(corners[0], corners[4], color, duration);
        DrawLine(corners[1], corners[5], color, duration);
        DrawLine(corners[2], corners[6], color, duration);
        DrawLine(corners[3], corners[7], color, duration);
    }

    // =========================================================================
    // Physics debug visualization
    // =========================================================================

    void DebugDraw::DrawCollider(const Ref<Physics::CollisionShape>& shape,
                                 const glm::vec3& position,
                                 const glm::quat& rotation,
                                 const glm::vec3& color,
                                 float duration) {
        if (!shape) return;

        switch (shape->GetType()) {
            case Physics::CollisionShape::ShapeType::Sphere: {
                auto sphere = std::static_pointer_cast<Physics::SphereShape>(shape);
                DrawSphere(position, sphere->GetRadius(), color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Box: {
                auto box = std::static_pointer_cast<Physics::BoxShape>(shape);
                DrawBox(position, box->GetHalfExtents() * 2.0f, rotation, color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Capsule: {
                auto cap = std::static_pointer_cast<Physics::CapsuleShape>(shape);
                DrawCapsule(position, cap->GetRadius(), cap->GetHeight(), rotation, color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Plane: {
                auto plane = std::static_pointer_cast<Physics::PlaneShape>(shape);
                DrawPlane(rotation * plane->GetNormal(), plane->GetDistance(), 5.0f, color, duration);
                break;
            }
            default:
                break;
        }
    }

    void DebugDraw::DrawContact(const glm::vec3& position, const glm::vec3& normal,
                                float penetration, const glm::vec3& color, float duration) {
        DrawCross(position, 0.1f, color, duration);
        DrawRay(position, normal * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f), duration);
        DrawLine(position, position - normal * penetration, glm::vec3(1.0f, 1.0f, 0.0f), duration);
    }

    void DebugDraw::DrawContactManifold(const Physics::ContactManifold& manifold, float duration) {
        for (const auto& contact : manifold.GetContacts()) {
            glm::vec3 contactPos = (contact.PositionA + contact.PositionB) * 0.5f;
            DrawContact(contactPos, contact.Normal, contact.Penetration,
                        glm::vec3(1.0f, 0.0f, 0.0f), duration);
            DrawCross(contact.PositionA, 0.05f, glm::vec3(0.0f, 1.0f, 0.0f), duration);
            DrawCross(contact.PositionB, 0.05f, glm::vec3(0.0f, 0.0f, 1.0f), duration);
        }
    }

    void DebugDraw::DrawConstraint(const Physics::Constraint* constraint,
                                   const glm::vec3& color, float duration) {
        if (!constraint) return;
        Physics::RigidBody* bodyA = constraint->GetBodyA();
        Physics::RigidBody* bodyB = constraint->GetBodyB();
        if (bodyA && bodyB) {
            DrawLine(bodyA->GetPosition(), bodyB->GetPosition(), color, duration);
            DrawCross(bodyA->GetPosition(), 0.15f, color, duration);
            DrawCross(bodyB->GetPosition(), 0.15f, color, duration);
        }
    }

    void DebugDraw::DrawPhysicsWorld(const Physics::PhysicsWorld* world,
                                     bool drawColliders, bool drawContacts,
                                     bool drawConstraints, float duration) {
        if (!world) return;

        if (drawColliders) {
            for (const auto& body : world->GetRigidBodies()) {
                if (body->HasCollisionShape()) {
                    glm::vec3 color;
                    switch (body->GetBodyType()) {
                        case Physics::RigidBody::BodyType::Static:
                            color = glm::vec3(0.5f); break;
                        case Physics::RigidBody::BodyType::Kinematic:
                            color = glm::vec3(0.0f, 0.8f, 0.8f); break;
                        case Physics::RigidBody::BodyType::Dynamic:
                            color = body->IsAwake() ? glm::vec3(0.0f, 1.0f, 0.0f)
                                                    : glm::vec3(0.3f, 0.3f, 0.6f);
                            break;
                    }
                    DrawCollider(body->GetCollisionShape(), body->GetPosition(),
                                 body->GetRotation(), color, duration);
                }
            }
        }

        if (drawContacts) {
            for (const auto& manifold : world->GetContactManifolds()) {
                DrawContactManifold(manifold, duration);
            }
        }

        if (drawConstraints) {
            for (const auto& constraint : world->GetConstraints()) {
                glm::vec3 color = constraint->Enabled
                                ? glm::vec3(1.0f, 0.5f, 0.0f)
                                : glm::vec3(0.5f);
                DrawConstraint(constraint.get(), color, duration);
            }
        }
    }

    void DebugDraw::DrawAABB(const glm::vec3& min, const glm::vec3& max,
                             const glm::vec3& color, float duration) {
        DrawBox((min + max) * 0.5f, max - min, glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
                color, duration);
    }

    void DebugDraw::DrawPlane(const glm::vec3& normal, float distance,
                              float size, const glm::vec3& color, float duration) {
        glm::vec3 planePoint = normal * distance;

        glm::vec3 tangent1;
        if (std::abs(normal.y) < 0.9f)
            tangent1 = glm::normalize(glm::cross(normal, glm::vec3(0.0f, 1.0f, 0.0f)));
        else
            tangent1 = glm::normalize(glm::cross(normal, glm::vec3(1.0f, 0.0f, 0.0f)));

        glm::vec3 tangent2 = glm::normalize(glm::cross(normal, tangent1));
        float halfSize = size * 0.5f;
        int   gridDiv  = 5;
        float step     = size / gridDiv;

        for (int i = 0; i <= gridDiv; i++) {
            float offset = -halfSize + i * step;

            DrawLine(planePoint + tangent1 * offset - tangent2 * halfSize,
                     planePoint + tangent1 * offset + tangent2 * halfSize,
                     color, duration);

            DrawLine(planePoint - tangent1 * halfSize + tangent2 * offset,
                     planePoint + tangent1 * halfSize + tangent2 * offset,
                     color, duration);
        }

        DrawRay(planePoint, normal * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f), duration);
    }

} // namespace GameEngine
