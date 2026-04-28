#include "Shader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#if HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

unsigned int Shader::s_nextId = 1;

Shader::Shader()
    : m_device(VK_NULL_HANDLE)
    , m_vertModule(VK_NULL_HANDLE)
    , m_fragModule(VK_NULL_HANDLE)
    , m_id(s_nextId++)
{
}

Shader::~Shader() {
    if (m_device != VK_NULL_HANDLE) {
        if (m_vertModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(m_device, m_vertModule, nullptr);
        if (m_fragModule != VK_NULL_HANDLE)
            vkDestroyShaderModule(m_device, m_fragModule, nullptr);
    }
}

std::string Shader::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<uint32_t> Shader::compileGLSL(const std::string& source, bool isVertex) {
#if HAS_SHADERC
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetSourceLanguage(shaderc_source_language_glsl);

    auto kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
    auto result = compiler.CompileGlslToSpv(source, kind,
                                            isVertex ? "vert" : "frag", options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader compilation error: " << result.GetErrorMessage() << std::endl;
        return {};
    }

    return {result.cbegin(), result.cend()};
#else
    // Without shaderc we cannot compile GLSL at runtime.
    // In a real build the shaders would be pre-compiled to SPIR-V.
    std::cerr << "shaderc not available -- cannot compile GLSL to SPIR-V at runtime" << std::endl;
    return {};
#endif
}

bool Shader::loadFromFiles(const std::string& vertexPath, const std::string& fragmentPath) {
    std::string vertexSrc = readFile(vertexPath);
    std::string fragmentSrc = readFile(fragmentPath);
    return loadFromSource(vertexSrc, fragmentSrc);
}

bool Shader::loadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
    if (m_device == VK_NULL_HANDLE) {
        std::cerr << "Shader: VkDevice not set before loadFromSource" << std::endl;
        return false;
    }

    auto vertSpirv = compileGLSL(vertexSrc, true);
    if (vertSpirv.empty()) return false;

    auto fragSpirv = compileGLSL(fragmentSrc, false);
    if (fragSpirv.empty()) return false;

    // Create shader modules
    VkShaderModuleCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    ci.codeSize = vertSpirv.size() * sizeof(uint32_t);
    ci.pCode = vertSpirv.data();
    if (vkCreateShaderModule(m_device, &ci, nullptr, &m_vertModule) != VK_SUCCESS) {
        std::cerr << "Failed to create vertex shader module" << std::endl;
        return false;
    }

    ci.codeSize = fragSpirv.size() * sizeof(uint32_t);
    ci.pCode = fragSpirv.data();
    if (vkCreateShaderModule(m_device, &ci, nullptr, &m_fragModule) != VK_SUCCESS) {
        std::cerr << "Failed to create fragment shader module" << std::endl;
        vkDestroyShaderModule(m_device, m_vertModule, nullptr);
        m_vertModule = VK_NULL_HANDLE;
        return false;
    }

    return true;
}
