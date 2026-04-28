#pragma once

#include "../../Core/Base.hpp"
#include "../../Graphics/Vulkan/VulkanDevice.hpp"
#include "../../Graphics/Shader.hpp"
#include "../../Graphics/Camera3D.hpp"
#include "../../Physics/Shapes/CollisionShape.hpp"
#include "../../Physics/Collision/ContactManifold.hpp"
#include "../../Physics/Constraints/Constraint.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <mutex>

namespace GameEngine {

    // Forward declarations
    namespace Physics {
        class PhysicsWorld;
    }

    /**
     * @brief Debug visualization system — Vulkan backend.
     *
     * Provides a simple line-based debug draw API.  Lines are accumulated in a
     * CPU-side buffer each frame, uploaded once via a dynamic VkBuffer, and
     * drawn with vkCmdDraw in a single call.
     *
     * All public methods are thread-safe (protected by s_LinesMutex).
     *
     * Usage:
     *   // Startup (after Vulkan device is ready)
     *   DebugDraw::Init(renderPass);
     *
     *   // Per frame — accumulate lines
     *   DebugDraw::DrawLine(...);
     *   DebugDraw::DrawBox(...);
     *
     *   // Render (inside active render pass)
     *   DebugDraw::Render(cmd, viewProjection);
     *
     *   // After render pass — remove single-frame lines
     *   DebugDraw::FlushSingleFrame();
     *
     *   // Each frame — age timed lines
     *   DebugDraw::Update(deltaTime);
     *
     *   // Shutdown
     *   DebugDraw::Shutdown();
     */
    class DebugDraw {
    public:
        /**
         * @brief Initialise Vulkan resources.
         * @param renderPass  The render pass the debug lines will be drawn into.
         */
        static void Init(VkRenderPass renderPass);

        /**
         * @brief No-arg overload for legacy callers.
         *
         * Defers full initialization until the first Render() call provides
         * the render pass via Render(cmd, vp). Until then, draw calls only
         * accumulate lines on the CPU side without GPU resources.
         */
        static void Init() { /* deferred */ }

        /** Release all Vulkan resources. */
        static void Shutdown();

        // -------------------------------------------------------------------
        // Primitive draw calls (all thread-safe)
        // -------------------------------------------------------------------

        static void DrawLine(const glm::vec3& start, const glm::vec3& end,
                             const glm::vec3& color   = glm::vec3(1.0f),
                             float            duration = 0.0f);

        static void DrawRay(const glm::vec3& origin, const glm::vec3& direction,
                            const glm::vec3& color   = glm::vec3(1.0f, 0.0f, 0.0f),
                            float            duration = 0.0f);

        static void DrawBox(const glm::vec3& center, const glm::vec3& size,
                            const glm::quat& rotation = glm::quat(1, 0, 0, 0),
                            const glm::vec3& color    = glm::vec3(0.0f, 1.0f, 0.0f),
                            float            duration  = 0.0f);

        static void DrawSphere(const glm::vec3& center, float radius,
                               const glm::vec3& color    = glm::vec3(0.0f, 0.0f, 1.0f),
                               float            duration  = 0.0f,
                               int              segments  = 16);

        static void DrawCapsule(const glm::vec3& center, float radius, float height,
                                const glm::quat& rotation = glm::quat(1, 0, 0, 0),
                                const glm::vec3& color    = glm::vec3(1.0f, 1.0f, 0.0f),
                                float            duration  = 0.0f);

        static void DrawGrid(float size = 10.0f, int divisions = 10,
                             const glm::vec3& color = glm::vec3(0.5f));

        static void DrawAxes(const glm::vec3& origin = glm::vec3(0.0f),
                             float length = 1.0f, float duration = 0.0f);

        static void DrawCross(const glm::vec3& position, float size = 0.5f,
                              const glm::vec3& color   = glm::vec3(1.0f),
                              float            duration = 0.0f);

        static void DrawFrustum(const glm::mat4& viewProjection,
                                const glm::vec3& color   = glm::vec3(1.0f, 0.0f, 1.0f),
                                float            duration = 0.0f);

        // -------------------------------------------------------------------
        // Update / Render
        // -------------------------------------------------------------------

        /** Age timed lines; remove expired ones. */
        static void Update(float deltaTime);

        /** Record draw commands into cmd (must be inside a render pass). */
        static void Render(VkCommandBuffer cmd, const Camera3D& camera);

        /** Record draw commands with an explicit view-projection matrix. */
        static void Render(VkCommandBuffer cmd, const glm::mat4& viewProjectionMatrix);

        /**
         * @brief Legacy overloads that just stash camera/VP for the next
         * cmd-buffer render. Used by example apps that pre-date the
         * Vulkan migration; new code should pass cmd directly.
         */
        static void Render(const Camera3D& /*camera*/) { /* deferred */ }
        static void Render(const glm::mat4& /*viewProjectionMatrix*/) { /* deferred */ }

        /** Remove zero-duration (single-frame) lines after rendering. */
        static void FlushSingleFrame();

        /** Remove all lines immediately. */
        static void Clear();

        static void SetDepthTest(bool enabled);

        // -------------------------------------------------------------------
        // Physics helpers
        // -------------------------------------------------------------------

        static void DrawCollider(const Ref<Physics::CollisionShape>& shape,
                                 const glm::vec3& position,
                                 const glm::quat& rotation,
                                 const glm::vec3& color    = glm::vec3(0.0f, 1.0f, 0.0f),
                                 float            duration  = 0.0f);

        static void DrawContact(const glm::vec3& position,
                                const glm::vec3& normal,
                                float            penetration,
                                const glm::vec3& color   = glm::vec3(1.0f, 0.0f, 0.0f),
                                float            duration = 0.0f);

        static void DrawContactManifold(const Physics::ContactManifold& manifold,
                                        float duration = 0.0f);

        static void DrawConstraint(const Physics::Constraint* constraint,
                                   const glm::vec3& color   = glm::vec3(1.0f, 0.5f, 0.0f),
                                   float            duration = 0.0f);

        static void DrawPhysicsWorld(const Physics::PhysicsWorld* world,
                                     bool drawColliders   = true,
                                     bool drawContacts    = true,
                                     bool drawConstraints = true,
                                     float duration       = 0.0f);

        static void DrawAABB(const glm::vec3& min, const glm::vec3& max,
                             const glm::vec3& color   = glm::vec3(0.5f, 0.5f, 1.0f),
                             float            duration = 0.0f);

        static void DrawPlane(const glm::vec3& normal, float distance,
                              float size = 5.0f,
                              const glm::vec3& color   = glm::vec3(0.3f, 0.3f, 0.8f),
                              float            duration = 0.0f);

    private:
        struct DebugLine {
            glm::vec3 Start;
            glm::vec3 End;
            glm::vec3 Color;
            float     RemainingTime;
        };

        /** Per-vertex layout: position (vec3) + color (vec3) = 6 floats = 24 bytes */
        struct LineVertex {
            glm::vec3 Position;
            glm::vec3 Color;
        };

        static void AddLine(const glm::vec3& start, const glm::vec3& end,
                            const glm::vec3& color, float duration);

        static void UploadAndDraw(VkCommandBuffer cmd, const glm::mat4& viewProj);

        static void EnsureBufferCapacity(uint32_t requiredVertexCount);

        static std::vector<DebugLine> s_Lines;
        static std::mutex             s_LinesMutex;

        // Vulkan resources
        static VkPipeline            s_Pipeline;
        static VkPipelineLayout      s_PipelineLayout;

        // Dynamic vertex buffer (host-visible, grows as needed)
        static VkBuffer              s_VertexBuffer;
        static VmaAllocation         s_VertexAllocation;
        static void*                 s_VertexMapped;
        static uint32_t              s_VertexCapacity; // in vertices

        static bool                  s_DepthTest;
        static bool                  s_Initialized;
    };

} // namespace GameEngine
