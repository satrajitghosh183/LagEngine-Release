#include "Renderer3D.hpp"
#include "RenderCommand.hpp"
#include "IBL.hpp"
#include "UniformBuffer.hpp"
#include "Frustum.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <sstream>
#include <functional>
#include <cstring>

namespace GameEngine {

    Renderer3D::SceneData Renderer3D::s_SceneData;
    Renderer3D::Statistics Renderer3D::s_Stats;
    std::vector<LightData> Renderer3D::s_Lights;
    std::vector<DrawCommand> Renderer3D::s_RenderQueue;
    Scope<UniformBuffer> Renderer3D::s_CameraUBO;
    Scope<UniformBuffer> Renderer3D::s_LightingUBO;
    Frustum Renderer3D::s_Frustum;

    void Renderer3D::Init() {
        s_SceneData.AmbientColor = glm::vec3(1.0f);
        s_SceneData.AmbientIntensity = 0.1f;

        s_Lights.reserve(MAX_LIGHTS);
        s_RenderQueue.reserve(256);

        // Camera UBO: mat4 viewProj (64) + vec3 cameraPos (16) = 80 bytes (std140)
        s_CameraUBO = CreateScope<UniformBuffer>();
        s_CameraUBO->Init(80, kCameraUBOBinding, true);

        // Lighting UBO: ambient(vec3+float) + lightCount(int) + padding + 16 lights (e.g. type, pos, dir, color, intensity each)
        const size_t lightStride = 64;
        const size_t lightingUBOSize = 32 + MAX_LIGHTS * lightStride;
        s_LightingUBO = CreateScope<UniformBuffer>();
        s_LightingUBO->Init(lightingUBOSize, kLightingUBOBinding, true);

        GE_CORE_INFO("Renderer3D initialized (deferred queue, UBOs, max {} lights)", MAX_LIGHTS);
    }

    void Renderer3D::Shutdown() {
        if (s_CameraUBO) s_CameraUBO->Shutdown();
        if (s_LightingUBO) s_LightingUBO->Shutdown();
        s_Lights.clear();
        s_RenderQueue.clear();
        GE_CORE_INFO("Renderer3D shutdown");
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
        s_CameraUBO->Bind();
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
        s_LightingUBO->Bind();
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

        uint32_t shaderID = material->GetShader() ? material->GetShader()->GetRendererID() : 0;
        uint32_t matHash = static_cast<uint32_t>(std::hash<void*>{}(material.get()));
        cmd.SortKey = (static_cast<uint64_t>(shaderID) << 32) | static_cast<uint64_t>(matHash);

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

        cmd.SortKey = static_cast<uint64_t>(shader ? shader->GetRendererID() : 0) << 32;

        s_RenderQueue.push_back(cmd);
    }

    void Renderer3D::FlushQueue() {
        for (const auto& cmd : s_RenderQueue) {
            ExecuteDrawCommand(cmd);
        }
    }

    void Renderer3D::ExecuteDrawCommand(const DrawCommand& cmd) {
        Ref<Shader> shader;

        if (cmd.Mat) {
            cmd.Mat->Bind();
            shader = cmd.Mat->GetShader();
        } else if (cmd.ShaderOnly) {
            cmd.ShaderOnly->Bind();
            shader = cmd.ShaderOnly;
        } else {
            return;
        }

        // Set common uniforms
        shader->SetUniformMat4("u_ViewProjection", s_SceneData.ViewProjectionMatrix);
        shader->SetUniformMat4("u_Model", cmd.Transform);
        shader->SetUniformMat4("u_Transform", cmd.Transform);
        shader->SetUniformVec3("u_CameraPosition", s_SceneData.CameraPosition);

        // Normal matrix
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(cmd.Transform)));
        shader->SetUniformMat3("u_NormalMatrix", normalMatrix);

        // Apply lighting
        ApplyLighting(shader);

        // Apply IBL if available
        if (IBL::IsReady()) {
            IBL::ApplyToShader(shader);
        }

        // Draw
        cmd.Mesh->Draw();

        // Stats
        s_Stats.DrawCalls++;
        s_Stats.VertexCount += cmd.Mesh->GetVertexCount();
        s_Stats.IndexCount += cmd.Mesh->GetIndexCount();

        if (cmd.Mat) {
            cmd.Mat->Unbind();
        } else {
            shader->Unbind();
        }
    }

    void Renderer3D::FlushShadowPass(const Ref<Shader>& depthShader, const glm::mat4& lightSpaceMatrix) {
        depthShader->Bind();

        for (const auto& cmd : s_RenderQueue) {
            depthShader->SetUniformMat4("u_LightSpaceMatrix", lightSpaceMatrix);
            depthShader->SetUniformMat4("u_Model", cmd.Transform);
            cmd.Mesh->Draw();
            s_Stats.DrawCalls++;
        }

        depthShader->Unbind();
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

    void Renderer3D::ApplyLighting(const Ref<Shader>& shader) {
        shader->SetUniformVec3("u_AmbientLight.color", s_SceneData.AmbientColor);
        shader->SetUniformFloat("u_AmbientLight.intensity", s_SceneData.AmbientIntensity);

        int lightCount = static_cast<int>(s_Lights.size());
        shader->SetUniformInt("u_LightCount", lightCount);

        for (int i = 0; i < lightCount; i++) {
            const auto& light = s_Lights[i];
            std::string prefix = "u_Lights[" + std::to_string(i) + "]";

            shader->SetUniformInt(prefix + ".type", static_cast<int>(light.Properties.Type));
            shader->SetUniformVec3(prefix + ".position", light.Position);
            shader->SetUniformVec3(prefix + ".direction", light.Direction);
            shader->SetUniformVec3(prefix + ".color", light.Properties.Color);
            shader->SetUniformFloat(prefix + ".intensity", light.Properties.Intensity);
            shader->SetUniformFloat(prefix + ".constant", light.Properties.Constant);
            shader->SetUniformFloat(prefix + ".linear", light.Properties.Linear);
            shader->SetUniformFloat(prefix + ".quadratic", light.Properties.Quadratic);
            shader->SetUniformFloat(prefix + ".range", light.Properties.Range);
            shader->SetUniformFloat(prefix + ".innerCutoff", glm::cos(glm::radians(light.Properties.InnerConeAngle)));
            shader->SetUniformFloat(prefix + ".outerCutoff", glm::cos(glm::radians(light.Properties.OuterConeAngle)));
        }
    }
}
