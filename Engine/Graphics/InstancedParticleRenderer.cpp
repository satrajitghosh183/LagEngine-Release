#include "InstancedParticleRenderer.hpp"
#include "../Core/Logger.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    Ref<Shader> InstancedParticleRenderer::s_ParticleShader = nullptr;
    uint32_t InstancedParticleRenderer::s_QuadVAO = 0;
    uint32_t InstancedParticleRenderer::s_QuadVBO = 0;
    uint32_t InstancedParticleRenderer::s_InstanceVBO = 0;
    uint32_t InstancedParticleRenderer::s_MaxParticles = 10000;
    Ref<Texture2D> InstancedParticleRenderer::s_DefaultTexture = nullptr;
    
    glm::mat4 InstancedParticleRenderer::s_ViewProjection = glm::mat4(1.0f);
    glm::vec3 InstancedParticleRenderer::s_CameraRight = glm::vec3(1, 0, 0);
    glm::vec3 InstancedParticleRenderer::s_CameraUp = glm::vec3(0, 1, 0);
    
    InstancedParticleRenderer::BlendMode InstancedParticleRenderer::s_BlendMode = BlendMode::Alpha;
    bool InstancedParticleRenderer::s_SoftParticles = false;
    float InstancedParticleRenderer::s_SoftParticleFadeDistance = 0.5f;
    int InstancedParticleRenderer::s_AtlasColumns = 1;
    int InstancedParticleRenderer::s_AtlasRows = 1;
    
    InstancedParticleRenderer::Statistics InstancedParticleRenderer::s_Stats;

    void InstancedParticleRenderer::Init() {
        CreateResources();
        GE_CORE_INFO("InstancedParticleRenderer initialized (max {} particles)", s_MaxParticles);
    }

    void InstancedParticleRenderer::Shutdown() {
        if (s_QuadVAO) {
            glDeleteVertexArrays(1, &s_QuadVAO);
            s_QuadVAO = 0;
        }
        if (s_QuadVBO) {
            glDeleteBuffers(1, &s_QuadVBO);
            s_QuadVBO = 0;
        }
        if (s_InstanceVBO) {
            glDeleteBuffers(1, &s_InstanceVBO);
            s_InstanceVBO = 0;
        }
        
        s_ParticleShader.reset();
        s_DefaultTexture.reset();
    }

    void InstancedParticleRenderer::CreateResources() {
        // Create particle shader
        const char* vertexSrc = R"(
            #version 420 core
            
            // Per-vertex attributes (quad corners)
            layout(location = 0) in vec2 a_Position;
            layout(location = 1) in vec2 a_TexCoord;
            
            // Per-instance attributes
            layout(location = 2) in vec3 a_InstancePosition;
            layout(location = 3) in float a_InstanceSize;
            layout(location = 4) in vec4 a_InstanceColor;
            layout(location = 5) in float a_InstanceRotation;
            layout(location = 6) in float a_InstanceLife;
            
            uniform mat4 u_ViewProjection;
            uniform vec3 u_CameraRight;
            uniform vec3 u_CameraUp;
            uniform ivec2 u_AtlasSize;
            
            out vec2 v_TexCoord;
            out vec4 v_Color;
            out float v_Life;
            
            void main() {
                // Calculate billboard position
                float cosR = cos(a_InstanceRotation);
                float sinR = sin(a_InstanceRotation);
                
                vec2 rotatedPos;
                rotatedPos.x = a_Position.x * cosR - a_Position.y * sinR;
                rotatedPos.y = a_Position.x * sinR + a_Position.y * cosR;
                
                vec3 worldPos = a_InstancePosition 
                    + u_CameraRight * rotatedPos.x * a_InstanceSize
                    + u_CameraUp * rotatedPos.y * a_InstanceSize;
                
                gl_Position = u_ViewProjection * vec4(worldPos, 1.0);
                
                // Calculate texture coordinates for atlas
                vec2 atlasCell = vec2(0.0);  // Could be passed per-instance
                vec2 cellSize = vec2(1.0 / float(u_AtlasSize.x), 1.0 / float(u_AtlasSize.y));
                v_TexCoord = atlasCell * cellSize + a_TexCoord * cellSize;
                
                v_Color = a_InstanceColor;
                v_Life = a_InstanceLife;
            }
        )";
        
        const char* fragmentSrc = R"(
            #version 420 core
            
            in vec2 v_TexCoord;
            in vec4 v_Color;
            in float v_Life;
            
            uniform sampler2D u_Texture;
            uniform bool u_SoftParticles;
            uniform float u_SoftFadeDistance;
            uniform sampler2D u_DepthTexture;
            uniform vec2 u_ScreenSize;
            
            layout(location = 0) out vec4 FragColor;
            
            void main() {
                vec4 texColor = texture(u_Texture, v_TexCoord);
                FragColor = texColor * v_Color;
                
                // Soft particles (optional depth fade)
                if (u_SoftParticles) {
                    vec2 screenUV = gl_FragCoord.xy / u_ScreenSize;
                    float sceneDepth = texture(u_DepthTexture, screenUV).r;
                    float particleDepth = gl_FragCoord.z;
                    float depthDiff = sceneDepth - particleDepth;
                    float fade = smoothstep(0.0, u_SoftFadeDistance, depthDiff);
                    FragColor.a *= fade;
                }
                
                // Discard fully transparent pixels
                if (FragColor.a < 0.01)
                    discard;
            }
        )";
        
        s_ParticleShader = CreateRef<Shader>("InstancedParticle", vertexSrc, fragmentSrc);
        
        // Create quad geometry
        float quadVertices[] = {
            // Position     // TexCoord
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f, -0.5f,   1.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f, -0.5f,   0.0f, 0.0f,
             0.5f,  0.5f,   1.0f, 1.0f,
            -0.5f,  0.5f,   0.0f, 1.0f
        };
        
        glGenVertexArrays(1, &s_QuadVAO);
        glGenBuffers(1, &s_QuadVBO);
        glGenBuffers(1, &s_InstanceVBO);
        
        glBindVertexArray(s_QuadVAO);
        
        // Quad vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        
        // TexCoord
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        // Instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_InstanceVBO);
        glBufferData(GL_ARRAY_BUFFER, s_MaxParticles * sizeof(ParticleInstance), nullptr, GL_DYNAMIC_DRAW);
        
        // Instance position
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, Position));
        glVertexAttribDivisor(2, 1);
        
        // Instance size
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, Size));
        glVertexAttribDivisor(3, 1);
        
        // Instance color
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, Color));
        glVertexAttribDivisor(4, 1);
        
        // Instance rotation
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, Rotation));
        glVertexAttribDivisor(5, 1);
        
        // Instance life
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleInstance), (void*)offsetof(ParticleInstance, LifeRemaining));
        glVertexAttribDivisor(6, 1);
        
        glBindVertexArray(0);
        
        // Create 1x1 white default texture
        uint32_t whitePixel = 0xFFFFFFFF;
        s_DefaultTexture = CreateRef<Texture2D>(1, 1);
        s_DefaultTexture->SetData(&whitePixel, sizeof(uint32_t));
    }

    void InstancedParticleRenderer::Begin(const Camera3D& camera) {
        s_ViewProjection = camera.GetViewProjectionMatrix();
        
        // Extract camera right and up vectors from view matrix
        glm::mat4 view = camera.GetViewMatrix();
        s_CameraRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
        s_CameraUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
        
        ResetStats();
    }

    void InstancedParticleRenderer::End() {
        // Nothing to flush - particles are drawn immediately
    }

    void InstancedParticleRenderer::DrawParticles(
        const std::vector<ParticleInstance>& particles,
        const Ref<Texture2D>& texture
    ) {
        if (particles.empty()) return;
        
        size_t particleCount = std::min(particles.size(), static_cast<size_t>(s_MaxParticles));
        
        // Update instance buffer
        glBindBuffer(GL_ARRAY_BUFFER, s_InstanceVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, particleCount * sizeof(ParticleInstance), particles.data());
        
        // Setup blending
        glEnable(GL_BLEND);
        switch (s_BlendMode) {
            case BlendMode::Additive:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                break;
            case BlendMode::Alpha:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                break;
            case BlendMode::Multiply:
                glBlendFunc(GL_DST_COLOR, GL_ZERO);
                break;
        }
        
        // Disable depth writing (but keep depth testing)
        glDepthMask(GL_FALSE);
        
        // Bind shader and set uniforms
        s_ParticleShader->Bind();
        s_ParticleShader->SetUniformMat4("u_ViewProjection", s_ViewProjection);
        s_ParticleShader->SetUniformVec3("u_CameraRight", s_CameraRight);
        s_ParticleShader->SetUniformVec3("u_CameraUp", s_CameraUp);
        s_ParticleShader->SetUniformInt("u_AtlasSize", s_AtlasColumns);
        s_ParticleShader->SetUniformBool("u_SoftParticles", s_SoftParticles);
        s_ParticleShader->SetUniformFloat("u_SoftFadeDistance", s_SoftParticleFadeDistance);
        
        // Bind texture
        if (texture) {
            texture->Bind(0);
        } else {
            s_DefaultTexture->Bind(0);
        }
        s_ParticleShader->SetUniformInt("u_Texture", 0);
        
        // Draw instanced
        glBindVertexArray(s_QuadVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(particleCount));
        glBindVertexArray(0);
        
        // Restore state
        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        s_ParticleShader->Unbind();
        
        // Update stats
        s_Stats.DrawCalls++;
        s_Stats.ParticlesRendered += static_cast<uint32_t>(particleCount);
        s_Stats.BatchCount++;
    }

    void InstancedParticleRenderer::SetBlendMode(BlendMode mode) {
        s_BlendMode = mode;
    }

    void InstancedParticleRenderer::SetSoftParticles(bool enabled, float fadeDistance) {
        s_SoftParticles = enabled;
        s_SoftParticleFadeDistance = fadeDistance;
    }

    void InstancedParticleRenderer::SetAtlasSize(int columns, int rows) {
        s_AtlasColumns = columns;
        s_AtlasRows = rows;
    }

}
