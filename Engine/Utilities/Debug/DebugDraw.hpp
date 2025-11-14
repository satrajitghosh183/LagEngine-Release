#pragma once

#include "../../Core/Base.hpp"
#include "../../Graphics/Shader.hpp"
#include "../../Graphics/Camera3D.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace GameEngine {

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
         * @brief Update (remove expired lines)
         */
        static void Update(float deltaTime);
        
        /**
         * @brief Render all debug shapes
         */
        static void Render(const Camera3D& camera);
        
        /**
         * @brief Clear all debug shapes
         */
        static void Clear();
        
        /**
         * @brief Enable/disable depth testing
         */
        static void SetDepthTest(bool enabled);
        
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
        static Ref<Shader> s_Shader;
        static uint32_t s_VAO;
        static uint32_t s_VBO;
        static bool s_DepthTest;
    };
}