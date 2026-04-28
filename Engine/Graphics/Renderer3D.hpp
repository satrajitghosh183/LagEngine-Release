#pragma once

#include "../Core/Base.hpp"
#include "Mesh3D.hpp"
#include "Material.hpp"
#include "Camera3D.hpp"
#include "Shader.hpp"
#include "Light.hpp"
#include "UniformBuffer.hpp"
#include "Frustum.hpp"
#include "Vulkan/VulkanDescriptors.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>

namespace GameEngine {

    struct LightData {
        Light Properties;
        glm::vec3 Position;
        glm::vec3 Direction;
    };

    struct DrawCommand {
        Ref<Mesh3D> Mesh;
        Ref<Material> Mat;
        Ref<Shader> ShaderOnly;
        glm::mat4 Transform;
        uint64_t SortKey = 0;
    };

    /**
     * @brief 3D Renderer (Vulkan)
     *
     * Deferred-submit renderer: Submit() queues draw commands,
     * EndScene() sorts by material/shader and records to the
     * active Vulkan command buffer.
     *
     * Push constants carry per-draw model matrix + normal matrix.
     * Camera and lighting data go in uniform buffers via descriptor sets.
     */
    class Renderer3D {
    public:
        static void Init();
        static void Shutdown();

        /**
         * @brief Set the command buffer for this frame's recording
         */
        static void SetCommandBuffer(VkCommandBuffer cmd);

        static void BeginScene(const Camera3D& camera);
        static void BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition);
        static void EndScene();

        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Material>& material, const glm::mat4& transform = glm::mat4(1.0f));
        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));

        static void SubmitLight(const Light& light, const glm::vec3& position, const glm::vec3& direction = glm::vec3(0, -1, 0));
        static void SetAmbientLight(const glm::vec3& color, float intensity = 0.1f);
        static void ClearLights();

        static const std::vector<LightData>& GetLights() { return s_Lights; }

        /**
         * @brief Flush the render queue for a shadow pass
         */
        static void FlushShadowPass(VkCommandBuffer cmd, VkPipelineLayout layout,
                                     const glm::mat4& lightSpaceMatrix);

        static const std::vector<DrawCommand>& GetRenderQueue() { return s_RenderQueue; }

        static const glm::mat4& GetViewProjection() { return s_SceneData.ViewProjectionMatrix; }
        static const glm::vec3& GetCameraPosition() { return s_SceneData.CameraPosition; }

        static constexpr int MAX_LIGHTS = 16;

        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t VertexCount = 0;
            uint32_t IndexCount = 0;
            uint32_t CulledObjects = 0;
            void Reset() { DrawCalls = 0; VertexCount = 0; IndexCount = 0; CulledObjects = 0; }
        };

        static const Statistics& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats.Reset(); }

        /**
         * @brief Get per-frame camera UBO descriptor info
         */
        static VkDescriptorBufferInfo GetCameraDescriptorInfo();

        /**
         * @brief Get per-frame lighting UBO descriptor info
         */
        static VkDescriptorBufferInfo GetLightingDescriptorInfo();

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

        static VkCommandBuffer s_ActiveCmd;
        static uint32_t s_CurrentFrame;

        static constexpr uint32_t kCameraUBOBinding = 0;
        static constexpr uint32_t kLightingUBOBinding = 1;
        static Scope<UniformBuffer> s_CameraUBO;
        static Scope<UniformBuffer> s_LightingUBO;
        static Frustum s_Frustum;
    };
}
