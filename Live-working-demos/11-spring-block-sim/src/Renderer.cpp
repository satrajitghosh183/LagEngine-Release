#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// Cube vertex data  (pos, normal, uv)
// ─────────────────────────────────────────────────────────────────────────────
static const float CUBE_VERTS[] = {
    // positions          normals            UVs
    // -Z face
    -1,-1,-1,  0, 0,-1,  0,0,
     1,-1,-1,  0, 0,-1,  1,0,
     1, 1,-1,  0, 0,-1,  1,1,
    -1, 1,-1,  0, 0,-1,  0,1,
    // +Z
    -1,-1, 1,  0, 0, 1,  0,0,
     1,-1, 1,  0, 0, 1,  1,0,
     1, 1, 1,  0, 0, 1,  1,1,
    -1, 1, 1,  0, 0, 1,  0,1,
    // -X
    -1,-1,-1, -1, 0, 0,  0,0,
    -1, 1,-1, -1, 0, 0,  1,0,
    -1, 1, 1, -1, 0, 0,  1,1,
    -1,-1, 1, -1, 0, 0,  0,1,
    // +X
     1,-1,-1,  1, 0, 0,  0,0,
     1, 1,-1,  1, 0, 0,  1,0,
     1, 1, 1,  1, 0, 0,  1,1,
     1,-1, 1,  1, 0, 0,  0,1,
    // -Y
    -1,-1,-1,  0,-1, 0,  0,0,
     1,-1,-1,  0,-1, 0,  1,0,
     1,-1, 1,  0,-1, 0,  1,1,
    -1,-1, 1,  0,-1, 0,  0,1,
    // +Y
    -1, 1,-1,  0, 1, 0,  0,0,
     1, 1,-1,  0, 1, 0,  1,0,
     1, 1, 1,  0, 1, 0,  1,1,
    -1, 1, 1,  0, 1, 0,  0,1,
};
static const GLuint CUBE_IDX[] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23,
};

// ─────────────────────────────────────────────────────────────────────────────
// Ground quad (large flat plane)
// ─────────────────────────────────────────────────────────────────────────────
static const float GROUND_VERTS[] = {
    // pos           normal     uv
    -50, 0,-50,  0,1,0,   0,  0,
     50, 0,-50,  0,1,0,  50,  0,
     50, 0, 50,  0,1,0,  50, 50,
    -50, 0, 50,  0,1,0,   0, 50,
};
static const GLuint GROUND_IDX[] = { 0,1,2, 0,2,3 };

// Full-screen quad
static const float QUAD_VERTS[] = {
    -1.0f,  1.0f, 0.0f, 1.0f,
    -1.0f, -1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 1.0f, 0.0f,
     1.0f,  1.0f, 1.0f, 1.0f,
};

// ─────────────────────────────────────────────────────────────────────────────
static GLuint makeBuffer(GLenum target, const void* data, GLsizeiptr size, GLenum usage = GL_STATIC_DRAW) {
    GLuint id; glGenBuffers(1, &id);
    glBindBuffer(target, id);
    glBufferData(target, size, data, usage);
    return id;
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::init(int width, int height) {
    m_w = width; m_h = height;

    m_pbrShader         = Shader("shaders/pbr.vert",          "shaders/pbr.frag");
    m_groundShader      = Shader("shaders/ground.vert",       "shaders/ground.frag");
    m_shadowShader      = Shader("shaders/shadow.vert",       "shaders/shadow.frag");
    m_bloomExtractShader= Shader("shaders/screen.vert",       "shaders/bloom_extract.frag");
    m_blurShader        = Shader("shaders/screen.vert",       "shaders/blur.frag");
    m_tonemapShader     = Shader("shaders/screen.vert",       "shaders/tonemap.frag");

    initCubeMesh();
    initGroundMesh();
    initQuad();
    initShadowMap();
    initHDRBuffer();
    initBloomBuffers();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void Renderer::resize(int width, int height) {
    m_w = width; m_h = height;
    initHDRBuffer();
    initBloomBuffers();
}

void Renderer::shutdown() {
    // OpenGL objects cleaned up on exit; spring meshes auto-destruct
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::initShadowMap() {
    glGenFramebuffers(1, &m_shadowFBO);
    glGenTextures(1, &m_shadowTex);
    glBindTexture(GL_TEXTURE_2D, m_shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_RES, SHADOW_RES, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = {1,1,1,1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowTex, 0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initHDRBuffer() {
    if (m_hdrFBO)  { glDeleteFramebuffers(1, &m_hdrFBO); }
    if (m_hdrColor){ glDeleteTextures(1, &m_hdrColor); }
    if (m_hdrDepth){ glDeleteRenderbuffers(1, &m_hdrDepth); }

    glGenFramebuffers(1, &m_hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

    glGenTextures(1, &m_hdrColor);
    glBindTexture(GL_TEXTURE_2D, m_hdrColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_w, m_h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrColor, 0);

    glGenRenderbuffers(1, &m_hdrDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, m_hdrDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_w, m_h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_hdrDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[Renderer] HDR FBO incomplete\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initBloomBuffers() {
    for (int i = 0; i < 2; ++i) {
        if (m_bloomFBO[i]) glDeleteFramebuffers(1, &m_bloomFBO[i]);
        if (m_bloomTex[i]) glDeleteTextures(1, &m_bloomTex[i]);

        glGenFramebuffers(1, &m_bloomFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFBO[i]);

        glGenTextures(1, &m_bloomTex[i]);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_w / 2, m_h / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomTex[i], 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void Renderer::initCubeMesh() {
    glGenVertexArrays(1, &m_cubeVAO);
    glBindVertexArray(m_cubeVAO);
    m_cubeVBO = makeBuffer(GL_ARRAY_BUFFER, CUBE_VERTS, sizeof(CUBE_VERTS));
    m_cubeEBO = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, CUBE_IDX, sizeof(CUBE_IDX));
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*4,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*4,(void*)(3*4));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*4,(void*)(6*4));
    glBindVertexArray(0);
}

void Renderer::initGroundMesh() {
    glGenVertexArrays(1, &m_groundVAO);
    glBindVertexArray(m_groundVAO);
    m_groundVBO = makeBuffer(GL_ARRAY_BUFFER, GROUND_VERTS, sizeof(GROUND_VERTS));
    m_groundEBO = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GROUND_IDX, sizeof(GROUND_IDX));
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*4,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*4,(void*)(3*4));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*4,(void*)(6*4));
    glBindVertexArray(0);
}

void Renderer::initQuad() {
    glGenVertexArrays(1, &m_quadVAO);
    glBindVertexArray(m_quadVAO);
    m_quadVBO = makeBuffer(GL_ARRAY_BUFFER, QUAD_VERTS, sizeof(QUAD_VERTS));
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*4,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*4,(void*)(2*4));
    glBindVertexArray(0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Light-space matrix (orthographic projection for directional shadow)
// ─────────────────────────────────────────────────────────────────────────────
static glm::mat4 computeLightSpace(const glm::vec3& dir) {
    glm::vec3 lightPos = -glm::normalize(dir) * 20.0f;
    glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0), glm::vec3(0,1,0));
    glm::mat4 lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 60.0f);
    return lightProj * lightView;
}

// ─────────────────────────────────────────────────────────────────────────────
// Shadow pass
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::shadowPass(const PhysicsWorld& world) {
    m_lightSpaceMatrix = computeLightSpace(m_lightDir);

    glViewport(0, 0, SHADOW_RES, SHADOW_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCullFace(GL_FRONT);   // Peter-Pan bias

    m_shadowShader.use();
    m_shadowShader.setMat4("lightSpaceMatrix", m_lightSpaceMatrix);

    // Draw blocks
    for (const auto& b : world.blocks) {
        glm::mat4 model = glm::translate(glm::mat4(1), b.position);
        model = glm::scale(model, b.halfExtents);
        m_shadowShader.setMat4("model", model);
        glBindVertexArray(m_cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }
    // Draw spring tubes
    for (int si = 0; si < (int)world.springs.size(); ++si) {
        m_shadowShader.setMat4("model", glm::mat4(1));
        if (si < (int)m_springMeshes.size() && m_springMeshes[si].ready())
            m_springMeshes[si].draw();
    }

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bind common PBR uniforms
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::bindPBRCommon(Shader& sh, const Camera& cam) {
    float aspect = (float)m_w / (float)m_h;
    sh.setMat4("view",             cam.viewMatrix(aspect));
    sh.setMat4("projection",       cam.projMatrix(aspect));
    sh.setMat4("lightSpaceMatrix", m_lightSpaceMatrix);
    sh.setVec3("camPos",           cam.position());
    sh.setVec3("dirLightDir",      m_lightDir);
    sh.setVec3("dirLightColor",    m_lightColor);
    sh.setInt ("numPointLights",   0);
    sh.setInt ("shadowMap",        0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_shadowTex);
}

// ─────────────────────────────────────────────────────────────────────────────
// Colour pass
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::colorPass(const PhysicsWorld& world, const Camera& cam,
                         int selBlock, int selSpring) {
    glViewport(0, 0, m_w, m_h);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
    glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (cfg.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // ── Ground ──────────────────────────────────────────────────────────────
    m_groundShader.use();
    bindPBRCommon(m_groundShader, cam);
    glm::mat4 groundModel = glm::mat4(1);
    m_groundShader.setMat4("model",            groundModel);
    m_groundShader.setMat4("lightSpaceMatrix", m_lightSpaceMatrix);
    glBindVertexArray(m_groundVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // ── Blocks ───────────────────────────────────────────────────────────────
    m_pbrShader.use();
    bindPBRCommon(m_pbrShader, cam);
    for (int bi = 0; bi < (int)world.blocks.size(); ++bi) {
        const Block& b = world.blocks[bi];
        glm::mat4 model = glm::translate(glm::mat4(1), b.position);
        model = glm::scale(model, b.halfExtents);
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

        m_pbrShader.setMat4("model",             model);
        m_pbrShader.setMat3("normalMatrix",      normalMat);
        m_pbrShader.setVec3("albedo",            b.albedo);
        m_pbrShader.setFloat("metallic",         b.metallic);
        m_pbrShader.setFloat("roughness",        b.roughness);
        m_pbrShader.setFloat("ao",               b.ao);
        m_pbrShader.setFloat("selectedBrightness", (bi == selBlock) ? 1.0f : 0.0f);

        glBindVertexArray(m_cubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }

    // ── Springs (tube meshes) ────────────────────────────────────────────────
    resizeSpringMeshes((int)world.springs.size());
    for (int si = 0; si < (int)world.springs.size(); ++si) {
        const Spring& s = world.springs[si];
        glm::vec3 pA = world.springEndA(si);
        glm::vec3 pB = world.springEndB(si);
        m_springMeshes[si].rebuild(pA, pB);

        glm::mat4 identity = glm::mat4(1);
        glm::mat3 normalMat = glm::mat3(1);
        m_pbrShader.setMat4("model",             identity);
        m_pbrShader.setMat3("normalMatrix",      normalMat);
        m_pbrShader.setVec3("albedo",            s.albedo);
        m_pbrShader.setFloat("metallic",         s.metallic);
        m_pbrShader.setFloat("roughness",        s.roughness);
        m_pbrShader.setFloat("ao",               1.0f);
        m_pbrShader.setFloat("selectedBrightness", (si == selSpring) ? 1.0f : 0.0f);

        if (m_springMeshes[si].ready())
            m_springMeshes[si].draw();
    }

    if (cfg.wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bloom pass
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::bloomPass() {
    // Extract bright regions (half res)
    glViewport(0, 0, m_w / 2, m_h / 2);
    glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFBO[0]);
    glClear(GL_COLOR_BUFFER_BIT);

    m_bloomExtractShader.use();
    m_bloomExtractShader.setInt  ("hdrBuffer", 0);
    m_bloomExtractShader.setFloat("threshold", cfg.bloomThreshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrColor);
    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Ping-pong blur (5 iterations)
    m_blurShader.use();
    m_blurShader.setInt("image", 0);
    bool horizontal = true;
    for (int i = 0; i < 10; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFBO[horizontal ? 1 : 0]);
        m_blurShader.setBool("horizontal", horizontal);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[horizontal ? 0 : 1]);
        glBindVertexArray(m_quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        horizontal = !horizontal;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tonemap pass → default framebuffer
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::tonemapPass() {
    glViewport(0, 0, m_w, m_h);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_tonemapShader.use();
    m_tonemapShader.setInt  ("hdrBuffer",    0);
    m_tonemapShader.setInt  ("bloomBuffer",  1);
    m_tonemapShader.setFloat("exposure",     cfg.exposure);
    m_tonemapShader.setFloat("bloomStrength",cfg.bloomEnabled ? cfg.bloomStrength : 0.0f);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_hdrColor);
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_bloomTex[0]);

    glBindVertexArray(m_quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glEnable(GL_DEPTH_TEST);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main render entry
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render(const PhysicsWorld& world, const Camera& cam,
                      int selectedBlock, int selectedSpring) {
    shadowPass(world);
    colorPass(world, cam, selectedBlock, selectedSpring);
    if (cfg.bloomEnabled) bloomPass();
    tonemapPass();
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::resizeSpringMeshes(int n) {
    while ((int)m_springMeshes.size() < n) {
        m_springMeshes.emplace_back();
        m_springMeshes.back().init(10, 14, 0.12f, 0.035f);
    }
    // Extra meshes are kept (no harm)
}
