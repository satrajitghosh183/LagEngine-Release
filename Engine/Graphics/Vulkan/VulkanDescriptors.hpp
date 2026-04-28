#pragma once

#include <vulkan/vulkan.h>
#include <unordered_map>
#include <vector>
#include <memory>

namespace GameEngine {

    // ---- Descriptor Set Layout Builder ----

    class VulkanDescriptorSetLayout {
    public:
        class Builder {
        public:
            Builder& AddBinding(uint32_t binding, VkDescriptorType descriptorType,
                                VkShaderStageFlags stageFlags, uint32_t count = 1);
            std::unique_ptr<VulkanDescriptorSetLayout> Build() const;

        private:
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;
        };

        VulkanDescriptorSetLayout(const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings);
        ~VulkanDescriptorSetLayout();

        VulkanDescriptorSetLayout(const VulkanDescriptorSetLayout&) = delete;
        VulkanDescriptorSetLayout& operator=(const VulkanDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }

    private:
        VkDescriptorSetLayout m_DescriptorSetLayout = VK_NULL_HANDLE;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings;

        friend class VulkanDescriptorWriter;
    };

    // ---- Descriptor Pool ----

    class VulkanDescriptorPool {
    public:
        class Builder {
        public:
            Builder& AddPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& SetPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& SetMaxSets(uint32_t count);
            std::unique_ptr<VulkanDescriptorPool> Build() const;

        private:
            std::vector<VkDescriptorPoolSize> m_PoolSizes;
            uint32_t m_MaxSets = 1000;
            VkDescriptorPoolCreateFlags m_PoolFlags = 0;
        };

        VulkanDescriptorPool(uint32_t maxSets, VkDescriptorPoolCreateFlags poolFlags,
                             const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VulkanDescriptorPool();

        VulkanDescriptorPool(const VulkanDescriptorPool&) = delete;
        VulkanDescriptorPool& operator=(const VulkanDescriptorPool&) = delete;

        bool AllocateDescriptorSet(const VkDescriptorSetLayout descriptorSetLayout,
                                   VkDescriptorSet& descriptor) const;

        void FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;
        void ResetPool();

        VkDescriptorPool GetPool() const { return m_DescriptorPool; }

    private:
        VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

        friend class VulkanDescriptorWriter;
    };

    // ---- Descriptor Writer ----

    class VulkanDescriptorWriter {
    public:
        VulkanDescriptorWriter(VulkanDescriptorSetLayout& setLayout, VulkanDescriptorPool& pool);

        VulkanDescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        VulkanDescriptorWriter& WriteImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);
        VulkanDescriptorWriter& WriteImages(uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfos);

        bool Build(VkDescriptorSet& set);
        void Overwrite(VkDescriptorSet& set);

    private:
        VulkanDescriptorSetLayout& m_SetLayout;
        VulkanDescriptorPool& m_Pool;
        std::vector<VkWriteDescriptorSet> m_Writes;
    };

} // namespace GameEngine
