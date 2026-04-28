#include "Renderer3D.hpp"
#include "RenderCommand.hpp"
#include "Frustum.hpp"
#include "UniformBuffer.hpp"
#include "../Core/Logger.hpp"
#include <cstring>
#include <functional>

namespace GameEngine {

    Renderer3D::SceneData Renderer3D::s_SceneData;
    Renderer3D::Statistics Renderer3D::s_Stats;
    std::vector<LightData> Renderer3D::s_Lights;
    std::vector<DrawCommand> Renderer3D::s_RenderQueue;
    VkCommandBuffer Renderer3D::s_ActiveCmd = VK_NULL_HANDLE;
    uint32_t Renderer3D::s_CurrentFrame = 0;
    Scope<UniformBuffer> Renderer3D::s_CameraUBO;
    Scope<UniformBuffer> Renderer3D::s_LightingUBO;
    Frustum Renderer3D::s_Frustum;

    void Renderer3D::Init() {
        s_SceneData.AmbientColor = glm::vec3(1.0f);
        s_SceneData.AmbientIntensity = 0.1f;

        s_Lights.reserve(MAX_LIGHTS);
        s_RenderQueue.reserve(256);

        // Camera UBO: mat4 viewProj (64) + vec3 cameraPos (12) + pad (4) = 80 bytes
        s_CameraUBO = CreateScope<UniformBuffer>();
        s_CameraUBO->Init(80, kCameraUBOBinding, 2);

        // Lighting UBO: ambient(16) + lightCount(16) + 16 lights * 64 bytes = 1056 bytes
        const size_t lightStride = 64;
        const size_t lightingUBOSize = 32 + MAX_LIGHTS * lightStride;
        s_LightingUBO = CreateScope<UniformBuffer>();
        s_LightingUBO->Init(lightingUBOSize, kLightingUBOBinding, 2);

        GE_CORE_INFO("Renderer3D initialized (Vulkan, max {} lights)", MAX_LIGHTS);
    }

    void Renderer3D::Shutdown() {
        if (s_CameraUBO) s_CameraUBO->Shutdown();
        if (s_LightingUBO) s_LightingUBO->Shutdown();
        s_Lights.clear();
        s_RenderQueue.clear();
        GE_CORE_INFO("Renderer3D shutdown");
    }

    void Renderer3D::SetCommandBuffer(VkCommandBuffer cmd) {
        s_ActiveCmd = cmd;
        RenderCommand::SetCommandBuffer(cmd);
    }

    void Renderer3D::UploadCameraUBO() {
        if (!s_CameraUBO || !s_CameraUBO->IsValid()) return;
        struct CameraUBOData {
            glm::mat4 ViewProjection;
            glm::vec3 CameraPosition;
            float _pad;
        } data;
        data.ViewProjection = s_SceneData.ViewProjectionMatrix;
        data.CameraPosition = s_SceneData.CameraPosition;
        data._pad = 0.0f;
        s_CameraUBO->SetData(&data, sizeof(data));
    }

    void Renderer3D::UploadLightingUBO() {
        if (!s_LightingUBO || !s_LightingUBO->IsValid()) return;
        struct LightingUBOData {
            glm::vec3 AmbientColor;
            float AmbientIntensity;
            int LightCount;
            float _pad[3];
            struct LightEntry {
                int Type;
                float _pad0[3];
                glm::vec3 Position;
                float _pad1;
                glm::vec3 Direction;
                float _pad2;
                glm::vec3 Color;
                float Intensity;
            } Lights[MAX_LIGHTS];
        } data;
        memset(&data, 0, sizeof(data));
        data.AmbientColor = s_SceneData.AmbientColor;
        data.AmbientIntensity = s_SceneData.AmbientIntensity;
        data.LightCount = static_cast<int>(s_Lights.size());
        for (size_t i = 0; i < s_Lights.size() && i < static_cast<size_t>(MAX_LIGHTS); i++) {
            data.Lights[i].Type = static_cast<int>(s_Lights[i].Properties.Type);
            data.Lights[i].Position = s_Lights[i].Position;
            data.Lights[i].Direction = s_Lights[i].Direction;
            data.Lights[i].Color = s_Lights[i].Properties.Color;
            data.Lights[i].Intensity = s_Lights[i].Properties.Intensity;
        }
        s_LightingUBO->SetData(&data, sizeof(data));
    }

    void Renderer3D::BeginScene(const Camera3D& camera) {
        s_SceneData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        s_SceneData.CameraPosition = camera.GetPosition();
        ClearLights();
        s_RenderQueue.clear();
        s_Stats.Reset();
        UploadCameraUBO();
        s_Frustum.Update(s_SceneData.ViewProjectionMatrix);
    }

    void Renderer3D::BeginScene(const glm::mat4& viewProjection, const glm::vec3& cameraPosition) {
        s_SceneData.ViewProjectionMatrix = viewProjection;
        s_SceneData.CameraPosition = cameraPosition;
        ClearLights();
        s_RenderQueue.clear();
        s_Stats.Reset();
        UploadCameraUBO();
        s_Frustum.Update(s_SceneData.ViewProjectionMatrix);
    }

    void Renderer3D::EndScene() {
        UploadLightingUBO();
        std::sort(s_RenderQueue.begin(), s_RenderQueue.end(),
            [](const DrawCommand& a, const DrawCommand& b) {
                return a.SortKey < b.SortKey;
            });
        FlushQueue();
    }

    void Renderer3D::Submit(const Ref<Mesh3D>& mesh, const Ref<Material>& material, const glm::mat4& transform) {
        if (!mesh) return;
        if (!s_Frustum.IsAABBVisible(mesh->GetAABB(), transform)) {
            s_Stats.CulledObjects++;
            return;
        }
        DrawCommand cmd;
        cmd.Mesh = mesh;
        cmd.Mat = material;
        cmd.Transform = transform;

        uint32_t matHash = static_cast<uint32_t>(std::hash<void*>{}(material.get()));
        cmd.SortKey = static_cast<uint64_t>(matHash);

        s_RenderQueue.push_back(cmd);
    }

    void Renderer3D::Submit(const Ref<Mesh3D>& mesh, const Ref<Shader>& shader, const glm::mat4& transform) {
        if (!mesh) return;
        if (!s_Frustum.IsAABBVisible(mesh->GetAABB(), transform)) {
            s_Stats.CulledObjects++;
            return;
        }
        DrawCommand cmd;
        cmd.Mesh = mesh;
        cmd.ShaderOnly = shader;
        cmd.Transform = transform;
        cmd.SortKey = 0;

        s_RenderQueue.push_back(cmd);
    }

    void Renderer3D::FlushQueue() {
        if (s_ActiveCmd == VK_NULL_HANDLE) return;

        for (const auto& cmd : s_RenderQueue) {
            ExecuteDrawCommand(cmd);
        }
    }

    void Renderer3D::ExecuteDrawCommand(const DrawCommand& cmd) {
        if (s_ActiveCmd == VK_NULL_HANDLE) return;

        // Record draw commands
        cmd.Mesh->Draw(s_ActiveCmd);

        s_Stats.DrawCalls++;
        s_Stats.VertexCount += cmd.Mesh->GetVertexCount();
        s_Stats.IndexCount += cmd.Mesh->GetIndexCount();
    }

    void Renderer3D::FlushShadowPass(VkCommandBuffer cmd, VkPipelineLayout layout,
                                      const glm::mat4& lightSpaceMatrix) {
        struct ShadowPushConstants {
            glm::mat4 LightSpaceMatrix;
            glm::mat4 Model;
        } push;

        push.LightSpaceMatrix = lightSpaceMatrix;

        for (const auto& drawCmd : s_RenderQueue) {
            push.Model = drawCmd.Transform;
            vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);
            drawCmd.Mesh->Draw(cmd);
            s_Stats.DrawCalls++;
        }
    }

    void Renderer3D::SubmitLight(const Light& light, const glm::vec3& position, const glm::vec3& direction) {
        if (s_Lights.size() >= MAX_LIGHTS) {
            GE_CORE_WARN("Renderer3D: Maximum lights ({}) exceeded", MAX_LIGHTS);
            return;
        }

        LightData data;
        data.Properties = light;
        data.Position = position;
        data.Direction = glm::normalize(direction);

        s_Lights.push_back(data);
    }

    void Renderer3D::SetAmbientLight(const glm::vec3& color, float intensity) {
        s_SceneData.AmbientColor = color;
        s_SceneData.AmbientIntensity = intensity;
    }

    void Renderer3D::ClearLights() {
        s_Lights.clear();
    }

    VkDescriptorBufferInfo Renderer3D::GetCameraDescriptorInfo() {
        return s_CameraUBO ? s_CameraUBO->GetDescriptorInfo() : VkDescriptorBufferInfo{};
    }

    VkDescriptorBufferInfo Renderer3D::GetLightingDescriptorInfo() {
        return s_LightingUBO ? s_LightingUBO->GetDescriptorInfo() : VkDescriptorBufferInfo{};
    }
}
