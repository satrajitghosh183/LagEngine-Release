#include "VulkanPipeline.hpp"
#include "VulkanDevice.hpp"
#include "../../Core/Logger.hpp"

#include <fstream>
#include <stdexcept>
#include <cassert>

#ifdef GE_HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

namespace GameEngine {

    VulkanPipeline::~VulkanPipeline() {
        Destroy();
    }

    void VulkanPipeline::Destroy() {
        auto device = VulkanDevice::Get().GetDevice();
        if (m_Pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, m_Pipeline, nullptr);
            m_Pipeline = VK_NULL_HANDLE;
        }
        if (m_ComputePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(device, m_ComputePipeline, nullptr);
            m_ComputePipeline = VK_NULL_HANDLE;
        }
    }

    void VulkanPipeline::CreateGraphicsPipeline(
        const std::string& vertFilepath,
        const std::string& fragFilepath,
        const PipelineConfigInfo& configInfo) {

        auto vertSpirv = readSpirvFile(vertFilepath);
        if (vertSpirv.empty()) {
            vertSpirv = compileGlslToSpirv(vertFilepath, VK_SHADER_STAGE_VERTEX_BIT);
        }

        auto fragSpirv = readSpirvFile(fragFilepath);
        if (fragSpirv.empty()) {
            fragSpirv = compileGlslToSpirv(fragFilepath, VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        CreateGraphicsPipeline(vertSpirv, fragSpirv, configInfo);
    }

    void VulkanPipeline::CreateGraphicsPipeline(
        const std::vector<uint32_t>& vertSpirv,
        const std::vector<uint32_t>& fragSpirv,
        const PipelineConfigInfo& configInfo) {

        assert(configInfo.PipelineLayout != VK_NULL_HANDLE &&
               "Cannot create graphics pipeline: no pipeline layout provided");
        assert(configInfo.RenderPass != VK_NULL_HANDLE &&
               "Cannot create graphics pipeline: no render pass provided");

        auto vertShaderModule = createShaderModule(vertSpirv);
        auto fragShaderModule = createShaderModule(fragSpirv);

        VkPipelineShaderStageCreateInfo shaderStages[2]{};
        shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[0].module = vertShaderModule;
        shaderStages[0].pName = "main";

        shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[1].module = fragShaderModule;
        shaderStages[1].pName = "main";

        // Vertex input — will be configured per-mesh via vertex input descriptions
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &configInfo.InputAssemblyInfo;
        pipelineInfo.pViewportState = &configInfo.ViewportInfo;
        pipelineInfo.pRasterizationState = &configInfo.RasterizationInfo;
        pipelineInfo.pMultisampleState = &configInfo.MultisampleInfo;
        pipelineInfo.pDepthStencilState = &configInfo.DepthStencilInfo;

        // Use multiple color blend attachments if provided (for G-Buffer)
        if (!configInfo.ColorBlendAttachments.empty()) {
            VkPipelineColorBlendStateCreateInfo colorBlending = configInfo.ColorBlendInfo;
            colorBlending.attachmentCount = static_cast<uint32_t>(configInfo.ColorBlendAttachments.size());
            colorBlending.pAttachments = configInfo.ColorBlendAttachments.data();
            pipelineInfo.pColorBlendState = &colorBlending;
        } else {
            pipelineInfo.pColorBlendState = &configInfo.ColorBlendInfo;
        }

        pipelineInfo.pDynamicState = &configInfo.DynamicStateInfo;
        pipelineInfo.layout = configInfo.PipelineLayout;
        pipelineInfo.renderPass = configInfo.RenderPass;
        pipelineInfo.subpass = configInfo.Subpass;
        pipelineInfo.basePipelineIndex = -1;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        auto device = VulkanDevice::Get().GetDevice();
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, vertShaderModule, nullptr);
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
    }

    void VulkanPipeline::CreateComputePipeline(
        const std::string& compFilepath,
        VkPipelineLayout layout) {

        auto compSpirv = readSpirvFile(compFilepath);
        if (compSpirv.empty()) {
            compSpirv = compileGlslToSpirv(compFilepath, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        CreateComputePipeline(compSpirv, layout);
    }

    void VulkanPipeline::CreateComputePipeline(
        const std::vector<uint32_t>& compSpirv,
        VkPipelineLayout layout) {

        auto device = VulkanDevice::Get().GetDevice();
        auto shaderModule = createShaderModule(compSpirv);

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineInfo.stage.module = shaderModule;
        pipelineInfo.stage.pName = "main";
        pipelineInfo.layout = layout;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_ComputePipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        vkDestroyShaderModule(device, shaderModule, nullptr);
    }

    void VulkanPipeline::Bind(VkCommandBuffer commandBuffer) const {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
    }

    void VulkanPipeline::BindCompute(VkCommandBuffer commandBuffer) const {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline);
    }

    VkShaderModule VulkanPipeline::createShaderModule(const std::vector<uint32_t>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size() * sizeof(uint32_t);
        createInfo.pCode = code.data();

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(VulkanDevice::Get().GetDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module!");
        }
        return shaderModule;
    }

    std::vector<uint32_t> VulkanPipeline::readSpirvFile(const std::string& filepath) {
        // Try .spv extension first
        std::string spvPath = filepath + ".spv";
        std::ifstream file(spvPath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            // Try the filepath as-is (maybe it's already .spv)
            file.open(filepath, std::ios::ate | std::ios::binary);
            if (!file.is_open()) {
                return {}; // File not found — will try runtime compilation
            }
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(fileSize));
        file.close();

        return buffer;
    }

    std::vector<uint32_t> VulkanPipeline::compileGlslToSpirv(
        const std::string& filepath, VkShaderStageFlagBits stage) {

#ifdef GE_HAS_SHADERC
        // Read GLSL source
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + filepath);
        }
        std::string source((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());

        shaderc::Compiler compiler;
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        options.SetOptimizationLevel(shaderc_optimization_level_performance);

        shaderc_shader_kind kind;
        switch (stage) {
            case VK_SHADER_STAGE_VERTEX_BIT:   kind = shaderc_vertex_shader; break;
            case VK_SHADER_STAGE_FRAGMENT_BIT: kind = shaderc_fragment_shader; break;
            case VK_SHADER_STAGE_COMPUTE_BIT:  kind = shaderc_compute_shader; break;
            default:
                throw std::runtime_error("Unsupported shader stage for compilation");
        }

        auto result = compiler.CompileGlslToSpv(source, kind, filepath.c_str(), options);

        if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
            GE_CORE_ERROR("Shader compilation failed for {0}:", filepath);
            GE_CORE_ERROR("  {0}", result.GetErrorMessage());
            throw std::runtime_error("Shader compilation failed: " + result.GetErrorMessage());
        }

        if (result.GetNumWarnings() > 0) {
            GE_CORE_WARN("Shader warnings for {0}: {1}", filepath, result.GetErrorMessage());
        }

        return {result.cbegin(), result.cend()};
#else
        GE_CORE_ERROR("Cannot compile GLSL to SPIR-V: shaderc not available. "
                       "Pre-compile shaders or install the Vulkan SDK.");
        throw std::runtime_error("Runtime shader compilation requires shaderc (Vulkan SDK). "
                                  "File: " + filepath);
        return {};
#endif
    }

    void VulkanPipeline::DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo) {
        configInfo.InputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        configInfo.InputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        configInfo.InputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

        configInfo.ViewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        configInfo.ViewportInfo.viewportCount = 1;
        configInfo.ViewportInfo.pViewports = nullptr;
        configInfo.ViewportInfo.scissorCount = 1;
        configInfo.ViewportInfo.pScissors = nullptr;

        configInfo.RasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        configInfo.RasterizationInfo.depthClampEnable = VK_FALSE;
        configInfo.RasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
        configInfo.RasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
        configInfo.RasterizationInfo.lineWidth = 1.0f;
        configInfo.RasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        configInfo.RasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        configInfo.RasterizationInfo.depthBiasEnable = VK_FALSE;

        configInfo.MultisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        configInfo.MultisampleInfo.sampleShadingEnable = VK_FALSE;
        configInfo.MultisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        configInfo.ColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        configInfo.ColorBlendAttachment.blendEnable = VK_FALSE;

        configInfo.ColorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        configInfo.ColorBlendInfo.logicOpEnable = VK_FALSE;
        configInfo.ColorBlendInfo.attachmentCount = 1;
        configInfo.ColorBlendInfo.pAttachments = &configInfo.ColorBlendAttachment;

        configInfo.DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        configInfo.DepthStencilInfo.depthTestEnable = VK_TRUE;
        configInfo.DepthStencilInfo.depthWriteEnable = VK_TRUE;
        configInfo.DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        configInfo.DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
        configInfo.DepthStencilInfo.stencilTestEnable = VK_FALSE;

        configInfo.DynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        configInfo.DynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        configInfo.DynamicStateInfo.pDynamicStates = configInfo.DynamicStateEnables.data();
        configInfo.DynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(configInfo.DynamicStateEnables.size());
    }

    void VulkanPipeline::EnableAlphaBlending(PipelineConfigInfo& configInfo) {
        configInfo.ColorBlendAttachment.blendEnable = VK_TRUE;
        configInfo.ColorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        configInfo.ColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        configInfo.ColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        configInfo.ColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        configInfo.ColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        configInfo.ColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        configInfo.ColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    }

    void VulkanPipeline::EnableWireframe(PipelineConfigInfo& configInfo) {
        configInfo.RasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
        configInfo.RasterizationInfo.lineWidth = 1.0f;
    }

} // namespace GameEngine
