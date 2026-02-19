#pragma once

#include "../Core/Base.hpp"
#include "Mesh3D.hpp"
#include "Material.hpp"
#include "Camera3D.hpp"
#include "Shader.hpp"
#include "Light.hpp"
#include "ShadowMap.hpp"
#include "UniformBuffer.hpp"
#include "Frustum.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

namespace GameEngine {

    /**
     * @brief Light data for rendering (with position/direction)
     */
    struct LightData {
        Light Properties;
        glm::vec3 Position;     // For point/spot lights
        glm::vec3 Direction;    // For directional/spot lights
    };

    /**
     * @brief Draw command queued during Submit()
     */
    struct DrawCommand {
        Ref<Mesh3D> Mesh;
        Ref<Material> Mat;
        Ref<Shader> ShaderOnly;   // Used when submitting with shader instead of material
        glm::mat4 Transform;
        uint64_t SortKey = 0;     // For sorting: shader ID << 32 | material hash
    };

    /**
     * @brief 3D Renderer
     *
     * Deferred-submit renderer: Submit() queues draw commands,
     * EndScene() sorts by material/shader and flushes.
     *
     * Supports multi-pass rendering via RenderGraph integration.
     *
     * Usage:
     *   Renderer3D::BeginScene(camera);
     *   Renderer3D::Submit(mesh, material, transform);
     *   Renderer3D::EndScene();       // sorts + draws
     */
    class Renderer3D {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene(const Camera3D& camera);
        static void BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition);
        static void EndScene();

        /**
         * @brief Submit mesh for rendering (queued, drawn in EndScene)
         */
        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Material>& material, const glm::mat4& transform = glm::mat4(1.0f));
        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));

        static void SubmitLight(const Light& light, const glm::vec3& position, const glm::vec3& direction = glm::vec3(0, -1, 0));
        static void SetAmbientLight(const glm::vec3& color, float intensity = 0.1f);
        static void ClearLights();

        static const std::vector<LightData>& GetLights() { return s_Lights; }

        /**
         * @brief Apply lighting uniforms to a shader
         */
        static void ApplyLighting(const Ref<Shader>& shader);

        /**
         * @brief Flush the render queue for a shadow pass (depth-only)
         * Renders all queued meshes with the depth shader.
         */
        static void FlushShadowPass(const Ref<Shader>& depthShader, const glm::mat4& lightSpaceMatrix);

        /**
         * @brief Get the current render queue (for external pass implementations)
         */
        static const std::vector<DrawCommand>& GetRenderQueue() { return s_RenderQueue; }

        /**
         * @brief Get current scene data
         */
        static const glm::mat4& GetViewProjection() { return s_SceneData.ViewProjectionMatrix; }
        static const glm::vec3& GetCameraPosition() { return s_SceneData.CameraPosition; }

        static constexpr int MAX_LIGHTS = 16;

        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t VertexCount = 0;
            uint32_t IndexCount = 0;
            uint32_t CulledObjects = 0;

            void Reset() {
                DrawCalls = 0;
                VertexCount = 0;
                IndexCount = 0;
                CulledObjects = 0;
            }
        };

        static const Statistics& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats.Reset(); }

    private:
        static void FlushQueue();
        static void ExecuteDrawCommand(const DrawCommand& cmd);

        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
            glm::vec3 CameraPosition;
            glm::vec3 AmbientColor;
            float AmbientIntensity;
        };

        static void UploadCameraUBO();
        static void UploadLightingUBO();

        static SceneData s_SceneData;
        static Statistics s_Stats;
        static std::vector<LightData> s_Lights;
        static std::vector<DrawCommand> s_RenderQueue;

        static constexpr uint32_t kCameraUBOBinding = 0;
        static constexpr uint32_t kLightingUBOBinding = 1;
        static Scope<UniformBuffer> s_CameraUBO;
        static Scope<UniformBuffer> s_LightingUBO;
        static Frustum s_Frustum;
    };
}
