//
// Created by barth on 27/09/23.
//
// Vulkan port: Shader is now a no-op data container. The actual GPU shaders
// are compiled at pipeline creation time inside the VulkanRenderer. The
// uniform setters are retained so that Material subclasses compile without
// changes, but the values are stored in CPU-side maps and consumed by the
// renderer when it builds push constants / descriptor sets.
//

#ifndef FEATHERGL_SHADER_H
#define FEATHERGL_SHADER_H

#include <string>
#include <unordered_map>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include "Observable.h"

// Forward declaration - Texture is no longer bound through shader
class Texture;

class Shader {
public:
    explicit Shader(const char *shaderFolder) : _shaderFolder(shaderFolder) {
        // No GL compilation; shaders are compiled as part of the Vulkan pipeline
    }

    void compile() {
        // No-op in Vulkan - pipeline creation handles this
    }

    virtual void bind() {
        onBeforeBindObservable.notifyObservers();
        onAfterBindObservable.notifyObservers();
    }

    virtual void unbind() {}

    void setMat4(const char *uniformName, const glm::mat4 *matrix) {
        _mat4Uniforms[uniformName] = *matrix;
    }

    void setVec2(const char *uniformName, const float x, const float y) {
        _vec2Uniforms[uniformName] = glm::vec2(x, y);
    }

    void setVec2(const char *uniformName, const glm::vec2 vector) {
        _vec2Uniforms[uniformName] = vector;
    }

    void setVec3(const char *uniformName, const glm::vec3 vector) {
        _vec3Uniforms[uniformName] = vector;
    }

    void setInt(const char *uniformName, int integer) {
        _intUniforms[uniformName] = integer;
    }

    void setFloat(const char *uniformName, float value) {
        _floatUniforms[uniformName] = value;
    }

    void setBool(const char *uniformName, bool value) {
        _intUniforms[uniformName] = value ? 1 : 0;
    }

    void bindTexture(const char *uniformName, Texture *texture, int id) {
        // No-op in Vulkan port - textures handled through descriptor sets
        (void)uniformName;
        (void)texture;
        (void)id;
    }

    void setDefine(const char *defineName) {
        // Store for potential future use
        (void)defineName;
    }

    const std::string& shaderFolder() const { return _shaderFolder; }

    // Accessors for uniform data (used by VulkanRenderer)
    const std::unordered_map<std::string, glm::mat4>& mat4Uniforms() const { return _mat4Uniforms; }
    const std::unordered_map<std::string, glm::vec3>& vec3Uniforms() const { return _vec3Uniforms; }
    const std::unordered_map<std::string, float>& floatUniforms() const { return _floatUniforms; }

    Observable<> onBeforeBindObservable{};
    Observable<> onAfterBindObservable{};

private:
    std::string _shaderFolder;
    std::unordered_map<std::string, glm::mat4> _mat4Uniforms;
    std::unordered_map<std::string, glm::vec3> _vec3Uniforms;
    std::unordered_map<std::string, glm::vec2> _vec2Uniforms;
    std::unordered_map<std::string, float> _floatUniforms;
    std::unordered_map<std::string, int> _intUniforms;
};

#endif //FEATHERGL_SHADER_H
