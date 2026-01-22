#include "OpenGLRenderServer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    void OpenGLRenderServer::Initialize() {
        RenderCommand::Init();
        Renderer3D::Init();
        GE_CORE_INFO("OpenGLRenderServer initialized");
    }

    void OpenGLRenderServer::Shutdown() {
        Renderer3D::Shutdown();
        GE_CORE_INFO("OpenGLRenderServer shutdown");
    }

    void OpenGLRenderServer::BeginFrame() {
        m_Lights.clear();
        // Renderer3D::BeginScene will be called when camera is set
    }

    void OpenGLRenderServer::EndFrame() {
        if (m_CurrentCamera) {
            Renderer3D::EndScene();
        }
    }

    void OpenGLRenderServer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        RenderCommand::SetViewport(x, y, width, height);
    }

    void OpenGLRenderServer::SubmitMesh(Ref<Mesh3D> mesh, Ref<Material> material, const glm::mat4& transform) {
        if (!m_CurrentCamera) {
            GE_CORE_WARN("SubmitMesh called without setting camera");
            return;
        }
        Renderer3D::Submit(mesh, material, transform);
    }

    void OpenGLRenderServer::SubmitLight(const Light& light) {
        m_Lights.push_back(light);
        // TODO: Integrate lights into Renderer3D when lighting system is implemented
    }

    void OpenGLRenderServer::SetCamera(const Camera3D& camera) {
        m_CurrentCamera = const_cast<Camera3D*>(&camera);
        Renderer3D::BeginScene(camera);
    }

    void OpenGLRenderServer::Clear(const glm::vec4& color) {
        RenderCommand::SetClearColor(color);
        RenderCommand::Clear();
    }

}

