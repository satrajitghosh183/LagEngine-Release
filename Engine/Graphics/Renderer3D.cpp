#include "Renderer3D.hpp"
#include "RenderCommand.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    Renderer3D::SceneData Renderer3D::s_SceneData;
    Renderer3D::Statistics Renderer3D::s_Stats;

    void Renderer3D::Init() {
        GE_CORE_INFO("Renderer3D initialized");
    }

    void Renderer3D::Shutdown() {
        GE_CORE_INFO("Renderer3D shutdown");
    }

    void Renderer3D::BeginScene(const Camera3D& camera) {
        s_SceneData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        s_SceneData.CameraPosition = camera.GetPosition();
        
        s_Stats.Reset();
    }

    void Renderer3D::EndScene() {
        // Flush any remaining batches (if batching is implemented)
    }

    void Renderer3D::Submit(const Ref<Mesh3D>& mesh, const Ref<Material>& material, const glm::mat4& transform) {
        // Bind material (shader + textures)
        material->Bind();
        
        // Set common uniforms
        auto shader = material->GetShader();
        shader->SetUniformMat4("u_ViewProjection", s_SceneData.ViewProjectionMatrix);
        shader->SetUniformMat4("u_Model", transform);
        shader->SetUniformVec3("u_CameraPosition", s_SceneData.CameraPosition);
        
        // Calculate normal matrix (inverse transpose of model matrix)
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        shader->SetUniformMat3("u_NormalMatrix", normalMatrix);
        
        // Draw mesh
        mesh->Draw();
        
        // Update stats
        s_Stats.DrawCalls++;
        s_Stats.VertexCount += mesh->GetVertexCount();
        s_Stats.IndexCount += mesh->GetIndexCount();
        
        material->Unbind();
    }

    void Renderer3D::Submit(const Ref<Mesh3D>& mesh, const Ref<Shader>& shader, const glm::mat4& transform) {
        shader->Bind();
        
        // Set common uniforms
        shader->SetUniformMat4("u_ViewProjection", s_SceneData.ViewProjectionMatrix);
        shader->SetUniformMat4("u_Model", transform);
        shader->SetUniformVec3("u_CameraPosition", s_SceneData.CameraPosition);
        
        // Calculate normal matrix
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
        shader->SetUniformMat3("u_NormalMatrix", normalMatrix);
        
        // Draw mesh
        mesh->Draw();
        
        // Update stats
        s_Stats.DrawCalls++;
        s_Stats.VertexCount += mesh->GetVertexCount();
        s_Stats.IndexCount += mesh->GetIndexCount();
        
        shader->Unbind();
    }
}