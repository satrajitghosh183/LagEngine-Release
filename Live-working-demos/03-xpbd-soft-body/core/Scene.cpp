//
// Created by barth on 19/09/2022.
//
// Vulkan port: Scene creates Vulkan graphics pipelines and records
// draw commands each frame.
//

#include "Scene.h"
#include "DepthMaterial.h"
#include <imgui.h>
#include <array>
#include <cstring>

#if defined(HAS_SHADERC) && HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

// -------------------------------------------------------------------
// Embedded SPIR-V for the lit vertex/fragment shaders.
// We compile from the GLSL source strings at pipeline init time
// using shaderc if available; otherwise we use pre-compiled SPIR-V.
// -------------------------------------------------------------------

// Lit vertex shader GLSL (matches EmbeddedShaders.hpp kLitVertexShader)
static const std::string kVertSrc = R"(
#version 450
layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 model;
    vec4 color;
} pc;
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragWorldPos;
void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragWorldPos = vec3(pc.model * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(pc.model))) * inNormal;
    fragColor = pc.color.rgb * inColor;
}
)";

// Lit fragment shader GLSL
static const std::string kFragSrc = R"(
#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragWorldPos;
layout(location = 0) out vec4 outColor;
void main() {
    vec3 lightDir = normalize(vec3(1.0, 1.0, 0.5));
    vec3 normal = normalize(fragNormal);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 ambient = 0.15 * fragColor;
    vec3 diffuse = diff * fragColor;
    outColor = vec4(ambient + diffuse, 1.0);
}
)";

// Push constant structure matching the shader
struct PushConstantData {
    glm::mat4 mvp;
    glm::mat4 model;
    glm::vec4 color; // rgb = albedo tint, a = unused
};

// -------------------------------------------------------------------
// Shader compilation helper
// -------------------------------------------------------------------
#if defined(HAS_SHADERC) && HAS_SHADERC
static std::vector<uint32_t> compileGLSL(const std::string& src,
                                          shaderc_shader_kind kind,
                                          const char* name)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = compiler.CompileGlslToSpv(src, kind, name, options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error(std::string("Shader compile error: ") + result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
}
#endif

// -------------------------------------------------------------------
// Scene implementation
// -------------------------------------------------------------------

Scene::Scene(std::shared_ptr<Engine> engine) {
    _engine = engine;

    _engine->onWindowResizeObservable.add([this](int width, int height) {
        if (this->_activeCamera)
            this->_activeCamera->setAspectRatio((float) width / (float) height);
    });

    // Set VulkanBase pointer for Mesh GPU buffer management
    Mesh::SetVulkanBase(&_engine->vulkanBase());
}

Scene::~Scene() {
    cleanupVulkanPipeline();
}

void Scene::initVulkanPipeline() {
    if (_pipelineReady) return;

    auto& vk = _engine->vulkanBase();
    VkDevice device = vk.GetDevice();

    // Compile shaders
#if defined(HAS_SHADERC) && HAS_SHADERC
    auto vertSpirv = compileGLSL(kVertSrc, shaderc_glsl_vertex_shader, "lit.vert");
    auto fragSpirv = compileGLSL(kFragSrc, shaderc_glsl_fragment_shader, "lit.frag");
    _vertShaderModule = vk.CreateShaderModule(vertSpirv);
    _fragShaderModule = vk.CreateShaderModule(fragSpirv);
#else
    // Without shaderc we cannot compile at runtime.
    // Use the basic embedded shaders from EmbeddedShaders.hpp via VulkanBase
    // For now, fail with a clear message.
    throw std::runtime_error("Runtime GLSL compilation requires shaderc. "
                             "Install the Vulkan SDK with shaderc support.");
#endif

    // Push constant range
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(PushConstantData);

    _pipelineLayout = vk.CreatePipelineLayout({}, {pcRange});

    // Vertex input: interleaved [pos(3) + normal(3) + color(3)] = 36 bytes
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = 9 * sizeof(float);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attribs{};
    // Position
    attribs[0].binding = 0;
    attribs[0].location = 0;
    attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribs[0].offset = 0;
    // Normal
    attribs[1].binding = 0;
    attribs[1].location = 1;
    attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribs[1].offset = 3 * sizeof(float);
    // Color
    attribs[2].binding = 0;
    attribs[2].location = 2;
    attribs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attribs[2].offset = 6 * sizeof(float);

    // Shader stages
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX;
    vertStage.module = _vertShaderModule;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT;
    fragStage.module = _fragShaderModule;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

    // Vertex input state
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vertexInputInfo.pVertexAttributeDescriptions = attribs.data();

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // Viewport / scissor (dynamic)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer for solid + back-face culling
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    // Color blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // Dynamic state
    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Create the main pipeline (solid, back-face culling)
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = vk.GetRenderPass();
    pipelineInfo.subpass = 0;

    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_pipeline);

    // Wireframe pipeline (back-face culling)
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_wireframePipeline);

    // No-cull solid pipeline
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_noCullPipeline);

    // No-cull wireframe pipeline
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_wireframeNoCullPipeline);

    // Line pipeline for line primitives
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_linePipeline);

    _pipelineReady = true;
}

void Scene::cleanupVulkanPipeline() {
    if (!_pipelineReady) return;
    VkDevice device = _engine->vulkanBase().GetDevice();
    vkDeviceWaitIdle(device);

    auto destroyPipeline = [&](VkPipeline& p) {
        if (p != VK_NULL_HANDLE) { vkDestroyPipeline(device, p, nullptr); p = VK_NULL_HANDLE; }
    };
    destroyPipeline(_pipeline);
    destroyPipeline(_wireframePipeline);
    destroyPipeline(_noCullPipeline);
    destroyPipeline(_wireframeNoCullPipeline);
    destroyPipeline(_linePipeline);

    if (_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
        _pipelineLayout = VK_NULL_HANDLE;
    }
    if (_vertShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, _vertShaderModule, nullptr);
        _vertShaderModule = VK_NULL_HANDLE;
    }
    if (_fragShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(device, _fragShaderModule, nullptr);
        _fragShaderModule = VK_NULL_HANDLE;
    }
    _pipelineReady = false;
}

void Scene::addMesh(std::shared_ptr<Mesh> mesh) {
    _meshes.push_back(mesh);
}

void Scene::removeMesh(std::shared_ptr<Mesh> mesh) {
    _meshes.erase(std::remove(_meshes.begin(), _meshes.end(), mesh), _meshes.end());
}

void Scene::render() {
    onBeforeRenderObservable.notifyObservers();

    auto& vk = _engine->vulkanBase();

    // Lazy-initialize the Vulkan pipeline
    if (!_pipelineReady) {
        initVulkanPipeline();
    }

    VkCommandBuffer cmd = vk.GetCurrentCommandBuffer();
    glm::mat4 projView = _activeCamera->projectionViewMatrix();

    // Ensure all meshes have GPU buffers
    for (auto& mesh : _meshes) {
        if (!mesh->isEnabled()) continue;
        if (mesh->isDirty() || !mesh->hasGPUBuffers()) {
            mesh->sendVertexDataToGPU();
        }
    }

    // Draw all meshes
    for (auto& mesh : _meshes) {
        if (!mesh->isEnabled()) continue;
        if (!mesh->hasGPUBuffers()) continue;

        const glm::mat4 world = mesh->transform()->computeWorldMatrix();
        mesh->aabb()->updateWithVertexData(mesh->vertexData(), world);

        // Select pipeline based on material properties
        bool isWireframe = mesh->material() && mesh->material()->wireframe();
        bool isNoCull = mesh->material() && !mesh->material()->isBackFaceCullingEnabled();
        bool isLine = (mesh->vertexData().positions.size() == 6); // 2 vertices = line

        VkPipeline selectedPipeline;
        if (isLine) {
            selectedPipeline = _linePipeline;
        } else if (isWireframe && isNoCull) {
            selectedPipeline = _wireframeNoCullPipeline;
        } else if (isWireframe) {
            selectedPipeline = _wireframePipeline;
        } else if (isNoCull) {
            selectedPipeline = _noCullPipeline;
        } else {
            selectedPipeline = _pipeline;
        }

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, selectedPipeline);

        // Push constants: MVP + model + color
        PushConstantData pc{};
        pc.mvp = projView * world;
        pc.model = world;
        pc.color = glm::vec4(mesh->material()->albedoColor(), 1.0f);

        vkCmdPushConstants(cmd, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

        // Bind vertex/index buffers
        VkBuffer vertexBuffers[] = {mesh->vertexBuffer().Buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(cmd, mesh->indexBuffer().Buffer, 0, VK_INDEX_TYPE_UINT32);

        // Draw
        vkCmdDrawIndexed(cmd, mesh->indexCount(), 1, 0, 0, 0);
    }

    // Render ImGui
    vk.ImGuiNewFrame();
    onRenderGuiObservable.notifyObservers();
    vk.ImGuiRender(cmd);

    onAfterRenderObservable.notifyObservers();
}

void Scene::setActiveCamera(std::shared_ptr<Camera> camera) {
    _activeCamera = camera;
}

void Scene::addDirectionalLight(std::shared_ptr<DirectionalLight> pLight) {
    _directionalLights.push_back(pLight);
}
