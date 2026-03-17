#pragma once
#include <glad/glad.h>
#include <vector>
#include "Shader.h"
#include "SpringMesh.h"
#include "Physics.h"
#include "Camera.h"

struct RenderConfig {
    float exposure      = 1.1f;
    float bloomStrength = 0.18f;
    float bloomThreshold= 1.0f;
    bool  bloomEnabled  = true;
    bool  wireframe     = false;
};

class Renderer {
public:
    RenderConfig cfg;

    void init(int width, int height);
    void resize(int width, int height);
    void render(const PhysicsWorld& world, const Camera& cam,
                int selectedBlock, int selectedSpring);
    void shutdown();

private:
    int m_w = 0, m_h = 0;

    // ── Shaders ───────────────────────────────────────────────────────────────
    Shader m_pbrShader;
    Shader m_groundShader;
    Shader m_shadowShader;
    Shader m_bloomExtractShader;
    Shader m_blurShader;
    Shader m_tonemapShader;

    // ── Shadow map ────────────────────────────────────────────────────────────
    GLuint m_shadowFBO  = 0;
    GLuint m_shadowTex  = 0;
    static constexpr int SHADOW_RES = 2048;

    // ── HDR colour buffer ─────────────────────────────────────────────────────
    GLuint m_hdrFBO     = 0;
    GLuint m_hdrColor   = 0;   // float texture
    GLuint m_hdrDepth   = 0;

    // ── Bloom ping-pong ───────────────────────────────────────────────────────
    GLuint m_bloomFBO[2]   = {};
    GLuint m_bloomTex[2]   = {};

    // ── Geometry ──────────────────────────────────────────────────────────────
    GLuint m_cubeVAO = 0, m_cubeVBO = 0, m_cubeEBO = 0;
    GLuint m_groundVAO = 0, m_groundVBO = 0, m_groundEBO = 0;
    GLuint m_quadVAO  = 0, m_quadVBO  = 0;

    // Spring meshes (one per spring, updated each frame)
    std::vector<SpringMesh> m_springMeshes;

    // ── Light ─────────────────────────────────────────────────────────────────
    glm::vec3 m_lightDir  = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.5f));
    glm::vec3 m_lightColor = {6.0f, 5.8f, 5.5f};

    glm::mat4 m_lightSpaceMatrix;

    // ── Private helpers ───────────────────────────────────────────────────────
    void initShadowMap();
    void initHDRBuffer();
    void initBloomBuffers();
    void initCubeMesh();
    void initGroundMesh();
    void initQuad();

    void shadowPass(const PhysicsWorld& world);
    void colorPass (const PhysicsWorld& world, const Camera& cam,
                    int selBlock, int selSpring);
    void bloomPass ();
    void tonemapPass();

    void drawBlock (const Block& b, bool selected);
    void resizeSpringMeshes(int n);

    // Set PBR common uniforms
    void bindPBRCommon(Shader& sh, const Camera& cam);
};
