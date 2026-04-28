// Shader.hpp -- Vulkan-compatible shader wrapper (stub)
//
// In the Vulkan port the entry points compile GLSL -> SPIR-V via shaderc
// and create VkShaderModules directly through VulkanBase.  This class is
// retained so that engine code that references Shader still compiles, but
// all legacy functionality has been removed.
#pragma once

#include <string>
#include <glm/glm.hpp>

namespace engine::graphics {

class Shader {
public:
    /// Construct from GLSL source file paths (no-op in Vulkan build).
    Shader(const std::string& /*vertexPath*/, const std::string& /*fragmentPath*/) {}
    ~Shader() = default;

    // ---- Pipeline binding (no-ops -- pipeline binding is done at command buffer level) ----
    void bind() const {}
    void unbind() const {}

    // ---- Uniform helpers (no-ops -- uniforms are push constants in Vulkan) ----
    void setMaterial(const std::string&, const glm::vec3&, const glm::vec3&,
                     const glm::vec3&, float) const {}
    void setLight(const std::string&, const glm::vec3&, const glm::vec3&,
                  const glm::vec3&, const glm::vec3&) const {}

    void setUniformMat4(const std::string&, const glm::mat4&) const {}
    void setUniformVec3(const std::string&, const glm::vec3&) const {}
    void setUniformFloat(const std::string&, float) const {}
    void setUniformInt(const std::string&, int) const {}
    void setMat4(const std::string&, const glm::mat4&) const {}
    void setVec3(const std::string&, const glm::vec3&) const {}
    void setFloat(const std::string&, float) const {}

    bool isValid() const { return true; }
};

} // namespace engine::graphics
