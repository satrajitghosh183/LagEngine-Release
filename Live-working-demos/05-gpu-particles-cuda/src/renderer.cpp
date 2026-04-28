#include "renderer.hpp"
#include <shaderc/shaderc.hpp>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#else
#include <unistd.h>
#endif

// ── Embedded GLSL shaders (compiled at runtime via shaderc) ──────────────

static const std::string kParticleVertShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    float pointSize;
} pc;

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor   = inColor;
    gl_Position = pc.viewProj * vec4(inPos.xyz, 1.0);
    gl_PointSize = pc.pointSize;
}
)";

static const std::string kParticleFragShader = R"(
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    // Round point sprite
    vec2 c  = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(c, c);
    if (r2 > 1.0) discard;
    float alpha = smoothstep(1.0, 0.6, r2);
    outColor = vec4(fragColor.rgb, alpha * fragColor.a);
}
)";

// ── Shader compilation helper ────────────────────────────────────────────

VkShaderModule Renderer::compileGLSL(const std::string& source, bool isVertex) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
    auto result = compiler.CompileGlslToSpv(source, kind,
        isVertex ? "particles.vert" : "particles.frag", options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::string("Shader compile error: ") +
                                 result.GetErrorMessage());
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    return vkBase->CreateShaderModule(spirv);
}

// ── External memory buffer creation (for CUDA-Vulkan interop) ────────────

void Renderer::createExternalBuffer(VkDeviceSize size,
                                     VkBufferUsageFlags usage,
                                     VkBuffer& outBuffer,
                                     VkDeviceMemory& outMemory,
                                     VkDeviceSize& outAllocSize)
{
    VkDevice device = vkBase->GetDevice();
    VkPhysicalDevice physDev = vkBase->GetPhysicalDevice();

    // Create buffer with external memory info
    VkExternalMemoryBufferCreateInfo extBufInfo{};
    extBufInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
#ifdef _WIN32
    extBufInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    extBufInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext = &extBufInfo;
    bci.size  = size;
    bci.usage = usage;
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bci, nullptr, &outBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to create external buffer");

    // Query memory requirements
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, outBuffer, &memReq);

    // Find device-local memory type
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProps);

    uint32_t memTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((memReq.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
            memTypeIndex = i;
            break;
        }
    }
    if (memTypeIndex == UINT32_MAX)
        throw std::runtime_error("Failed to find suitable memory type for external buffer");

    // Allocate with export info
    VkExportMemoryAllocateInfo exportInfo{};
    exportInfo.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO;
#ifdef _WIN32
    exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
    exportInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.pNext = &exportInfo;
    mai.allocationSize  = memReq.size;
    mai.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device, &mai, nullptr, &outMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate external memory");

    vkBindBufferMemory(device, outBuffer, outMemory, 0);
    outAllocSize = memReq.size;
}

// ── Get platform-specific memory handle ──────────────────────────────────

void* Renderer::getMemoryHandle(VkDeviceMemory memory) {
    VkDevice device = vkBase->GetDevice();

#ifdef _WIN32
    VkMemoryGetWin32HandleInfoKHR handleInfo{};
    handleInfo.sType      = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
    handleInfo.memory     = memory;
    handleInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;

    auto fn = (PFN_vkGetMemoryWin32HandleKHR)
        vkGetDeviceProcAddr(device, "vkGetMemoryWin32HandleKHR");
    if (!fn) throw std::runtime_error("vkGetMemoryWin32HandleKHR not available");

    HANDLE handle = nullptr;
    if (fn(device, &handleInfo, &handle) != VK_SUCCESS)
        throw std::runtime_error("Failed to get Win32 memory handle");

    return static_cast<void*>(handle);
#else
    VkMemoryGetFdInfoKHR fdInfo{};
    fdInfo.sType      = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
    fdInfo.memory     = memory;
    fdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

    auto fn = (PFN_vkGetMemoryFdKHR)
        vkGetDeviceProcAddr(device, "vkGetMemoryFdKHR");
    if (!fn) throw std::runtime_error("vkGetMemoryFdKHR not available");

    int fd = -1;
    if (fn(device, &fdInfo, &fd) != VK_SUCCESS)
        throw std::runtime_error("Failed to get fd memory handle");

    return reinterpret_cast<void*>(static_cast<intptr_t>(fd));
#endif
}

// ── Pipeline creation ────────────────────────────────────────────────────

bool Renderer::init(vkdemo::VulkanBase& base, size_t particleCount) {
    vkBase = &base;
    VkDevice device = vkBase->GetDevice();

    // ── Compile shaders ──
    VkShaderModule vertModule = compileGLSL(kParticleVertShader, true);
    VkShaderModule fragModule = compileGLSL(kParticleFragShader, false);

    // ── Push constant range ──
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(ParticlePushConstants);

    pipelineLayout = vkBase->CreatePipelineLayout({}, {pcRange});

    // ── Shader stages ──
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertModule;
    vertStage.pName  = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragModule;
    fragStage.pName  = "main";

    std::array<VkPipelineShaderStageCreateInfo, 2> stages = {vertStage, fragStage};

    // ── Vertex input: two bindings (pos float4, color float4) ──
    std::array<VkVertexInputBindingDescription, 2> bindings{};
    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(float) * 4;
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    bindings[1].binding   = 1;
    bindings[1].stride    = sizeof(float) * 4;
    bindings[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attrs{};
    attrs[0].binding  = 0;
    attrs[0].location = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[0].offset   = 0;
    attrs[1].binding  = 1;
    attrs[1].location = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrs[1].offset   = 0;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions      = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions    = attrs.data();

    // ── Input assembly: point list ──
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    // ── Dynamic viewport/scissor ──
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    std::array<VkDynamicState, 2> dynStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynState.pDynamicStates    = dynStates.data();

    // ── Rasterization ──
    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    // ── Multisampling (off) ──
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // ── Depth stencil ──
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // ── Color blending (alpha blending for smooth point sprites) ──
    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.blendEnable         = VK_TRUE;
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp        = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttach.alphaBlendOp        = VK_BLEND_OP_ADD;
    blendAttach.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments    = &blendAttach;

    // ── Create graphics pipeline ──
    VkGraphicsPipelineCreateInfo pci{};
    pci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount          = static_cast<uint32_t>(stages.size());
    pci.pStages             = stages.data();
    pci.pVertexInputState   = &vertexInput;
    pci.pInputAssemblyState = &inputAssembly;
    pci.pViewportState      = &viewportState;
    pci.pRasterizationState = &raster;
    pci.pMultisampleState   = &msaa;
    pci.pDepthStencilState  = &depthStencil;
    pci.pColorBlendState    = &colorBlend;
    pci.pDynamicState       = &dynState;
    pci.layout              = pipelineLayout;
    pci.renderPass          = vkBase->GetRenderPass();
    pci.subpass             = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create particle graphics pipeline");

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);

    // ── Create shared Vulkan-CUDA vertex buffers ──
    VkDeviceSize posBytes = particleCount * sizeof(float) * 4;
    VkDeviceSize colBytes = particleCount * sizeof(float) * 4;

    createExternalBuffer(posBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         posBuffer, posMemory, posAllocSize);
    createExternalBuffer(colBytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         colBuffer, colMemory, colAllocSize);

    // Import into CUDA
    void* posHandle = getMemoryHandle(posMemory);
    importVulkanBufferToCuda(cudaPosBuf, posHandle, posAllocSize, posBytes);

    void* colHandle = getMemoryHandle(colMemory);
    importVulkanBufferToCuda(cudaColBuf, colHandle, colAllocSize, colBytes);

    std::cout << "Vulkan Renderer initialized (CUDA-Vulkan interop)\n";

    // Print Vulkan device info
    VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(vkBase->GetPhysicalDevice(), &devProps);
    std::cout << "  Vulkan Device: " << devProps.deviceName << "\n";
    std::cout << "  API Version:   " << VK_VERSION_MAJOR(devProps.apiVersion) << "."
              << VK_VERSION_MINOR(devProps.apiVersion) << "."
              << VK_VERSION_PATCH(devProps.apiVersion) << "\n\n";

    return true;
}

// ── Draw call ────────────────────────────────────────────────────────────

void Renderer::draw(VkCommandBuffer cmd, size_t particleCount, const float* viewProj) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Push constants
    ParticlePushConstants pc{};
    std::memcpy(pc.viewProj, viewProj, sizeof(float) * 16);
    pc.pointSize = 3.0f;
    vkCmdPushConstants(cmd, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(ParticlePushConstants), &pc);

    // Bind vertex buffers
    VkBuffer     buffers[] = {posBuffer, colBuffer};
    VkDeviceSize offsets[]  = {0, 0};
    vkCmdBindVertexBuffers(cmd, 0, 2, buffers, offsets);

    // Draw
    vkCmdDraw(cmd, static_cast<uint32_t>(particleCount), 1, 0, 0);
}

// ── Cleanup ──────────────────────────────────────────────────────────────

void Renderer::shutdown() {
    if (!vkBase) return;
    VkDevice device = vkBase->GetDevice();

    vkDeviceWaitIdle(device);

    // Destroy CUDA interop first
    destroyCudaVulkanBuffer(cudaPosBuf);
    destroyCudaVulkanBuffer(cudaColBuf);

    // Destroy Vulkan resources
    if (posBuffer) { vkDestroyBuffer(device, posBuffer, nullptr); posBuffer = VK_NULL_HANDLE; }
    if (posMemory) { vkFreeMemory(device, posMemory, nullptr);    posMemory = VK_NULL_HANDLE; }
    if (colBuffer) { vkDestroyBuffer(device, colBuffer, nullptr); colBuffer = VK_NULL_HANDLE; }
    if (colMemory) { vkFreeMemory(device, colMemory, nullptr);    colMemory = VK_NULL_HANDLE; }

    if (pipeline)       { vkDestroyPipeline(device, pipeline, nullptr);             pipeline = VK_NULL_HANDLE; }
    if (pipelineLayout) { vkDestroyPipelineLayout(device, pipelineLayout, nullptr); pipelineLayout = VK_NULL_HANDLE; }

    vkBase = nullptr;
}
