#pragma once

#include "../Core/Base.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Vulkan shader program (vertex + fragment stages)
     *
     * Loads SPIR-V bytecode (precompiled .spv files) or compiles GLSL
     * at runtime via shaderc. Creates VkShaderModules and stores
     * stage create infos for pipeline creation.
     *
     * Usage:
     *   auto shader = CreateRef<Shader>("shaders/pbr.vert.spv", "shaders/pbr.frag.spv");
     *   // Pass shader->GetStageCreateInfos() to VulkanPipeline
     *   // Push constants / descriptor sets replace SetUniform*
     */
    class Shader {
    public:
        /**
         * @brief Create shader from SPIR-V file paths
         */
        Shader(const std::string& vertexPath, const std::string& fragmentPath);

        /**
         * @brief Create shader from GLSL source strings (compiled at runtime)
         */
        Shader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);

        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        const std::string& GetName() const { return m_Name; }

        VkShaderModule GetVertexModule() const { return m_VertModule; }
        VkShaderModule GetFragmentModule() const { return m_FragModule; }

        /**
         * @brief Get pipeline shader stage create infos (vertex + fragment)
         */
        std::vector<VkPipelineShaderStageCreateInfo> GetStageCreateInfos() const;

        /**
         * @brief Hot-reload shader from disk
         */
        void Reload();

        /**
         * @brief Hot-reload from source strings
         */
        void ReloadFromSource(const std::string& vertexSrc, const std::string& fragmentSrc);

        bool IsValid() const { return m_VertModule != VK_NULL_HANDLE && m_FragModule != VK_NULL_HANDLE; }

    private:
        void CreateModules(const std::vector<uint32_t>& vertSpirv,
                           const std::vector<uint32_t>& fragSpirv);
        void DestroyModules();

        static std::vector<uint32_t> LoadSpirv(const std::string& path);
        static std::vector<uint32_t> CompileGlsl(const std::string& source,
                                                   const std::string& name,
                                                   bool isVertex);
        static std::string ReadFile(const std::string& filepath);

    private:
        std::string m_Name;
        std::string m_VertexPath;
        std::string m_FragmentPath;

        VkShaderModule m_VertModule = VK_NULL_HANDLE;
        VkShaderModule m_FragModule = VK_NULL_HANDLE;
    };

    /**
     * @brief Shader library for managing multiple shaders
     */
    class ShaderLibrary {
    public:
        void Add(const Ref<Shader>& shader);
        void Add(const std::string& name, const Ref<Shader>& shader);

        Ref<Shader> Load(const std::string& filepath);
        Ref<Shader> Load(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);

        Ref<Shader> Get(const std::string& name);
        bool Exists(const std::string& name) const;

    private:
        std::unordered_map<std::string, Ref<Shader>> m_Shaders;
    };
}
