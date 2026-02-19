#pragma once

#include "../../Core/Base.hpp"
#include <mutex>
#include "../../Graphics/Shader.hpp"
#include "../../Graphics/Camera3D.hpp"
#include "../../Physics/Shapes/CollisionShape.hpp"
#include "../../Physics/Collision/ContactManifold.hpp"
#include "../../Physics/Constraints/Constraint.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace GameEngine {

    // Forward declarations
    namespace Physics {
        class PhysicsWorld;
    }

    /**
     * @brief Debug visualization system
     * 
     * Draw simple shapes for debugging physics, AI, etc.
     * Lines persist for a specified duration
     */
    class DebugDraw {
    public:
        /**
         * @brief Initialize debug draw system
         */
        static void Init();
        
        /**
         * @brief Shutdown debug draw system
         */
        static void Shutdown();
        
        /**
         * @brief Draw line
         */
        static void DrawLine(const glm::vec3& start, const glm::vec3& end, 
                            const glm::vec3& color = glm::vec3(1, 1, 1), 
                            float duration = 0.0f);
        
        /**
         * @brief Draw ray
         */
        static void DrawRay(const glm::vec3& origin, const glm::vec3& direction, 
                           const glm::vec3& color = glm::vec3(1, 0, 0), 
                           float duration = 0.0f);
        
        /**
         * @brief Draw wireframe box
         */
        static void DrawBox(const glm::vec3& center, const glm::vec3& size, 
                           const glm::quat& rotation = glm::quat(1, 0, 0, 0),
                           const glm::vec3& color = glm::vec3(0, 1, 0), 
                           float duration = 0.0f);
        
        /**
         * @brief Draw wireframe sphere
         */
        static void DrawSphere(const glm::vec3& center, float radius, 
                              const glm::vec3& color = glm::vec3(0, 0, 1), 
                              float duration = 0.0f, int segments = 16);
        
        /**
         * @brief Draw wireframe capsule
         */
        static void DrawCapsule(const glm::vec3& center, float radius, float height,
                               const glm::quat& rotation = glm::quat(1, 0, 0, 0),
                               const glm::vec3& color = glm::vec3(1, 1, 0), 
                               float duration = 0.0f);
        
        /**
         * @brief Draw grid
         */
        static void DrawGrid(float size = 10.0f, int divisions = 10, 
                            const glm::vec3& color = glm::vec3(0.5f));
        
        /**
         * @brief Draw coordinate axes (X=red, Y=green, Z=blue)
         */
        static void DrawAxes(const glm::vec3& origin = glm::vec3(0), 
                            float length = 1.0f, float duration = 0.0f);
        
        /**
         * @brief Draw cross (3D marker)
         */
        static void DrawCross(const glm::vec3& position, float size = 0.5f,
                             const glm::vec3& color = glm::vec3(1), 
                             float duration = 0.0f);
        
        /**
         * @brief Draw frustum (for camera visualization)
         */
        static void DrawFrustum(const glm::mat4& viewProjection, 
                               const glm::vec3& color = glm::vec3(1, 0, 1), 
                               float duration = 0.0f);
        
        /**
         * @brief Update and render debug shapes
         *
         * Removes expired lines, then renders all remaining lines
         * using the active scene's main camera.
         */
        static void Update(float deltaTime);

        /**
         * @brief Render all debug shapes with explicit camera
         */
        static void Render(const Camera3D& camera);

        /**
         * @brief Render all debug shapes with explicit view-projection matrix
         *
         * Used by the editor viewport which has its own EditorCamera
         */
        static void Render(const glm::mat4& viewProjectionMatrix);

        /**
         * @brief Flush single-frame (duration=0) lines after rendering
         */
        static void FlushSingleFrame();

        /**
         * @brief Clear all debug shapes
         */
        static void Clear();
        
        /**
         * @brief Enable/disable depth testing
         */
        static void SetDepthTest(bool enabled);
        
        // ==================== Physics Debug Visualization ====================
        
        /**
         * @brief Draw collision shape
         */
        static void DrawCollider(const Ref<Physics::CollisionShape>& shape,
                                const glm::vec3& position,
                                const glm::quat& rotation,
                                const glm::vec3& color = glm::vec3(0, 1, 0),
                                float duration = 0.0f);
        
        /**
         * @brief Draw contact point
         */
        static void DrawContact(const glm::vec3& position,
                               const glm::vec3& normal,
                               float penetration,
                               const glm::vec3& color = glm::vec3(1, 0, 0),
                               float duration = 0.0f);
        
        /**
         * @brief Draw contact manifold
         */
        static void DrawContactManifold(const Physics::ContactManifold& manifold,
                                       float duration = 0.0f);
        
        /**
         * @brief Draw constraint (joint)
         */
        static void DrawConstraint(const Physics::Constraint* constraint,
                                  const glm::vec3& color = glm::vec3(1, 0.5f, 0),
                                  float duration = 0.0f);
        
        /**
         * @brief Draw entire physics world debug visualization
         */
        static void DrawPhysicsWorld(const Physics::PhysicsWorld* world,
                                    bool drawColliders = true,
                                    bool drawContacts = true,
                                    bool drawConstraints = true,
                                    float duration = 0.0f);
        
        /**
         * @brief Draw AABB
         */
        static void DrawAABB(const glm::vec3& min, const glm::vec3& max,
                            const glm::vec3& color = glm::vec3(0.5f, 0.5f, 1.0f),
                            float duration = 0.0f);
        
        /**
         * @brief Draw plane (infinite plane represented as bounded quad)
         */
        static void DrawPlane(const glm::vec3& normal, float distance,
                             float size = 5.0f,
                             const glm::vec3& color = glm::vec3(0.3f, 0.3f, 0.8f),
                             float duration = 0.0f);
        
    private:
        struct DebugLine {
            glm::vec3 Start;
            glm::vec3 End;
            glm::vec3 Color;
            float RemainingTime;
        };
        
        static void AddLine(const glm::vec3& start, const glm::vec3& end, 
                           const glm::vec3& color, float duration);
        
        static std::vector<DebugLine> s_Lines;
        static std::mutex s_LinesMutex;
        static Ref<Shader> s_Shader;
        static uint32_t s_VAO;
        static uint32_t s_VBO;
        static bool s_DepthTest;
    };
}