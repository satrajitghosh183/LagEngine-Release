#include "SSAO.hpp"
#include "Shader.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <random>

namespace GameEngine {

    SSAO::~SSAO() { Cleanup(); }

    void SSAO::Init(int width, int height) {
        m_Width = width;
        m_Height = height;
        GenerateKernel();
        GenerateNoiseTexture();
        CreateFramebuffers(width, height);

        const char* ssaoVert = R"(
#version 420 core
out vec2 v_TexCoord;
void main() {
    float x = -1.0 + float((gl_VertexID & 1) << 2);
    float y = -1.0 + float((gl_VertexID & 2) << 1);
    gl_Position = vec4(x, y, 0.0, 1.0);
    v_TexCoord = (gl_Position.xy * 0.5 + 0.5);
})";

        const char* ssaoFrag = R"(
#version 420 core
out float FragColor;
in vec2 v_TexCoord;
uniform sampler2D u_PositionTex;
uniform sampler2D u_NormalTex;
uniform sampler2D u_NoiseTex;
uniform vec3 u_Samples[64];
uniform mat4 u_Projection;
uniform vec2 u_NoiseScale;
uniform float u_Radius;
uniform float u_Bias;
uniform int u_KernelSize;

void main() {
    vec3 fragPos = texture(u_PositionTex, v_TexCoord).xyz;
    vec3 normal = normalize(texture(u_NormalTex, v_TexCoord).xyz);
    vec3 randomVec = normalize(texture(u_NoiseTex, v_TexCoord * u_NoiseScale).xyz);
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    float occlusion = 0.0;
    for (int i = 0; i < u_KernelSize; i++) {
        vec3 samplePos = TBN * u_Samples[i];
        samplePos = fragPos + samplePos * u_Radius;
        vec4 offset = u_Projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;
        float sampleDepth = texture(u_PositionTex, offset.xy).z;
        float rangeCheck = smoothstep(0.0, 1.0, u_Radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + u_Bias ? 1.0 : 0.0) * rangeCheck;
    }
    FragColor = 1.0 - (occlusion / float(u_KernelSize));
})";

        const char* blurFrag = R"(
#version 420 core
out float FragColor;
in vec2 v_TexCoord;
uniform sampler2D u_SSAOInput;
void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(u_SSAOInput, 0));
    float result = 0.0;
    for (int x = -2; x < 2; x++) {
        for (int y = -2; y < 2; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(u_SSAOInput, v_TexCoord + offset).r;
        }
    }
    FragColor = result / 16.0;
})";

        m_SSAOShader = CreateRef<Shader>("SSAO", ssaoVert, ssaoFrag);
        m_BlurShader = CreateRef<Shader>("SSAOBlur", ssaoVert, blurFrag);
    }

    void SSAO::Resize(int width, int height) {
        Cleanup();
        m_Width = width;
        m_Height = height;
        CreateFramebuffers(width, height);
    }

    void SSAO::GenerateKernel() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::default_random_engine gen;
        m_Kernel.clear();
        for (int i = 0; i < 64; i++) {
            glm::vec3 sample(dist(gen) * 2.0f - 1.0f, dist(gen) * 2.0f - 1.0f, dist(gen));
            sample = glm::normalize(sample) * dist(gen);
            float scale = float(i) / 64.0f;
            scale = 0.1f + scale * scale * 0.9f; // Lerp
            sample *= scale;
            m_Kernel.push_back(sample);
        }
    }

    void SSAO::GenerateNoiseTexture() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::default_random_engine gen;
        std::vector<glm::vec3> noise;
        for (int i = 0; i < 16; i++) {
            noise.emplace_back(dist(gen) * 2.0f - 1.0f, dist(gen) * 2.0f - 1.0f, 0.0f);
        }
        glGenTextures(1, &m_NoiseTexture);
        glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, noise.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    void SSAO::CreateFramebuffers(int width, int height) {
        // SSAO FBO
        glGenFramebuffers(1, &m_SSAOFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_SSAOFBO);
        glGenTextures(1, &m_SSAOTexture);
        glBindTexture(GL_TEXTURE_2D, m_SSAOTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAOTexture, 0);

        // Blur FBO
        glGenFramebuffers(1, &m_SSAOBlurFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_SSAOBlurFBO);
        glGenTextures(1, &m_SSAOBlurTexture);
        glBindTexture(GL_TEXTURE_2D, m_SSAOBlurTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAOBlurTexture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SSAO::Compute(uint32_t positionTex, uint32_t normalTex, const glm::mat4& projection) {
        if (!Enabled || !m_SSAOShader) return;

        glBindFramebuffer(GL_FRAMEBUFFER, m_SSAOFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        m_SSAOShader->Bind();

        for (int i = 0; i < static_cast<int>(m_Kernel.size()) && i < 64; i++) {
            m_SSAOShader->SetUniformVec3("u_Samples[" + std::to_string(i) + "]", m_Kernel[i]);
        }
        m_SSAOShader->SetUniformMat4("u_Projection", projection);
        m_SSAOShader->SetUniformFloat("u_Radius", Radius);
        m_SSAOShader->SetUniformFloat("u_Bias", Bias);
        m_SSAOShader->SetUniformInt("u_KernelSize", KernelSize);
        m_SSAOShader->SetUniformVec2("u_NoiseScale", glm::vec2(m_Width / 4.0f, m_Height / 4.0f));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, positionTex);
        m_SSAOShader->SetUniformInt("u_PositionTex", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, normalTex);
        m_SSAOShader->SetUniformInt("u_NormalTex", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_NoiseTexture);
        m_SSAOShader->SetUniformInt("u_NoiseTex", 2);

        // Render fullscreen triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SSAO::Blur() {
        if (!m_BlurShader) return;
        glBindFramebuffer(GL_FRAMEBUFFER, m_SSAOBlurFBO);
        glClear(GL_COLOR_BUFFER_BIT);
        m_BlurShader->Bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_SSAOTexture);
        m_BlurShader->SetUniformInt("u_SSAOInput", 0);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void SSAO::Cleanup() {
        if (m_SSAOFBO) { glDeleteFramebuffers(1, &m_SSAOFBO); m_SSAOFBO = 0; }
        if (m_SSAOBlurFBO) { glDeleteFramebuffers(1, &m_SSAOBlurFBO); m_SSAOBlurFBO = 0; }
        if (m_SSAOTexture) { glDeleteTextures(1, &m_SSAOTexture); m_SSAOTexture = 0; }
        if (m_SSAOBlurTexture) { glDeleteTextures(1, &m_SSAOBlurTexture); m_SSAOBlurTexture = 0; }
        if (m_NoiseTexture) { glDeleteTextures(1, &m_NoiseTexture); m_NoiseTexture = 0; }
    }

}
