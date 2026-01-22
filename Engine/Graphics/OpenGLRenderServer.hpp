#pragma once

#include "../Core/Runtime/IRenderServer.hpp"
#include "Renderer3D.hpp"
#include "RenderCommand.hpp"
#include "Camera3D.hpp"
#include "Light.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief OpenGL implementation of render server
     * 
     * Wraps Renderer3D and RenderCommand behind IRenderServer interface
     */
    class OpenGLRenderServer : public IRenderServer {
    public:
        OpenGLRenderServer() = default;
        ~OpenGLRenderServer() override = default;

        void Initialize() override;
        void Shutdown() override;

        void BeginFrame() override;
        void EndFrame() override;

        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

        void SubmitMesh(Ref<Mesh3D> mesh, Ref<Material> material, const glm::mat4& transform) override;

        void SubmitLight(const Light& light) override;

        void SetCamera(const Camera3D& camera) override;

        void Clear(const glm::vec4& color) override;

    private:
        Camera3D* m_CurrentCamera = nullptr;
        std::vector<Light> m_Lights;
    };

}

