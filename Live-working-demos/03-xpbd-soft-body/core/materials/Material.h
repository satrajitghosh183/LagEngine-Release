//
// Created by barth on 19/09/2022.
//
// Vulkan port: Material no longer sets GL state. Wireframe and back-face
// culling flags are stored and read by the Vulkan renderer to select
// the appropriate pipeline.
//

#ifndef FEATHERGL_MATERIAL_H
#define FEATHERGL_MATERIAL_H

#include <string>
#include <glm/ext/matrix_float4x4.hpp>
#include "Texture.h"
#include "Observable.h"
#include "Shader.h"

class Material {
public:
    explicit Material(const char *shaderFolder);

    virtual void bind();

    virtual void unbind() {
        _shader.unbind();
    }

    void setWireframe(bool enabled) {
        _isWireframe = enabled;
    }

    bool wireframe() const {
        return _isWireframe;
    }

    bool isBackFaceCullingEnabled() const {
        return _isBackFaceCullingEnabled;
    }

    void setBackFaceCullingEnabled(bool enabled) {
        _isBackFaceCullingEnabled = enabled;
    }

    Shader *shader() {
        return &_shader;
    }

    // Vulkan rendering parameters
    glm::vec3 albedoColor() const { return _albedoColor; }
    void setAlbedoColorInternal(const glm::vec3& c) { _albedoColor = c; }

private:
    Shader _shader;
    bool _isWireframe = false;
    bool _isBackFaceCullingEnabled = true;
    glm::vec3 _albedoColor{1.0f, 1.0f, 1.0f};
};

#endif //FEATHERGL_MATERIAL_H
