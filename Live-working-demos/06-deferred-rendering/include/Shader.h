#pragma once

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

// Vulkan shader: compiles GLSL to SPIR-V at runtime via shaderc,
// then creates VkShaderModule objects.
class Shader {
public:
    Shader();
    ~Shader();

    // Initialize with a Vulkan device (must be called before load)
    void setDevice(VkDevice device) { m_device = device; }

    bool loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath);
    bool loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

    VkShaderModule getVertexModule() const { return m_vertModule; }
    VkShaderModule getFragmentModule() const { return m_fragModule; }

    // Legacy ID accessor (unused in Vulkan, kept for RenderTask GLState compat)
    unsigned int getID() const { return m_id; }

private:
    VkDevice m_device = VK_NULL_HANDLE;
    VkShaderModule m_vertModule = VK_NULL_HANDLE;
    VkShaderModule m_fragModule = VK_NULL_HANDLE;
    unsigned int m_id = 0; // compatibility stub
    static unsigned int s_nextId;

    std::string readFile(const std::string& path);
    std::vector<uint32_t> compileGLSL(const std::string& source, bool isVertex);
};
