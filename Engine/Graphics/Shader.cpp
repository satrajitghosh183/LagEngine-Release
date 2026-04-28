#include "Shader.hpp"
#include "../Core/Logger.hpp"
#include "../Core/RuntimePaths.hpp"
#include "Vulkan/VulkanDevice.hpp"
#include <fstream>
#include <sstream>

#ifdef GE_HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace GameEngine {

    // ========== Shader ==========

    Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
        : m_VertexPath(vertexPath), m_FragmentPath(fragmentPath) {

        // Extract name from file path
        auto lastSlash = vertexPath.find_last_of("/\\");
        lastSlash = lastSlash == std::string::npos ? 0 : lastSlash + 1;
        auto lastDot = vertexPath.rfind('.');
        auto count = lastDot == std::string::npos ? vertexPath.size() - lastSlash : lastDot - lastSlash;
        m_Name = vertexPath.substr(lastSlash, count);

        auto vertSpirv = LoadSpirv(vertexPath);
        auto fragSpirv = LoadSpirv(fragmentPath);

        if (vertSpirv.empty() || fragSpirv.empty()) {
            // Try loading as GLSL source and compiling
            std::string resolvedVert = RuntimePaths::ResolveShader(vertexPath);
            std::string resolvedFrag = RuntimePaths::ResolveShader(fragmentPath);
            std::string vertSrc = ReadFile(resolvedVert);
            std::string fragSrc = ReadFile(resolvedFrag);

            if (!vertSrc.empty() && !fragSrc.empty()) {
                vertSpirv = CompileGlsl(vertSrc, m_Name + ".vert", true);
                fragSpirv = CompileGlsl(fragSrc, m_Name + ".frag", false);
            }
        }

        CreateModules(vertSpirv, fragSpirv);
        GE_CORE_INFO("Shader '{0}' created successfully", m_Name);
    }

    Shader::Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
        : m_Name(name) {

        auto vertSpirv = CompileGlsl(vertexSrc, name + ".vert", true);
        auto fragSpirv = CompileGlsl(fragmentSrc, name + ".frag", false);

        CreateModules(vertSpirv, fragSpirv);
        GE_CORE_INFO("Shader '{0}' created from source", m_Name);
    }

    Shader::~Shader() {
        DestroyModules();
    }

    std::vector<VkPipelineShaderStageCreateInfo> Shader::GetStageCreateInfos() const {
        std::vector<VkPipelineShaderStageCreateInfo> stages(2);

        stages[0] = {};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = m_VertModule;
        stages[0].pName = "main";

        stages[1] = {};
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = m_FragModule;
        stages[1].pName = "main";

        return stages;
    }

    void Shader::Reload() {
        if (!m_VertexPath.empty() && !m_FragmentPath.empty()) {
            GE_CORE_INFO("Reloading shader '{0}'...", m_Name);
            DestroyModules();

            auto vertSpirv = LoadSpirv(m_VertexPath);
            auto fragSpirv = LoadSpirv(m_FragmentPath);

            if (vertSpirv.empty() || fragSpirv.empty()) {
                std::string resolvedVert = RuntimePaths::ResolveShader(m_VertexPath);
                std::string resolvedFrag = RuntimePaths::ResolveShader(m_FragmentPath);
                std::string vertSrc = ReadFile(resolvedVert);
                std::string fragSrc = ReadFile(resolvedFrag);

                vertSpirv = CompileGlsl(vertSrc, m_Name + ".vert", true);
                fragSpirv = CompileGlsl(fragSrc, m_Name + ".frag", false);
            }

            CreateModules(vertSpirv, fragSpirv);
            GE_CORE_INFO("Shader '{0}' reloaded successfully", m_Name);
        }
    }

    void Shader::ReloadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc) {
        GE_CORE_INFO("Reloading shader '{0}' from source...", m_Name);
        DestroyModules();

        auto vertSpirv = CompileGlsl(vertexSrc, m_Name + ".vert", true);
        auto fragSpirv = CompileGlsl(fragmentSrc, m_Name + ".frag", false);

        CreateModules(vertSpirv, fragSpirv);
        GE_CORE_INFO("Shader '{0}' reloaded from source successfully", m_Name);
    }

    void Shader::CreateModules(const std::vector<uint32_t>& vertSpirv,
                                const std::vector<uint32_t>& fragSpirv) {
        auto device = VulkanDevice::Get().GetDevice();

        if (!vertSpirv.empty()) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = vertSpirv.size() * sizeof(uint32_t);
            createInfo.pCode = vertSpirv.data();

            if (vkCreateShaderModule(device, &createInfo, nullptr, &m_VertModule) != VK_SUCCESS) {
                GE_CORE_ERROR("Failed to create vertex shader module: {0}", m_Name);
            }
        }

        if (!fragSpirv.empty()) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = fragSpirv.size() * sizeof(uint32_t);
            createInfo.pCode = fragSpirv.data();

            if (vkCreateShaderModule(device, &createInfo, nullptr, &m_FragModule) != VK_SUCCESS) {
                GE_CORE_ERROR("Failed to create fragment shader module: {0}", m_Name);
            }
        }
    }

    void Shader::DestroyModules() {
        auto device = VulkanDevice::Get().GetDevice();
        if (m_VertModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_VertModule, nullptr);
            m_VertModule = VK_NULL_HANDLE;
        }
        if (m_FragModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_FragModule, nullptr);
            m_FragModule = VK_NULL_HANDLE;
        }
    }

    std::vector<uint32_t> Shader::LoadSpirv(const std::string& path) {
        // Try loading as .spv binary first
        std::string spvPath = path;
        if (spvPath.find(".spv") == std::string::npos) {
            spvPath += ".spv";
        }

        std::string resolvedPath = RuntimePaths::ResolveShader(spvPath);
        std::ifstream file(resolvedPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return {}; // Caller will try GLSL compilation
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> code(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(fileSize));

        return code;
    }

    std::vector<uint32_t> Shader::CompileGlsl(const std::string& source,
                                                const std::string& name,
                                                bool isVertex) {
#ifdef GE_HAS_SHADERC
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        auto kind = isVertex ? shaderc_glsl_vertex_shader : shaderc_glsl_fragment_shader;
        auto result = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            GE_CORE_ERROR("Shader compilation failed ({0}): {1}", name, result.GetErrorMessage());
            return {};
        }

        return {result.cbegin(), result.cend()};
#else
        (void)source; (void)name; (void)isVertex;
        GE_CORE_ERROR("Runtime GLSL compilation requires shaderc (GE_HAS_SHADERC)");
        return {};
#endif
    }

    std::string Shader::ReadFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            GE_CORE_ERROR("Failed to open shader file: {0}", filepath);
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    // ========== ShaderLibrary ==========

    void ShaderLibrary::Add(const Ref<Shader>& shader) {
        auto& name = shader->GetName();
        Add(name, shader);
    }

    void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader) {
        GE_CORE_ASSERT(!Exists(name), "Shader already exists!");
        m_Shaders[name] = shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& filepath) {
        auto shader = CreateRef<Shader>(filepath, filepath);
        Add(shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath) {
        auto shader = CreateRef<Shader>(vertexPath, fragmentPath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(const std::string& name) {
        GE_CORE_ASSERT(Exists(name), "Shader not found!");
        return m_Shaders[name];
    }

    bool ShaderLibrary::Exists(const std::string& name) const {
        return m_Shaders.find(name) != m_Shaders.end();
    }
}
