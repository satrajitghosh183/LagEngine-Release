#include "DeferredRenderer.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>

namespace GameEngine {

    void DeferredRenderer::Init(int width, int height) {
        m_Width = width;
        m_Height = height;
        m_GBuffer.Init(width, height);
        m_SSAO.Init(width, height);
        CreateFullscreenQuad();

        // Geometry pass shader (deferred GBuffer output)
        const char* geoVert = R"(
#version 420 core
layout(location=0) in vec3 a_Position;
layout(location=1) in vec3 a_Normal;
layout(location=2) in vec2 a_TexCoord;
uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat3 u_NormalMatrix;
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
void main() {
    vec4 wp = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = wp.xyz;
    v_Normal = u_NormalMatrix * a_Normal;
    v_TexCoord = a_TexCoord;
    gl_Position = u_ViewProjection * wp;
})";

        const char* geoFrag = R"(
#version 420 core
layout(location=0) out vec3 gPosition;
layout(location=1) out vec3 gNormal;
layout(location=2) out vec4 gAlbedo;
layout(location=3) out vec2 gMetallicRoughness;
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
struct MaterialProperties { vec3 albedo; float metallic; float roughness; float ao; };
uniform MaterialProperties u_Material;
void main() {
    gPosition = v_WorldPos;
    gNormal = normalize(v_Normal);
    gAlbedo = vec4(u_Material.albedo, 1.0);
    gMetallicRoughness = vec2(u_Material.metallic, u_Material.roughness);
})";

        // Lighting pass shader
        const char* lightVert = R"(
#version 420 core
layout(location=0) in vec3 a_Position;
layout(location=1) in vec2 a_TexCoord;
out vec2 v_TexCoord;
void main() { v_TexCoord = a_TexCoord; gl_Position = vec4(a_Position, 1.0); })";

        const char* lightFrag = R"(
#version 420 core
out vec4 FragColor;
in vec2 v_TexCoord;
uniform sampler2D u_GPosition;
uniform sampler2D u_GNormal;
uniform sampler2D u_GAlbedo;
uniform sampler2D u_GMetallicRoughness;
uniform sampler2D u_SSAOTex;
uniform vec3 u_CameraPosition;
uniform int u_SSAOEnabled;
#define MAX_LIGHTS 16
struct LightInfo { int type; vec3 position; vec3 direction; vec3 color; float intensity; float range; };
uniform LightInfo u_Lights[MAX_LIGHTS];
uniform int u_LightCount;
const float PI = 3.14159265359;
void main() {
    vec3 fragPos = texture(u_GPosition, v_TexCoord).rgb;
    vec3 normal = texture(u_GNormal, v_TexCoord).rgb;
    vec3 albedo = texture(u_GAlbedo, v_TexCoord).rgb;
    vec2 mr = texture(u_GMetallicRoughness, v_TexCoord).rg;
    float ssao = u_SSAOEnabled != 0 ? texture(u_SSAOTex, v_TexCoord).r : 1.0;
    vec3 V = normalize(u_CameraPosition - fragPos);
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < u_LightCount && i < MAX_LIGHTS; i++) {
        vec3 L = u_Lights[i].type == 0 ? normalize(-u_Lights[i].direction) : normalize(u_Lights[i].position - fragPos);
        float NdotL = max(dot(normal, L), 0.0);
        float att = 1.0;
        if (u_Lights[i].type != 0) {
            float d = length(u_Lights[i].position - fragPos);
            att = 1.0 / (1.0 + d * d / max(u_Lights[i].range * u_Lights[i].range, 0.01));
        }
        Lo += albedo * u_Lights[i].color * u_Lights[i].intensity * NdotL * att;
    }
    vec3 ambient = vec3(0.1) * albedo * ssao;
    vec3 color = ambient + Lo;
    color = pow(color / (color + vec3(1.0)), vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
})";

        m_GeometryShader = CreateRef<Shader>("DeferredGeometry", geoVert, geoFrag);
        m_LightingShader = CreateRef<Shader>("DeferredLighting", lightVert, lightFrag);

        GE_CORE_INFO("Deferred renderer initialized ({}x{})", width, height);
    }

    void DeferredRenderer::Resize(int width, int height) {
        m_Width = width; m_Height = height;
        m_GBuffer.Resize(width, height);
        m_SSAO.Resize(width, height);
    }

    void DeferredRenderer::Shutdown() {
        if (m_QuadVAO) { glDeleteVertexArrays(1, &m_QuadVAO); m_QuadVAO = 0; }
        if (m_QuadVBO) { glDeleteBuffers(1, &m_QuadVBO); m_QuadVBO = 0; }
    }

    void DeferredRenderer::BeginGeometryPass() {
        m_GBuffer.BindForWriting();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
    }

    void DeferredRenderer::EndGeometryPass() {
        m_GBuffer.Unbind();
    }

    void DeferredRenderer::RenderLightingPass(const glm::mat4& projection, const glm::vec3& cameraPos) {
        // SSAO pass
        if (SSAOEnabled) {
            m_SSAO.Compute(m_GBuffer.GetPositionTexture(), m_GBuffer.GetNormalTexture(), projection);
            m_SSAO.Blur();
        }

        // Lighting pass
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        m_LightingShader->Bind();

        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_GBuffer.GetPositionTexture());
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_GBuffer.GetNormalTexture());
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_GBuffer.GetAlbedoTexture());
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_GBuffer.GetMetallicRoughnessTexture());
        glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, m_SSAO.GetSSAOTexture());

        m_LightingShader->SetUniformInt("u_GPosition", 0);
        m_LightingShader->SetUniformInt("u_GNormal", 1);
        m_LightingShader->SetUniformInt("u_GAlbedo", 2);
        m_LightingShader->SetUniformInt("u_GMetallicRoughness", 3);
        m_LightingShader->SetUniformInt("u_SSAOTex", 4);
        m_LightingShader->SetUniformInt("u_SSAOEnabled", SSAOEnabled ? 1 : 0);
        m_LightingShader->SetUniformVec3("u_CameraPosition", cameraPos);

        RenderFullscreenQuad();
        glEnable(GL_DEPTH_TEST);
    }

    void DeferredRenderer::CreateFullscreenQuad() {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        };
        glGenVertexArrays(1, &m_QuadVAO);
        glGenBuffers(1, &m_QuadVBO);
        glBindVertexArray(m_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }

    void DeferredRenderer::RenderFullscreenQuad() {
        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

}
