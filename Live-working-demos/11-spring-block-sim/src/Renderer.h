#pragma once
#include <vector>
#include <VulkanBase.hpp>
#include <glm/glm.hpp>
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

    void init(vkdemo::VulkanBase* vkBase);
    void resize(int width, int height);
    void render(VkCommandBuffer cmd, const PhysicsWorld& world, const Camera& cam,
                int selectedBlock, int selectedSpring);
    void shutdown();

private:
    vkdemo::VulkanBase* m_vkBase = nullptr;
    int m_w = 0, m_h = 0;

    // ── Push constant structures ─────────────────────────────────────────────
    // PBR push constants: MVP + model + material params
    struct PBRPushConstants {
        glm::mat4 mvp;          // 0
        glm::mat4 model;        // 64
        glm::vec4 albedo;       // 128  (xyz = albedo, w = ao)
        glm::vec4 params;       // 144  (x = metallic, y = roughness, z = selectedBrightness, w = unused)
        glm::vec4 camPos;       // 160  (xyz = camera position)
        glm::vec4 lightDir;     // 176  (xyz = light direction)
        glm::vec4 lightColor;   // 192  (xyz = light color)
    };  // 208 bytes total

    // Ground push constants (same layout, reuse PBR struct)

    // ── Pipelines ────────────────────────────────────────────────────────────
    VkPipelineLayout m_pbrPipelineLayout = VK_NULL_HANDLE;
    VkPipeline       m_pbrPipeline       = VK_NULL_HANDLE;
    VkPipeline       m_pbrWireframePipeline = VK_NULL_HANDLE;
    VkPipeline       m_groundPipeline    = VK_NULL_HANDLE;

    // ── Geometry buffers ─────────────────────────────────────────────────────
    vkdemo::GPUBuffer m_cubeVertexBuffer{};
    vkdemo::GPUBuffer m_cubeIndexBuffer{};
    vkdemo::GPUBuffer m_groundVertexBuffer{};
    vkdemo::GPUBuffer m_groundIndexBuffer{};

    // Spring meshes (one per spring, updated each frame)
    std::vector<SpringMesh> m_springMeshes;

    // ── Light ────────────────────────────────────────────────────────────────
    glm::vec3 m_lightDir   = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.5f));
    glm::vec3 m_lightColor = {6.0f, 5.8f, 5.5f};

    // ── Private helpers ──────────────────────────────────────────────────────
    void createPipelines();
    void initCubeBuffers();
    void initGroundBuffers();

    void drawBlocks(VkCommandBuffer cmd, const PhysicsWorld& world,
                    const Camera& cam, int selBlock);
    void drawSprings(VkCommandBuffer cmd, const PhysicsWorld& world,
                     const Camera& cam, int selSpring);
    void drawGround(VkCommandBuffer cmd, const Camera& cam);

    void resizeSpringMeshes(int n);

    // Shader compilation
    VkShaderModule compileGLSL(const std::string& source, const std::string& name,
                               bool isVertex);
};
