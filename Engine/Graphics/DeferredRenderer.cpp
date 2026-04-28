#include "DeferredRenderer.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    void DeferredRenderer::Init(int width, int height) {
        m_Width = width;
        m_Height = height;
        m_GBuffer.Init(width, height);
        CreateFullscreenQuad();

        GE_CORE_INFO("Deferred renderer initialized ({0}x{1})", width, height);
    }

    void DeferredRenderer::Resize(int width, int height) {
        m_Width = width;
        m_Height = height;
        m_GBuffer.Resize(width, height);
    }

    void DeferredRenderer::Shutdown() {
        m_QuadVB.reset();
        m_QuadIB.reset();
    }

    void DeferredRenderer::BeginGeometryPass(VkCommandBuffer cmd) {
        m_GBuffer.BeginRenderPass(cmd);
    }

    void DeferredRenderer::EndGeometryPass(VkCommandBuffer cmd) {
        m_GBuffer.EndRenderPass(cmd);
    }

    void DeferredRenderer::RenderLightingPass(VkCommandBuffer cmd,
                                               const glm::mat4& projection,
                                               const glm::vec3& cameraPos) {
        (void)projection;
        (void)cameraPos;

        // Bind fullscreen quad and draw
        // The caller is responsible for:
        //   1. Beginning the output render pass (swapchain or offscreen)
        //   2. Binding the lighting pipeline
        //   3. Binding descriptor sets with G-Buffer textures + camera/lighting UBOs
        // This just records the fullscreen triangle/quad draw.

        if (m_QuadVB && m_QuadIB) {
            m_QuadVB->Bind(cmd);
            m_QuadIB->Bind(cmd);
            vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
        }
    }

    void DeferredRenderer::CreateFullscreenQuad() {
        struct QuadVertex {
            float x, y, z;
            float u, v;
        };

        QuadVertex vertices[] = {
            {-1.0f,  1.0f, 0.0f,  0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f,  0.0f, 0.0f},
            { 1.0f, -1.0f, 0.0f,  1.0f, 0.0f},
            { 1.0f,  1.0f, 0.0f,  1.0f, 1.0f},
        };

        uint32_t indices[] = {0, 1, 2, 0, 2, 3};

        m_QuadVB = CreateRef<VertexBuffer>(vertices, sizeof(vertices));
        m_QuadVB->SetLayout({
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float2, "a_TexCoord"}
        });

        m_QuadIB = CreateRef<IndexBuffer>(indices, 6);
    }

}
