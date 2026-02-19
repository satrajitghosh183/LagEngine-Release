#pragma once

#include "../Core/Base.hpp"
#include "GBuffer.hpp"
#include "SSAO.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    class DeferredRenderer {
    public:
        void Init(int width, int height);
        void Resize(int width, int height);
        void Shutdown();

        void BeginGeometryPass();
        void EndGeometryPass();
        void RenderLightingPass(const glm::mat4& projection, const glm::vec3& cameraPos);

        Ref<Shader> GetGeometryShader() { return m_GeometryShader; }
        GBuffer& GetGBuffer() { return m_GBuffer; }
        SSAO& GetSSAO() { return m_SSAO; }

        bool SSAOEnabled = true;

    private:
        void CreateFullscreenQuad();
        void RenderFullscreenQuad();

        GBuffer m_GBuffer;
        SSAO m_SSAO;
        Ref<Shader> m_GeometryShader;
        Ref<Shader> m_LightingShader;

        uint32_t m_QuadVAO = 0, m_QuadVBO = 0;
        int m_Width = 0, m_Height = 0;
    };

}
