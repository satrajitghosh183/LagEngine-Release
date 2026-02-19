#pragma once

#include "../Core/Base.hpp"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    class Shader;

    /**
     * @brief Screen-Space Ambient Occlusion
     *
     * Computes per-pixel ambient occlusion by sampling a hemisphere kernel
     * around each fragment in view-space, checking depth discontinuities.
     * A 4x4 box blur is applied to smooth the result.
     *
     * Usage:
     *   ssao.Compute(gBuffer.GetPositionTexture(), gBuffer.GetNormalTexture(), projection);
     *   ssao.Blur();
     *   uint32_t aoTex = ssao.GetSSAOTexture();
     */
    class SSAO {
    public:
        SSAO() = default;
        ~SSAO();

        void Init(int width, int height);
        void Resize(int width, int height);
        void Compute(uint32_t positionTex, uint32_t normalTex, const glm::mat4& projection);
        void Blur();
        uint32_t GetSSAOTexture() const { return m_SSAOBlurTexture; }

        float Radius = 0.5f;
        float Bias = 0.025f;
        int KernelSize = 32;
        bool Enabled = true;

    private:
        void GenerateKernel();
        void GenerateNoiseTexture();
        void CreateFramebuffers(int width, int height);
        void CreateShaders();
        void Cleanup();

        static void RenderFullscreenQuad();

        uint32_t m_SSAOFBO = 0, m_SSAOBlurFBO = 0;
        uint32_t m_SSAOTexture = 0, m_SSAOBlurTexture = 0;
        uint32_t m_NoiseTexture = 0;
        uint32_t m_QuadVAO = 0, m_QuadVBO = 0;
        std::vector<glm::vec3> m_Kernel;
        Ref<Shader> m_SSAOShader;
        Ref<Shader> m_BlurShader;
        int m_Width = 0, m_Height = 0;
    };

}
