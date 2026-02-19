#pragma once

#include "RenderGraph.h"
#include "ShadowMap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include <memory>

class Shader;
class Mesh;
class Framebuffer;

class DemoScene {
public:
    DemoScene(int width, int height);
    ~DemoScene();
    
    void initialize();
    void resize(int width, int height);
    void update(float deltaTime);
    
    std::shared_ptr<RenderGraph> getRenderGraph() { return m_renderGraph; }
    
    // Render passes (called by tasks)
    void renderShadowPass();
    void renderGBufferPass();
    void renderLightingPass();
    void renderPostFXPass();
    
    // CPU tasks (for work stealing)
    void performFrustumCulling();
    void updateMeshes();
    void updatePhysics();
    
    Camera& getCamera() { return m_camera; }

private:
    int m_width;
    int m_height;
    
    std::shared_ptr<RenderGraph> m_renderGraph;
    std::shared_ptr<ShadowMap> m_shadowMap;
    std::shared_ptr<GBuffer> m_gbuffer;
    std::shared_ptr<SSAO> m_ssao;
    std::shared_ptr<ParticleSystem> m_particles;
    
    std::shared_ptr<Shader> m_shadowShader;
    std::shared_ptr<Shader> m_gbufferShader;
    std::shared_ptr<Shader> m_lightingShader;
    std::shared_ptr<Shader> m_postFXShader;
    
    std::shared_ptr<Mesh> m_cubeMesh;
    std::shared_ptr<Mesh> m_sphereMesh;
    
    std::shared_ptr<Framebuffer> m_outputFBO;
    
    Camera m_camera;
    
    float m_rotationAngle;
    float m_lightRotation;
    float m_deltaTime;
    
    void createRenderGraph();
    void setupMeshes();
    void setupShaders();
};

