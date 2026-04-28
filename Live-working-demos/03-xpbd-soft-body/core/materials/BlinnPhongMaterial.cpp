//
// Created by barth on 03/10/2022.
//
// Vulkan port: bind() stores uniform data in CPU-side shader maps. No GL calls.

#include "BlinnPhongMaterial.h"
#include "Settings.h"

BlinnPhongMaterial::BlinnPhongMaterial(std::shared_ptr<Scene> scene) : Material("./assets/shaders/blinnPhong"),
                                                                       _scene(scene) {
}

void BlinnPhongMaterial::setDiffuseTexture(std::shared_ptr<Texture> texture) {
    _diffuseTexture = texture;
}

void BlinnPhongMaterial::setDiffuseTextureFromFile(const char *filePath) {
    auto diffuseTexture = std::make_shared<Texture>(filePath);
    setDiffuseTexture(diffuseTexture);
}

void BlinnPhongMaterial::setAmbientTexture(std::shared_ptr<Texture> texture) {
    _ambientTexture = texture;
}

void BlinnPhongMaterial::receiveShadows(std::shared_ptr<ShadowRenderer> shadowRenderer) {
    _shadowRenderer = shadowRenderer;
}

void BlinnPhongMaterial::bind() {
    Material::bind();

    shader()->setVec3("diffuseColor", _diffuseColor);
    shader()->setVec3("ambientColor", _ambientColor);
    shader()->setFloat("shininess", _shininess);
    shader()->setBool("lightingEnabled", _lightingEnabled);

    // Color used for rendering in Vulkan (ambient + diffuse blend)
    glm::vec3 color = _ambientColor + _diffuseColor;
    if (glm::length(color) < 0.001f) color = glm::vec3(1.0f);
    setAlbedoColorInternal(color);
}

void BlinnPhongMaterial::unbind() {
    Material::unbind();
}

void BlinnPhongMaterial::setAmbientColor(float r, float g, float b) {
    _ambientColor.x = r;
    _ambientColor.y = g;
    _ambientColor.z = b;
}

void BlinnPhongMaterial::setDiffuseColor(float r, float g, float b) {
    _diffuseColor.x = r;
    _diffuseColor.y = g;
    _diffuseColor.z = b;
}

void BlinnPhongMaterial::setAlphaColor(float r, float g, float b) {
    _hasAlphaColor = true;
    _alphaColor.x = r;
    _alphaColor.y = g;
    _alphaColor.z = b;
}

void BlinnPhongMaterial::setAmbientTextureFromFile(const char *filePath) {
    auto ambientTexture = std::make_shared<Texture>(filePath);
    setAmbientTexture(ambientTexture);
}
