#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace GameEngine {

    struct PipelineConfigInfo {
        VkPipelineViewportStateCreateInfo ViewportInfo{};
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo{};
        VkPipelineRasterizationStateCreateInfo RasterizationInfo{};
        VkPipelineMultisampleStateCreateInfo MultisampleInfo{};
        VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
        VkPipelineColorBlendStateCreateInfo ColorBlendInfo{};
        VkPipelineDepthStencilStateCreateInfo DepthStencilInfo{};
        std::vector<VkDynamicState> DynamicStateEnables;
        VkPipelineDynamicStateCreateInfo DynamicStateInfo{};

        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
        VkRenderPass RenderPass = VK_NULL_HANDLE;
        uint32_t Subpass = 0;

        // Multiple color attachment support for G-Buffer
        std::vector<VkPipelineColorBlendAttachmentState> ColorBlendAttachments;
    };

    class VulkanPipeline {
    public:
        VulkanPipeline() = default;
        ~VulkanPipeline();

        void CreateGraphicsPipeline(
            const std::string& vertFilepath,
            const std::string& fragFilepath,
            const PipelineConfigInfo& configInfo);

        void CreateGraphicsPipeline(
            const std::vector<uint32_t>& vertSpirv,
            const std::vector<uint32_t>& fragSpirv,
            const PipelineConfigInfo& configInfo);

        void CreateComputePipeline(
            const std::string& compFilepath,
            VkPipelineLayout layout);

        void CreateComputePipeline(
            const std::vector<uint32_t>& compSpirv,
            VkPipelineLayout layout);

        void Bind(VkCommandBuffer commandBuffer) const;
        void BindCompute(VkCommandBuffer commandBuffer) const;
        void Destroy();

        VkPipeline GetPipeline() const { return m_Pipeline; }

        static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
        static void EnableAlphaBlending(PipelineConfigInfo& configInfo);
        static void EnableWireframe(PipelineConfigInfo& configInfo);

    private:
        VkShaderModule createShaderModule(const std::vector<uint32_t>& code);
        std::vector<uint32_t> readSpirvFile(const std::string& filepath);
        std::vector<uint32_t> compileGlslToSpirv(const std::string& filepath, VkShaderStageFlagBits stage);

    private:
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipeline m_ComputePipeline = VK_NULL_HANDLE;
    };

} // namespace GameEngine
