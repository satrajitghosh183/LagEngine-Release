#include "IBL.hpp"
#include "RenderCommand.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    uint32_t IBL::s_EnvCubemap = 0;
    uint32_t IBL::s_IrradianceMap = 0;
    uint32_t IBL::s_PrefilterMap = 0;
    uint32_t IBL::s_BRDFLUT = 0;
    bool IBL::s_Initialized = false;
    Ref<Shader> IBL::s_BRDFShader;

    // Cube VAO for rendering to cubemap faces
    static uint32_t s_CubeVAO = 0;
    static uint32_t s_CubeVBO = 0;
    static uint32_t s_QuadVAO = 0;
    static uint32_t s_QuadVBO = 0;

    static void CreateCube() {
        if (s_CubeVAO != 0) return;
        float vertices[] = {
            -1, -1, -1,  1, -1, -1,  1,  1, -1,  1,  1, -1, -1,  1, -1, -1, -1, -1,
            -1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1, -1,  1,
            -1,  1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1,  1,  1,
             1,  1,  1,  1,  1, -1,  1, -1, -1,  1, -1, -1,  1, -1,  1,  1,  1,  1,
            -1, -1, -1,  1, -1, -1,  1, -1,  1,  1, -1,  1, -1, -1,  1, -1, -1, -1,
            -1,  1, -1,  1,  1, -1,  1,  1,  1,  1,  1,  1, -1,  1,  1, -1,  1, -1
        };
        glGenVertexArrays(1, &s_CubeVAO);
        glGenBuffers(1, &s_CubeVBO);
        glBindVertexArray(s_CubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_CubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }

    static void CreateQuad() {
        if (s_QuadVAO != 0) return;
        float vertices[] = {
            -1, -1, 0, 0,
             1, -1, 1, 0,
             1,  1, 1, 1,
            -1, -1, 0, 0,
             1,  1, 1, 1,
            -1,  1, 0, 1
        };
        glGenVertexArrays(1, &s_QuadVAO);
        glGenBuffers(1, &s_QuadVBO);
        glBindVertexArray(s_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void IBL::Init() {
        if (s_Initialized) return;

        CreateCube();
        CreateQuad();
        GenerateBRDFLUT();
        GenerateProceduralCubemap();
        GenerateIrradianceMap();
        GeneratePrefilterMap();

        s_Initialized = true;
        GE_CORE_INFO("IBL initialized (procedural sky, BRDF LUT {}x{})", 512, 512);
    }

    void IBL::Shutdown() {
        if (s_EnvCubemap) glDeleteTextures(1, &s_EnvCubemap);
        if (s_IrradianceMap) glDeleteTextures(1, &s_IrradianceMap);
        if (s_PrefilterMap) glDeleteTextures(1, &s_PrefilterMap);
        if (s_BRDFLUT) glDeleteTextures(1, &s_BRDFLUT);
        if (s_CubeVAO) { glDeleteVertexArrays(1, &s_CubeVAO); glDeleteBuffers(1, &s_CubeVBO); }
        if (s_QuadVAO) { glDeleteVertexArrays(1, &s_QuadVAO); glDeleteBuffers(1, &s_QuadVBO); }
        s_EnvCubemap = s_IrradianceMap = s_PrefilterMap = s_BRDFLUT = 0;
        s_CubeVAO = s_CubeVBO = s_QuadVAO = s_QuadVBO = 0;
        s_Initialized = false;
        s_BRDFShader.reset();
    }

    void IBL::Bind(uint32_t irradianceSlot, uint32_t prefilterSlot, uint32_t brdfLUTSlot) {
        if (!s_Initialized) return;
        glActiveTexture(GL_TEXTURE0 + irradianceSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_IrradianceMap);
        glActiveTexture(GL_TEXTURE0 + prefilterSlot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_PrefilterMap);
        glActiveTexture(GL_TEXTURE0 + brdfLUTSlot);
        glBindTexture(GL_TEXTURE_2D, s_BRDFLUT);
    }

    void IBL::ApplyToShader(const Ref<Shader>& shader,
                            uint32_t irradianceSlot, uint32_t prefilterSlot, uint32_t brdfLUTSlot) {
        if (!s_Initialized) return;
        Bind(irradianceSlot, prefilterSlot, brdfLUTSlot);
        shader->SetUniformInt("u_IrradianceMap", static_cast<int>(irradianceSlot));
        shader->SetUniformInt("u_PrefilterMap", static_cast<int>(prefilterSlot));
        shader->SetUniformInt("u_BRDFLUT", static_cast<int>(brdfLUTSlot));
        shader->SetUniformInt("u_HasIBL", 1);
    }

    void IBL::GenerateBRDFLUT() {
        const char* brdfVert = R"(
            #version 420 core
            layout(location = 0) in vec2 a_Position;
            layout(location = 1) in vec2 a_TexCoord;
            out vec2 v_TexCoord;
            void main() {
                v_TexCoord = a_TexCoord;
                gl_Position = vec4(a_Position, 0.0, 1.0);
            }
        )";

        const char* brdfFrag = R"(
            #version 420 core
            in vec2 v_TexCoord;
            out vec2 FragColor;

            const float PI = 3.14159265359;

            float RadicalInverse_VdC(uint bits) {
                bits = (bits << 16u) | (bits >> 16u);
                bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
                bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
                bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
                bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
                return float(bits) * 2.3283064365386963e-10;
            }

            vec2 Hammersley(uint i, uint N) {
                return vec2(float(i) / float(N), RadicalInverse_VdC(i));
            }

            vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
                float a = roughness * roughness;
                float phi = 2.0 * PI * Xi.x;
                float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
                float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
                vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
                vec3 up = abs(N.z) < 0.999 ? vec3(0,0,1) : vec3(1,0,0);
                vec3 tangent = normalize(cross(up, N));
                vec3 bitangent = cross(N, tangent);
                return normalize(tangent * H.x + bitangent * H.y + N * H.z);
            }

            float GeometrySchlickGGX(float NdotV, float roughness) {
                float a = roughness;
                float k = (a * a) / 2.0;
                return NdotV / (NdotV * (1.0 - k) + k);
            }

            float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
                float NdotV = max(dot(N, V), 0.0);
                float NdotL = max(dot(N, L), 0.0);
                return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
            }

            vec2 IntegrateBRDF(float NdotV, float roughness) {
                vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
                float A = 0.0, B = 0.0;
                vec3 N = vec3(0.0, 0.0, 1.0);
                const uint SAMPLE_COUNT = 1024u;
                for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
                    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
                    vec3 H = ImportanceSampleGGX(Xi, N, roughness);
                    vec3 L = normalize(2.0 * dot(V, H) * H - V);
                    float NdotL = max(L.z, 0.0);
                    float NdotH = max(H.z, 0.0);
                    float VdotH = max(dot(V, H), 0.0);
                    if (NdotL > 0.0) {
                        float G = GeometrySmith(N, V, L, roughness);
                        float G_Vis = (G * VdotH) / (NdotH * NdotV);
                        float Fc = pow(1.0 - VdotH, 5.0);
                        A += (1.0 - Fc) * G_Vis;
                        B += Fc * G_Vis;
                    }
                }
                return vec2(A, B) / float(SAMPLE_COUNT);
            }

            void main() {
                FragColor = IntegrateBRDF(v_TexCoord.x, v_TexCoord.y);
            }
        )";

        s_BRDFShader = CreateRef<Shader>("BRDF_LUT", brdfVert, brdfFrag);

        // Create BRDF LUT texture
        glGenTextures(1, &s_BRDFLUT);
        glBindTexture(GL_TEXTURE_2D, s_BRDFLUT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Render BRDF LUT
        uint32_t fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_BRDFLUT, 0);

        GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (fboStatus != GL_FRAMEBUFFER_COMPLETE) {
            GE_CORE_ERROR("IBL: BRDF LUT framebuffer incomplete (status: 0x{:X})", fboStatus);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &fbo);
            return;
        }

        glViewport(0, 0, 512, 512);
        s_BRDFShader->Bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindVertexArray(s_QuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        s_BRDFShader->Unbind();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);

        GE_CORE_DEBUG("IBL: BRDF LUT generated");
    }

    void IBL::GenerateProceduralCubemap() {
        // Generate a simple gradient sky cubemap procedurally (no HDR file needed)
        const int size = 64;
        glGenTextures(1, &s_EnvCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_EnvCubemap);

        // Sky gradient colors
        glm::vec3 skyTop(0.2f, 0.4f, 0.8f);
        glm::vec3 skyHorizon(0.6f, 0.7f, 0.9f);
        glm::vec3 ground(0.15f, 0.12f, 0.1f);

        for (int face = 0; face < 6; face++) {
            std::vector<float> data(size * size * 3);
            for (int y = 0; y < size; y++) {
                for (int x = 0; x < size; x++) {
                    // Simple vertical gradient based on face
                    float t;
                    if (face == 2) t = 1.0f; // +Y (top)
                    else if (face == 3) t = -1.0f; // -Y (bottom)
                    else t = (float(y) / float(size) - 0.5f) * -2.0f;

                    glm::vec3 color;
                    if (t > 0.0f) {
                        color = glm::mix(skyHorizon, skyTop, t);
                    } else {
                        color = glm::mix(skyHorizon, ground, -t);
                    }

                    int idx = (y * size + x) * 3;
                    data[idx] = color.r;
                    data[idx + 1] = color.g;
                    data[idx + 2] = color.b;
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                         size, size, 0, GL_RGB, GL_FLOAT, data.data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GE_CORE_DEBUG("IBL: Procedural sky cubemap generated ({}x{})", size, size);
    }

    void IBL::GenerateIrradianceMap() {
        // For the procedural sky, generate a simple averaged irradiance cubemap
        const int size = 32;
        glGenTextures(1, &s_IrradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_IrradianceMap);

        // Simple diffuse irradiance: average sky color per face direction
        glm::vec3 skyAvg(0.35f, 0.5f, 0.75f);
        glm::vec3 groundAvg(0.1f, 0.09f, 0.08f);

        glm::vec3 faceColors[6] = {
            skyAvg * 0.6f,                                  // +X
            skyAvg * 0.6f,                                  // -X
            skyAvg,                                         // +Y (top)
            groundAvg,                                      // -Y (bottom)
            skyAvg * 0.6f,                                  // +Z
            skyAvg * 0.6f                                   // -Z
        };

        for (int face = 0; face < 6; face++) {
            std::vector<float> data(size * size * 3);
            for (int i = 0; i < size * size; i++) {
                data[i * 3] = faceColors[face].r;
                data[i * 3 + 1] = faceColors[face].g;
                data[i * 3 + 2] = faceColors[face].b;
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                         size, size, 0, GL_RGB, GL_FLOAT, data.data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GE_CORE_DEBUG("IBL: Irradiance map generated ({}x{})", size, size);
    }

    void IBL::GeneratePrefilterMap() {
        // Generate prefiltered environment map with mip levels for roughness
        const int size = 128;
        const int maxMipLevels = 5;

        glGenTextures(1, &s_PrefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, s_PrefilterMap);

        for (int face = 0; face < 6; face++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB16F,
                         size, size, 0, GL_RGB, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // Fill mip levels with progressively blurred sky colors
        glm::vec3 skyAvg(0.35f, 0.5f, 0.75f);

        for (int mip = 0; mip < maxMipLevels; mip++) {
            int mipSize = size >> mip;
            if (mipSize < 1) mipSize = 1;

            float roughness = float(mip) / float(maxMipLevels - 1);
            // Blur toward average as roughness increases
            glm::vec3 skyTop = glm::mix(glm::vec3(0.2f, 0.4f, 0.8f), skyAvg, roughness);
            glm::vec3 skyHorizon = glm::mix(glm::vec3(0.6f, 0.7f, 0.9f), skyAvg, roughness);

            for (int face = 0; face < 6; face++) {
                std::vector<float> data(mipSize * mipSize * 3);
                for (int y = 0; y < mipSize; y++) {
                    for (int x = 0; x < mipSize; x++) {
                        float t = (face == 2) ? 1.0f : (face == 3) ? -0.5f :
                                  (float(y) / float(mipSize) - 0.5f) * -2.0f;

                        glm::vec3 color = (t > 0.0f) ?
                            glm::mix(skyHorizon, skyTop, t * 0.5f) :
                            glm::mix(skyHorizon, skyAvg * 0.3f, -t);

                        int idx = (y * mipSize + x) * 3;
                        data[idx] = color.r;
                        data[idx + 1] = color.g;
                        data[idx + 2] = color.b;
                    }
                }
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, GL_RGB16F,
                             mipSize, mipSize, 0, GL_RGB, GL_FLOAT, data.data());
            }
        }

        GE_CORE_DEBUG("IBL: Prefilter map generated ({}x{}, {} mips)", size, size, maxMipLevels);
    }

    void IBL::LoadEnvironment(const std::string& hdrPath) {
        // TODO: Load HDR equirectangular image, convert to cubemap,
        // then regenerate irradiance and prefilter maps via GPU convolution.
        // For now, the procedural sky is used.
        GE_CORE_WARN("IBL::LoadEnvironment not yet implemented, using procedural sky");
    }

}
