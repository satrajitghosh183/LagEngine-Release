#include "ComputeShader.hpp"
#include "../Core/Logger.hpp"
#include "../Core/RuntimePaths.hpp"
#include <fstream>
#include <algorithm>

#ifdef GE_HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace GameEngine {

    ComputeShader::ComputeShader(const std::string& spirvPath) {
        LoadFromFile(spirvPath);
    }

    ComputeShader::ComputeShader(const std::string& name, const std::vector<uint32_t>& spirvCode) {
        LoadFromBytecode(spirvCode, name);
    }

    ComputeShader::~ComputeShader() {
        Destroy();
    }

    ComputeShader::ComputeShader(ComputeShader&& other) noexcept
        : m_Name(std::move(other.m_Name))
        , m_ShaderModule(other.m_ShaderModule)
        , m_Pipeline(other.m_Pipeline)
        , m_PipelineLayout(other.m_PipelineLayout) {
        other.m_ShaderModule = VK_NULL_HANDLE;
        other.m_Pipeline = VK_NULL_HANDLE;
        other.m_PipelineLayout = VK_NULL_HANDLE;
    }

    ComputeShader& ComputeShader::operator=(ComputeShader&& other) noexcept {
        if (this != &other) {
            Destroy();
            m_Name = std::move(other.m_Name);
            m_ShaderModule = other.m_ShaderModule;
            m_Pipeline = other.m_Pipeline;
            m_PipelineLayout = other.m_PipelineLayout;
            other.m_ShaderModule = VK_NULL_HANDLE;
            other.m_Pipeline = VK_NULL_HANDLE;
            other.m_PipelineLayout = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool ComputeShader::LoadFromFile(const std::string& spirvPath) {
        // Try .spv extension first
        std::string path = spirvPath;
        if (path.find(".spv") == std::string::npos) {
            path += ".spv";
        }

        std::string resolvedPath = RuntimePaths::ResolveShader(path);

        std::ifstream file(resolvedPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            // Try without .spv — maybe it's GLSL source to compile at runtime
            std::string glslPath = spirvPath;
            if (glslPath.size() >= 4 &&
                glslPath.compare(glslPath.size() - 4, 4, ".spv") == 0) {
                glslPath = glslPath.substr(0, glslPath.size() - 4);
            }
            resolvedPath = RuntimePaths::ResolveShader(glslPath);
            std::ifstream glslFile(resolvedPath);
            if (glslFile.is_open()) {
                std::string source((std::istreambuf_iterator<char>(glslFile)),
                                    std::istreambuf_iterator<char>());
                return LoadFromSource(source, spirvPath);
            }
            GE_CORE_ERROR("Failed to open compute shader: {0}", spirvPath);
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> code(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(fileSize));

        // Extract name from path
        auto lastSlash = spirvPath.find_last_of("/\\");
        m_Name = (lastSlash != std::string::npos) ? spirvPath.substr(lastSlash + 1) : spirvPath;

        return LoadFromBytecode(code, m_Name);
    }

    bool ComputeShader::LoadFromSource(const std::string& glslSource, const std::string& name) {
#ifdef GE_HAS_SHADERC
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        auto result = compiler.CompileGlslToSpv(glslSource, shaderc_glsl_compute_shader,
                                                 name.c_str(), options);
        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            GE_CORE_ERROR("Compute shader compilation failed ({0}): {1}", name, result.GetErrorMessage());
            return false;
        }

        std::vector<uint32_t> spirv(result.cbegin(), result.cend());
        return LoadFromBytecode(spirv, name);
#else
        (void)glslSource; (void)name;
        GE_CORE_ERROR("Runtime GLSL compilation requires shaderc (GE_HAS_SHADERC)");
        return false;
#endif
    }

    bool ComputeShader::LoadFromBytecode(const std::vector<uint32_t>& spirvCode, const std::string& name) {
        m_Name = name;
        auto device = VulkanDevice::Get().GetDevice();

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = spirvCode.size() * sizeof(uint32_t);
        createInfo.pCode = spirvCode.data();

        if (vkCreateShaderModule(device, &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create compute shader module: {0}", name);
            return false;
        }

        GE_CORE_INFO("Compute shader module loaded: {0}", name);
        return true;
    }

    bool ComputeShader::CreatePipeline(const std::vector<VkDescriptorSetLayout>& setLayouts,
                                        uint32_t pushConstantSize,
                                        VkShaderStageFlags pushConstantStage) {
        auto device = VulkanDevice::Get().GetDevice();

        // Pipeline layout
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
        layoutInfo.pSetLayouts = setLayouts.data();

        VkPushConstantRange pushRange{};
        if (pushConstantSize > 0) {
            pushRange.stageFlags = pushConstantStage;
            pushRange.offset = 0;
            pushRange.size = pushConstantSize;
            layoutInfo.pushConstantRangeCount = 1;
            layoutInfo.pPushConstantRanges = &pushRange;
        }

        if (vkCreatePipelineLayout(device, &layoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create compute pipeline layout: {0}", m_Name);
            return false;
        }

        // Compute pipeline
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = m_PipelineLayout;
        pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineInfo.stage.module = m_ShaderModule;
        pipelineInfo.stage.pName = "main";

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
            GE_CORE_ERROR("Failed to create compute pipeline: {0}", m_Name);
            return false;
        }

        GE_CORE_INFO("Compute pipeline created: {0}", m_Name);
        return true;
    }

    void ComputeShader::Bind(VkCommandBuffer cmd) const {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
    }

    void ComputeShader::BindDescriptorSets(VkCommandBuffer cmd,
                                            const VkDescriptorSet* sets,
                                            uint32_t setCount,
                                            uint32_t firstSet) const {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
                                m_PipelineLayout, firstSet, setCount, sets, 0, nullptr);
    }

    void ComputeShader::PushConstants(VkCommandBuffer cmd, const void* data,
                                       uint32_t size, uint32_t offset) const {
        vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, offset, size, data);
    }

    void ComputeShader::Dispatch(VkCommandBuffer cmd, uint32_t groupCountX,
                                  uint32_t groupCountY, uint32_t groupCountZ) const {
        vkCmdDispatch(cmd, groupCountX, groupCountY, groupCountZ);
    }

    void ComputeShader::DispatchAutoGroups(VkCommandBuffer cmd, uint32_t elementCount,
                                            uint32_t localSizeX) const {
        uint32_t groupCount = (elementCount + localSizeX - 1) / localSizeX;
        vkCmdDispatch(cmd, groupCount, 1, 1);
    }

    void ComputeShader::BufferBarrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize size,
                                       VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                       VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = buffer;
        barrier.offset = 0;
        barrier.size = size;

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                             0, nullptr, 1, &barrier, 0, nullptr);
    }

    void ComputeShader::ImageBarrier(VkCommandBuffer cmd, VkImage image,
                                      VkImageLayout oldLayout, VkImageLayout newLayout,
                                      VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                      VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);
    }

    void ComputeShader::Destroy() {
        auto device = VulkanDevice::Get().GetDevice();
        if (m_Pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }
        if (m_PipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
            m_PipelineLayout = VK_NULL_HANDLE;
        }
        if (m_ShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(device, m_ShaderModule, nullptr);
            m_ShaderModule = VK_NULL_HANDLE;
        }
    }

} // namespace GameEngine
