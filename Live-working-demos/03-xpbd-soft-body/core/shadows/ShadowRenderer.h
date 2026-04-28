//
// Created by barth on 17/10/23.
//
// Vulkan port: ShadowRenderer is a stub. Shadow mapping requires a
// dedicated Vulkan render pass with depth-only attachment, which is
// beyond the scope of this physics demo port. The shadow caster list
// is maintained so the rest of the code compiles unchanged.
//

#ifndef FEATHERGL_SHADOWRENDERER_H
#define FEATHERGL_SHADOWRENDERER_H

#include "DirectionalLight.h"
#include "DepthMaterial.h"

class ShadowRenderer {
public:
    explicit ShadowRenderer(std::shared_ptr<DirectionalLight> directionalLight,
                            const unsigned int shadowMapWidth = 2048, const unsigned int shadowMapHeight = 2048)
            : _width(shadowMapWidth), _height(shadowMapHeight), _directionalLight(directionalLight),
              _depthMaterial(std::make_shared<DepthMaterial>()) {
    }

    void bind() {
        computeProjectionViewMatrix();
    }

    void unbind() {}

    void render() {
        // No-op: shadow pass disabled in Vulkan port
        computeProjectionViewMatrix();
    }

    void computeProjectionViewMatrix() {
        float near_plane = 1.0f, far_plane = 75.0f;
        glm::mat4 lightProjection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, near_plane, far_plane);

        glm::mat4 lightView = glm::lookAt(_directionalLight->getDirection() * 20.0f,
                                          glm::vec3(0.0f, 0.0f, 0.0f),
                                          glm::vec3(0.0f, 1.0f, 0.0f));

        _projectionViewMatrix = lightProjection * lightView;
    }

    glm::mat4 projectionViewMatrix() {
        return _projectionViewMatrix;
    }

    Texture *depthTexture() {
        return &_depthTexture;
    }

    std::shared_ptr<DirectionalLight> directionalLight() {
        return _directionalLight;
    }

    void addShadowCaster(std::shared_ptr<Mesh> mesh) {
        _shadowCasters.push_back(mesh);
    }

    void removeShadowCaster(std::shared_ptr<Mesh> mesh) {
        _shadowCasters.erase(std::remove(_shadowCasters.begin(), _shadowCasters.end(), mesh), _shadowCasters.end());
    }

private:
    std::shared_ptr<DirectionalLight> _directionalLight;
    glm::mat4 _projectionViewMatrix{1.0f};

    std::vector<std::shared_ptr<Mesh>> _shadowCasters;

    std::shared_ptr<DepthMaterial> _depthMaterial;

    Texture _depthTexture;

    unsigned int _width{};
    unsigned int _height{};
};

#endif //FEATHERGL_SHADOWRENDERER_H
