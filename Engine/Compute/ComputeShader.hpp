#pragma once

#include "../Core/Base.hpp"
#include "../Graphics/Vulkan/VulkanDevice.hpp"
#include "../Graphics/Vulkan/VulkanDescriptors.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Vulkan compute shader wrapper
     *
     * Encapsulates a compute pipeline: load SPIR-V, bind descriptor sets,
     * dispatch work groups. Used for GPU physics, particle simulation,
     * SSAO, IBL convolution, post-processing, etc.
     *
     * Usage:
     *   ComputeShader compute("shaders/particle_update.comp.spv");
     *   compute.Bind(commandBuffer);
     *   compute.PushConstants(commandBuffer, &params, sizeof(params));
     *   compute.Dispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
     */
    class ComputeShader {
    public:
        ComputeShader() = default;

        /**
         * @brief Create from SPIR-V file path
         */
        explicit ComputeShader(const std::string& spirvPath);

        /**
         * @brief Create from SPIR-V bytecode
         */
        ComputeShader(const std::string& name, const std::vector<uint32_t>& spirvCode);

        ~ComputeShader();

        ComputeShader(const ComputeShader&) = delete;
        ComputeShader& operator=(const ComputeShader&) = delete;
        ComputeShader(ComputeShader&& other) noexcept;
        ComputeShader& operator=(ComputeShader&& other) noexcept;

        /**
         * @brief Initialize from SPIR-V file or source
         */
        bool LoadFromFile(const std::string& spirvPath);
        bool LoadFromSource(const std::string& glslSource, const std::string& name = "compute");
        bool LoadFromBytecode(const std::vector<uint32_t>& spirvCode, const std::string& name = "compute");

        /**
         * @brief Create pipeline with given descriptor set layouts and push constant range
         */
        bool CreatePipeline(const std::vector<VkDescriptorSetLayout>& setLayouts,
                             uint32_t pushConstantSize = 0,
                             VkShaderStageFlags pushConstantStage = VK_SHADER_STAGE_COMPUTE_BIT);

        /**
         * @brief Bind the compute pipeline to a command buffer
         */
        void Bind(VkCommandBuffer cmd) const;

        /**
         * @brief Bind descriptor sets
         */
        void BindDescriptorSets(VkCommandBuffer cmd,
                                 const VkDescriptorSet* sets,
                                 uint32_t setCount,
                                 uint32_t firstSet = 0) const;

        /**
         * @brief Push constants
         */
        void PushConstants(VkCommandBuffer cmd, const void* data, uint32_t size, uint32_t offset = 0) const;

        /**
         * @brief Dispatch compute work groups
         */
        void Dispatch(VkCommandBuffer cmd, uint32_t groupCountX,
                      uint32_t groupCountY = 1, uint32_t groupCountZ = 1) const;

        /**
         * @brief Dispatch with automatic group count from element count and local size
         */
        void DispatchAutoGroups(VkCommandBuffer cmd, uint32_t elementCount,
                                 uint32_t localSizeX = 256) const;

        /**
         * @brief Insert memory barrier for compute-to-compute or compute-to-graphics sync
         */
        static void BufferBarrier(VkCommandBuffer cmd, VkBuffer buffer, VkDeviceSize size,
                                   VkAccessFlags srcAccess = VK_ACCESS_SHADER_WRITE_BIT,
                                   VkAccessFlags dstAccess = VK_ACCESS_SHADER_READ_BIT,
                                   VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                   VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

        static void ImageBarrier(VkCommandBuffer cmd, VkImage image,
                                  VkImageLayout oldLayout, VkImageLayout newLayout,
                                  VkAccessFlags srcAccess, VkAccessFlags dstAccess,
                                  VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

        const std::string& GetName() const { return m_Name; }
        VkPipeline GetPipeline() const { return m_Pipeline; }
        VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
        bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

        void Destroy();

    private:
        std::string m_Name;
        VkShaderModule m_ShaderModule = VK_NULL_HANDLE;
        VkPipeline m_Pipeline = VK_NULL_HANDLE;
        VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
    };

} // namespace GameEngine
