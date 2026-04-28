#include "VulkanDescriptors.hpp"
#include "VulkanDevice.hpp"

#include <cassert>
#include <stdexcept>

namespace GameEngine {

    // ---- Descriptor Set Layout Builder ----

    VulkanDescriptorSetLayout::Builder& VulkanDescriptorSetLayout::Builder::AddBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count) {

        assert(m_Bindings.count(binding) == 0 && "Binding already in use");

        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;

        m_Bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<VulkanDescriptorSetLayout> VulkanDescriptorSetLayout::Builder::Build() const {
        return std::make_unique<VulkanDescriptorSetLayout>(m_Bindings);
    }

    // ---- Descriptor Set Layout ----

    VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
        const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& bindings)
        : m_Bindings(bindings) {

        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings;
        for (auto& [binding, layoutBinding] : bindings) {
            setLayoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        createInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(VulkanDevice::Get().GetDevice(), &createInfo, nullptr,
                                         &m_DescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }
    }

    VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() {
        if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(VulkanDevice::Get().GetDevice(), m_DescriptorSetLayout, nullptr);
        }
    }

    // ---- Descriptor Pool Builder ----

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::AddPoolSize(
        VkDescriptorType descriptorType, uint32_t count) {
        m_PoolSizes.push_back({descriptorType, count});
        return *this;
    }

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::SetPoolFlags(
        VkDescriptorPoolCreateFlags flags) {
        m_PoolFlags = flags;
        return *this;
    }

    VulkanDescriptorPool::Builder& VulkanDescriptorPool::Builder::SetMaxSets(uint32_t count) {
        m_MaxSets = count;
        return *this;
    }

    std::unique_ptr<VulkanDescriptorPool> VulkanDescriptorPool::Builder::Build() const {
        return std::make_unique<VulkanDescriptorPool>(m_MaxSets, m_PoolFlags, m_PoolSizes);
    }

    // ---- Descriptor Pool ----

    VulkanDescriptorPool::VulkanDescriptorPool(
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes) {

        VkDescriptorPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        createInfo.pPoolSizes = poolSizes.data();
        createInfo.maxSets = maxSets;
        createInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(VulkanDevice::Get().GetDevice(), &createInfo, nullptr,
                                    &m_DescriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    VulkanDescriptorPool::~VulkanDescriptorPool() {
        if (m_DescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(VulkanDevice::Get().GetDevice(), m_DescriptorPool, nullptr);
        }
    }

    bool VulkanDescriptorPool::AllocateDescriptorSet(
        const VkDescriptorSetLayout descriptorSetLayout,
        VkDescriptorSet& descriptor) const {

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_DescriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        if (vkAllocateDescriptorSets(VulkanDevice::Get().GetDevice(), &allocInfo, &descriptor) != VK_SUCCESS) {
            return false;
        }
        return true;
    }

    void VulkanDescriptorPool::FreeDescriptors(std::vector<VkDescriptorSet>& descriptors) const {
        vkFreeDescriptorSets(VulkanDevice::Get().GetDevice(), m_DescriptorPool,
                             static_cast<uint32_t>(descriptors.size()), descriptors.data());
    }

    void VulkanDescriptorPool::ResetPool() {
        vkResetDescriptorPool(VulkanDevice::Get().GetDevice(), m_DescriptorPool, 0);
    }

    // ---- Descriptor Writer ----

    VulkanDescriptorWriter::VulkanDescriptorWriter(
        VulkanDescriptorSetLayout& setLayout,
        VulkanDescriptorPool& pool)
        : m_SetLayout(setLayout), m_Pool(pool) {}

    VulkanDescriptorWriter& VulkanDescriptorWriter::WriteBuffer(
        uint32_t binding, VkDescriptorBufferInfo* bufferInfo) {

        assert(m_SetLayout.m_Bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = m_SetLayout.m_Bindings[binding];

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        m_Writes.push_back(write);
        return *this;
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImage(
        uint32_t binding, VkDescriptorImageInfo* imageInfo) {

        assert(m_SetLayout.m_Bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = m_SetLayout.m_Bindings[binding];

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        m_Writes.push_back(write);
        return *this;
    }

    VulkanDescriptorWriter& VulkanDescriptorWriter::WriteImages(
        uint32_t binding, std::vector<VkDescriptorImageInfo>& imageInfos) {

        assert(m_SetLayout.m_Bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = m_SetLayout.m_Bindings[binding];

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfos.data();
        write.descriptorCount = static_cast<uint32_t>(imageInfos.size());

        m_Writes.push_back(write);
        return *this;
    }

    bool VulkanDescriptorWriter::Build(VkDescriptorSet& set) {
        bool success = m_Pool.AllocateDescriptorSet(m_SetLayout.GetDescriptorSetLayout(), set);
        if (!success) return false;
        Overwrite(set);
        return true;
    }

    void VulkanDescriptorWriter::Overwrite(VkDescriptorSet& set) {
        for (auto& write : m_Writes) {
            write.dstSet = set;
        }
        vkUpdateDescriptorSets(VulkanDevice::Get().GetDevice(),
                               static_cast<uint32_t>(m_Writes.size()), m_Writes.data(), 0, nullptr);
    }

} // namespace GameEngine
