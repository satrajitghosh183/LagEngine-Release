//
// Created by barth on 19/09/2022.
//
// Vulkan port: Scene::render() records Vulkan draw commands.
//

#ifndef FEATHERGL_SCENE_H
#define FEATHERGL_SCENE_H

#include "../meshes/Mesh.h"
#include "cameras/Camera.h"
#include "lights/PointLight.h"
#include "postprocessing.h"
#include "shadows/ShadowRenderer.h"
#include "DirectionalLight.h"
#include <VulkanBase.hpp>

class Scene {
public:
    explicit Scene(std::shared_ptr<Engine> engine);

    ~Scene();

    void addMesh(std::shared_ptr<Mesh> mesh);

    void removeMesh(std::shared_ptr<Mesh> mesh);

    void addPointLight(std::shared_ptr<PointLight> light) {
        _pointLights.push_back(light);
    }

    void addPostProcess(std::shared_ptr<PostProcessing> postProcess) {
        _postProcesses.push_back(postProcess);
        // PostProcessing is a no-op in Vulkan port
    }

    void addShadowRenderer(std::shared_ptr<ShadowRenderer> shadowRenderer) {
        _shadowRenderers.push_back(shadowRenderer);
    }

    void setActiveCamera(std::shared_ptr<Camera> camera);

    std::shared_ptr<Camera> activeCamera() { return _activeCamera; }

    std::vector<std::shared_ptr<PointLight>> *pointLights() {
        return &_pointLights;
    }

    std::vector<std::shared_ptr<DirectionalLight>> *directionalLights() {
        return &_directionalLights;
    }

    void render();

    void compileShaders() {
        // No-op in Vulkan port
    }

    std::vector<std::shared_ptr<Mesh>> meshes() {
        return _meshes;
    }

    std::pair<std::shared_ptr<Mesh>, PickResult> pickWithRay(glm::vec3 origin, glm::vec3 direction) {
        std::vector<std::shared_ptr<Mesh>> meshes;
        for (const auto& mesh: _meshes) {
            if(!mesh->isEnabled()) continue;
            if(!mesh->isPickingEnabled()) continue;
            if(!mesh->aabb()->intersectsRay(origin, direction)) continue;
            meshes.push_back(mesh);
        }

        // sort meshes by distance to the ray origin
        std::sort(meshes.begin(), meshes.end(), [origin](const std::shared_ptr<Mesh>& a, const std::shared_ptr<Mesh>& b) {
            return glm::distance(a->aabb()->center(), origin) < glm::distance(b->aabb()->center(), origin);
        });

        for (const auto& mesh: meshes) {
            PickResult r = mesh->pickWithRay(origin, direction);
            if(r.hasHit) {
                return {mesh, r};
            }
        }

        return {};
    }

    Observable<> onBeforeRenderObservable{};
    Observable<> onAfterRenderObservable{};

    Observable<> onRenderGuiObservable{};

    void addDirectionalLight(std::shared_ptr<DirectionalLight> pLight);

private:
    void initVulkanPipeline();
    void cleanupVulkanPipeline();

    std::shared_ptr<Engine> _engine;

    std::vector<std::shared_ptr<PointLight>> _pointLights{};
    std::vector<std::shared_ptr<DirectionalLight>> _directionalLights{};

    std::vector<std::shared_ptr<Mesh>> _meshes{};

    std::vector<std::shared_ptr<ShadowRenderer>> _shadowRenderers{};
    std::vector<std::shared_ptr<PostProcessing>> _postProcesses{};

    std::shared_ptr<Camera> _activeCamera = nullptr;

    // Vulkan pipeline state
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipeline _wireframePipeline = VK_NULL_HANDLE;
    VkPipeline _noCullPipeline = VK_NULL_HANDLE;
    VkPipeline _wireframeNoCullPipeline = VK_NULL_HANDLE;
    VkPipeline _linePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkShaderModule _vertShaderModule = VK_NULL_HANDLE;
    VkShaderModule _fragShaderModule = VK_NULL_HANDLE;
    bool _pipelineReady = false;
};

#endif //FEATHERGL_SCENE_H
