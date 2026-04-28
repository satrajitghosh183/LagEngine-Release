//
// Created by barth on 19/10/23.
//
// Vulkan port: PBR material stores parameters but does not issue GL calls.
// The Vulkan renderer reads material properties to build push constants.
//

#ifndef FEATHERGL_PBRMATERIAL_H
#define FEATHERGL_PBRMATERIAL_H

#include "Material.h"
#include "Scene.h"
#include "Settings.h"

class PbrMaterial : public Material {
public:
    explicit PbrMaterial(std::shared_ptr<Scene> scene) : Material("./assets/shaders/pbr"), _scene(scene) {
    }

    void bind() override {
        Material::bind();
        // All uniform data is stored in CPU-side maps via Shader setters.
        // The Vulkan renderer reads material albedoColor / metallic / roughness
        // directly and builds push constants from them.
        shader()->setVec3("albedoColor", _albedoColor);
        shader()->setFloat("metallic", metallic);
        shader()->setFloat("roughness", roughness);
    }

    void unbind() override {
        Material::unbind();
    }

    void setLightingEnabled(bool enabled) { _lightingEnabled = enabled; }

    void setMetallic(float m) { this->metallic = m; }

    void setRoughness(float r) { this->roughness = r; }

    void setRoughnessTexture(Texture *texture) {
        _roughnessTexture = texture;
    }

    void receiveShadows(std::shared_ptr<ShadowRenderer> shadowRenderer) {
        _shadowRenderer = shadowRenderer;
    }

    void setAlbedoColor(float r, float g, float b) {
        _albedoColor.x = r;
        _albedoColor.y = g;
        _albedoColor.z = b;
        setAlbedoColorInternal(_albedoColor);
    }

    void setAlbedoTexture(Texture *texture) {
        _albedoTexture = texture;
    }

    void setNormalTexture(Texture *texture) {
        _normalTexture = texture;
    }

    void setAmbientColor(float r, float g, float b) {
        _ambientColor.x = r;
        _ambientColor.y = g;
        _ambientColor.z = b;
    }

    glm::vec3 getAlbedoColor() const { return _albedoColor; }
    float getMetallic() const { return metallic; }
    float getRoughness() const { return roughness; }

private:
    std::shared_ptr<Scene> _scene;

    Texture *_albedoTexture = nullptr;
    Texture *_normalTexture = nullptr;
    Texture *_metallicTexture = nullptr;
    Texture *_roughnessTexture = nullptr;
    Texture *_aoTexture = nullptr;

    glm::vec3 _albedoColor = glm::vec3(1.0);
    glm::vec3 _ambientColor = glm::vec3(0.0);
    bool _hasAlphaColor = false;
    glm::vec3 _alphaColor = glm::vec3(0.0);

    float metallic = 1.0f;
    float roughness = 0.5f;
    float ao = 1.0f;

    bool _lightingEnabled = true;

    std::shared_ptr<ShadowRenderer> _shadowRenderer = nullptr;
};

#endif //FEATHERGL_PBRMATERIAL_H
