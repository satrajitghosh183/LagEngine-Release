#pragma once

#include "RenderGraph.h"
#include "ShadowMap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace vkdemo { class VulkanBase; }
class Shader;
class Mesh;
class Framebuffer;

class DemoScene {
public:
    DemoScene(vkdemo::VulkanBase* vkBase, int width, int height);
    ~DemoScene();

    void initialize();
    void resize(int width, int height);
    void update(float deltaTime);

    // Record Vulkan commands for each pass
    void recordShadowPass(VkCommandBuffer cmd);
    void recordGBufferPass(VkCommandBuffer cmd);
    void recordLightingPass(VkCommandBuffer cmd);
    void recordPostFXPass(VkCommandBuffer cmd);

    // CPU tasks (unchanged)
    void performFrustumCulling();
    void updateMeshes();
    void updatePhysics();

    // Particle update (CUDA or CPU)
    void updateParticles(float deltaTime, void* cudaStream);

    std::shared_ptr<RenderGraph> getRenderGraph() { return m_renderGraph; }
    Camera& getCamera() { return m_camera; }

    // Accessors for the scheduler to record commands
    ShadowMap*      getShadowMap()     { return m_shadowMap.get(); }
    GBuffer*        getGBuffer()       { return m_gbuffer.get(); }
    Framebuffer*    getOutputFBO()     { return m_outputFBO.get(); }
    ParticleSystem* getParticles()     { return m_particles.get(); }
    SSAO*           getSSAO()          { return m_ssao.get(); }

    // Pipeline objects for each pass
    VkPipeline       getGBufferPipeline()  const { return m_gbufferPipeline; }
    VkPipelineLayout getGBufferLayout()    const { return m_gbufferLayout; }
    VkPipeline       getShadowPipeline()   const { return m_shadowPipeline; }
    VkPipelineLayout getShadowLayout()     const { return m_shadowLayout; }
    VkPipeline       getLightingPipeline() const { return m_lightingPipeline; }
    VkPipelineLayout getLightingLayout()   const { return m_lightingLayout; }
    VkPipeline       getPostFXPipeline()   const { return m_postfxPipeline; }
    VkPipelineLayout getPostFXLayout()     const { return m_postfxLayout; }
    VkPipeline       getParticlePipeline() const { return m_particlePipeline; }
    VkPipelineLayout getParticleLayout()   const { return m_particleLayout; }

    float getDeltaTime() const { return m_deltaTime; }

private:
    vkdemo::VulkanBase* m_vkBase;
    int m_width;
    int m_height;

    std::shared_ptr<RenderGraph> m_renderGraph;
    std::shared_ptr<ShadowMap>    m_shadowMap;
    std::shared_ptr<GBuffer>      m_gbuffer;
    std::shared_ptr<SSAO>         m_ssao;
    std::shared_ptr<ParticleSystem> m_particles;
    std::shared_ptr<Framebuffer>  m_outputFBO;

    std::shared_ptr<Shader> m_shadowShader;
    std::shared_ptr<Shader> m_gbufferShader;
    std::shared_ptr<Shader> m_lightingShader;
    std::shared_ptr<Shader> m_postFXShader;
    std::shared_ptr<Shader> m_particleShader;

    std::shared_ptr<Mesh> m_cubeMesh;
    std::shared_ptr<Mesh> m_sphereMesh;
    std::shared_ptr<Mesh> m_quadMesh;

    Camera m_camera;

    float m_rotationAngle = 0.0f;
    float m_lightRotation = 0.0f;
    float m_deltaTime     = 0.0f;

    // Vulkan pipelines
    VkPipeline       m_gbufferPipeline   = VK_NULL_HANDLE;
    VkPipelineLayout m_gbufferLayout     = VK_NULL_HANDLE;
    VkPipeline       m_shadowPipeline    = VK_NULL_HANDLE;
    VkPipelineLayout m_shadowLayout      = VK_NULL_HANDLE;
    VkPipeline       m_lightingPipeline  = VK_NULL_HANDLE;
    VkPipelineLayout m_lightingLayout    = VK_NULL_HANDLE;
    VkPipeline       m_postfxPipeline    = VK_NULL_HANDLE;
    VkPipelineLayout m_postfxLayout      = VK_NULL_HANDLE;
    VkPipeline       m_particlePipeline  = VK_NULL_HANDLE;
    VkPipelineLayout m_particleLayout    = VK_NULL_HANDLE;

    // Descriptor sets for lighting/postfx (G-buffer sampling)
    VkDescriptorPool      m_descriptorPool   = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_lightingDSLayout = VK_NULL_HANDLE;
    VkDescriptorSet       m_lightingDS       = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_postfxDSLayout   = VK_NULL_HANDLE;
    VkDescriptorSet       m_postfxDS         = VK_NULL_HANDLE;

    void createRenderGraph();
    void setupMeshes();
    void setupShaders();
    void createPipelines();
    void createDescriptors();
    void updateDescriptors();
    void cleanupPipelines();
};
