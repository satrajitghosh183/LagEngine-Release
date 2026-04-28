#include "DemoScene.h"
#include "Shader.h"
#include "Mesh.h"
#include "Framebuffer.h"
#include "ShadowMap.h"
#include "GBuffer.h"
#include "SSAO.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include "VulkanBase.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <algorithm>
#include <array>

DemoScene::DemoScene(vkdemo::VulkanBase* vkBase, int width, int height)
    : m_vkBase(vkBase)
    , m_width(width)
    , m_height(height)
    , m_rotationAngle(0.0f)
    , m_lightRotation(0.0f)
{
    m_camera.setAspect((float)width / (float)height);
}

DemoScene::~DemoScene() {
    cleanupPipelines();
}

void DemoScene::initialize() {
    createRenderGraph();
    setupMeshes();
    setupShaders();

    m_shadowMap = std::make_shared<ShadowMap>();
    m_shadowMap->init(m_vkBase, 2048, 2048);

    m_gbuffer = std::make_shared<GBuffer>();
    m_gbuffer->init(m_vkBase, m_width, m_height);

    m_ssao = std::make_shared<SSAO>();
    m_ssao->init(m_vkBase, m_width, m_height);

    m_particles = std::make_shared<ParticleSystem>(50000);
    m_particles->init(m_vkBase);

    m_outputFBO = std::make_shared<Framebuffer>();
    m_outputFBO->init(m_vkBase, m_width, m_height);

    createPipelines();
    createDescriptors();
    updateDescriptors();
}

void DemoScene::createRenderGraph() {
    m_renderGraph = std::make_shared<RenderGraph>();

    // Create tasks with executor types for three-tier parallelization
    // Vulkan tasks: Main thread only (ShadowPass, GBufferPass, LightingPass, PostFXPass)
    // CUDA tasks: Parallel streams (CudaSSAO, CudaParticles)
    // CPU tasks: Worker threads (FrustumCulling, MeshUpdate, PhysicsUpdate) - enables work stealing!
    auto shadowTask = m_renderGraph->addTask(TaskType::ShadowPass, "ShadowPass", 2.0f, TaskExecutor::OpenGL_MainThread);
    auto gbufferTask = m_renderGraph->addTask(TaskType::GBufferPass, "GBufferPass", 5.0f, TaskExecutor::OpenGL_MainThread);
    auto ssaoTask = m_renderGraph->addTask(TaskType::CudaSSAO, "CudaSSAO", 1.5f, TaskExecutor::CUDA_Stream);
    auto particleTask = m_renderGraph->addTask(TaskType::CudaParticles, "CudaParticles", 8.0f, TaskExecutor::CUDA_Stream);
    auto lightingTask = m_renderGraph->addTask(TaskType::LightingPass, "LightingPass", 3.0f, TaskExecutor::OpenGL_MainThread);
    auto postfxTask = m_renderGraph->addTask(TaskType::PostFXPass, "PostFXPass", 1.0f, TaskExecutor::OpenGL_MainThread);

    // CPU tasks for work stealing - these run in parallel on worker threads
    auto cullingTask = m_renderGraph->addTask(TaskType::FrustumCulling, "FrustumCulling", 3.0f, TaskExecutor::CPU_Worker);
    auto meshUpdateTask = m_renderGraph->addTask(TaskType::MeshUpdate, "MeshUpdate", 2.5f, TaskExecutor::CPU_Worker);
    auto physicsTask = m_renderGraph->addTask(TaskType::PhysicsUpdate, "PhysicsUpdate", 4.0f, TaskExecutor::CPU_Worker);

    // Set execute functions
    // In the Vulkan port the shadow, gbuffer, lighting, and postfx passes are recorded
    // as Vulkan commands externally (recordXxxPass).  The scheduler tasks run placeholders
    // or CPU/CUDA work only; actual GPU draw recording is done separately.
    shadowTask->setExecuteFunc([this]() {
        // Shadow pass recorded externally via recordShadowPass(cmd)
    });
    gbufferTask->setExecuteFunc([this]() {
        // GBuffer pass recorded externally via recordGBufferPass(cmd)
    });
    ssaoTask->setExecuteFunc([this]() {
        // CUDA SSAO processing - placeholder
    });
    particleTask->setExecuteFunc([this, particleTask]() {
        if (m_particles && m_deltaTime > 0.0f && m_deltaTime < 1.0f) {
            void* streamPtr = particleTask->getCudaStreamPtr();
            m_particles->update(m_deltaTime, m_camera.getViewMatrix(), streamPtr);
        }
    });
    lightingTask->setExecuteFunc([this]() {
        // Lighting pass recorded externally via recordLightingPass(cmd)
    });
    postfxTask->setExecuteFunc([this]() {
        // PostFX pass recorded externally via recordPostFXPass(cmd)
    });

    // CPU task execute functions
    cullingTask->setExecuteFunc([this]() { this->performFrustumCulling(); });
    meshUpdateTask->setExecuteFunc([this]() { this->updateMeshes(); });
    physicsTask->setExecuteFunc([this]() { this->updatePhysics(); });

    // Set state descriptors for work stealing cost heuristics
    GLState shadowState;
    shadowState.shaderID = m_shadowShader ? m_shadowShader->getID() : 0;
    shadowState.fboID = m_shadowMap ? m_shadowMap->getFBO() : 0;
    shadowTask->setGLState(shadowState);

    GLState gbufferState;
    gbufferState.shaderID = m_gbufferShader ? m_gbufferShader->getID() : 0;
    gbufferState.fboID = m_gbuffer ? m_gbuffer->getFBO() : 0;
    gbufferTask->setGLState(gbufferState);

    // Build dependencies
    // CPU tasks can run in parallel with early passes
    m_renderGraph->addDependency(cullingTask, gbufferTask);
    m_renderGraph->addDependency(meshUpdateTask, gbufferTask);
    m_renderGraph->addDependency(physicsTask, gbufferTask);

    // Rendering dependencies
    m_renderGraph->addDependency(shadowTask, lightingTask);
    m_renderGraph->addDependency(gbufferTask, lightingTask);
    m_renderGraph->addDependency(gbufferTask, ssaoTask);
    m_renderGraph->addDependency(lightingTask, postfxTask);
    m_renderGraph->addDependency(ssaoTask, postfxTask);
    m_renderGraph->addDependency(particleTask, postfxTask);

    // Build graph
    if (!m_renderGraph->build()) {
        std::cerr << "Failed to build render graph" << std::endl;
    }
}

void DemoScene::setupMeshes() {
    auto cubeData = createCubeMeshData();
    m_cubeMesh = std::make_shared<Mesh>();
    m_cubeMesh->init(m_vkBase, cubeData.vertices, cubeData.indices);

    auto sphereData = createSphereMeshData(32);
    m_sphereMesh = std::make_shared<Mesh>();
    m_sphereMesh->init(m_vkBase, sphereData.vertices, sphereData.indices);

    auto quadData = createQuadMeshData();
    m_quadMesh = std::make_shared<Mesh>();
    m_quadMesh->init(m_vkBase, quadData.vertices, quadData.indices);
}

void DemoScene::setupShaders() {
    VkDevice device = m_vkBase->GetDevice();

    // Shadow shader
    m_shadowShader = std::make_shared<Shader>();
    m_shadowShader->setDevice(device);
    if (!m_shadowShader->loadFromFiles("shaders/shadow.vert", "shaders/shadow.frag")) {
        std::cerr << "ERROR: Failed to load shadow shader" << std::endl;
    } else {
        std::cout << "Shadow shader loaded successfully" << std::endl;
    }

    // G-buffer shader
    m_gbufferShader = std::make_shared<Shader>();
    m_gbufferShader->setDevice(device);
    if (!m_gbufferShader->loadFromFiles("shaders/gbuffer.vert", "shaders/gbuffer.frag")) {
        std::cerr << "ERROR: Failed to load G-buffer shader" << std::endl;
    } else {
        std::cout << "G-buffer shader loaded successfully" << std::endl;
    }

    // Lighting shader
    m_lightingShader = std::make_shared<Shader>();
    m_lightingShader->setDevice(device);
    if (!m_lightingShader->loadFromFiles("shaders/lighting.vert", "shaders/lighting.frag")) {
        std::cerr << "ERROR: Failed to load lighting shader" << std::endl;
    } else {
        std::cout << "Lighting shader loaded successfully" << std::endl;
    }

    // PostFX shader
    m_postFXShader = std::make_shared<Shader>();
    m_postFXShader->setDevice(device);
    if (!m_postFXShader->loadFromFiles("shaders/postfx.vert", "shaders/postfx.frag")) {
        std::cerr << "ERROR: Failed to load post-FX shader" << std::endl;
    } else {
        std::cout << "PostFX shader loaded successfully" << std::endl;
    }

    // Particle shader
    m_particleShader = std::make_shared<Shader>();
    m_particleShader->setDevice(device);
    if (!m_particleShader->loadFromFiles("shaders/particle.vert", "shaders/particle.frag")) {
        std::cerr << "ERROR: Failed to load particle shader" << std::endl;
    } else {
        std::cout << "Particle shader loaded successfully" << std::endl;
    }
}

// Helper to create a graphics pipeline for a given render pass + shader pair
static VkPipeline createGraphicsPipeline(
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    VkShaderModule vertModule,
    VkShaderModule fragModule,
    uint32_t colorAttachmentCount,
    bool depthOnly,
    VkPrimitiveTopology topology,
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attribs,
    bool enableDepthTest,
    bool enableDepthWrite)
{
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT;
    stages[1].module = fragModule;
    stages[1].pName = "main";
    uint32_t stageCount = (fragModule != VK_NULL_HANDLE) ? 2 : 1;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribs.size());
    vertexInput.pVertexAttributeDescriptions = attribs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = topology;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.lineWidth = 1.0f;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = enableDepthTest ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = enableDepthWrite ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(colorAttachmentCount);
    for (auto& att : blendAttachments) {
        att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        att.blendEnable = VK_FALSE;
    }

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = colorAttachmentCount;
    colorBlend.pAttachments = blendAttachments.data();

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = stageCount;
    pci.pStages = stages;
    pci.pVertexInputState = &vertexInput;
    pci.pInputAssemblyState = &inputAssembly;
    pci.pViewportState = &viewportState;
    pci.pRasterizationState = &raster;
    pci.pMultisampleState = &multisample;
    pci.pDepthStencilState = &depthStencil;
    pci.pColorBlendState = depthOnly ? nullptr : &colorBlend;
    pci.pDynamicState = &dynamicState;
    pci.layout = layout;
    pci.renderPass = renderPass;
    pci.subpass = 0;

    VkPipeline pipeline = VK_NULL_HANDLE;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS) {
        std::cerr << "Failed to create graphics pipeline" << std::endl;
    }
    return pipeline;
}

// Vertex input descriptions for Vertex struct (pos, normal, texcoord)
static std::vector<VkVertexInputBindingDescription> getVertexBindings() {
    return {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
}

static std::vector<VkVertexInputAttributeDescription> getVertexAttributes() {
    return {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, texCoords)},
    };
}

// Particle vertex input (x, y, z, w packed as vec4)
static std::vector<VkVertexInputBindingDescription> getParticleBindings() {
    return {{0, sizeof(float) * 4, VK_VERTEX_INPUT_RATE_VERTEX}};
}

static std::vector<VkVertexInputAttributeDescription> getParticleAttributes() {
    return {
        {0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
    };
}

void DemoScene::createPipelines() {
    VkDevice device = m_vkBase->GetDevice();

    // Push constant range: model + view + projection matrices (3 * mat4 = 192 bytes)
    VkPushConstantRange meshPC{};
    meshPC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    meshPC.offset = 0;
    meshPC.size = sizeof(glm::mat4) * 3; // model, view, proj

    // Shadow pipeline (depth-only, vertex-only shader)
    {
        VkPushConstantRange shadowPC{};
        shadowPC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        shadowPC.offset = 0;
        shadowPC.size = sizeof(glm::mat4) * 2; // lightSpaceMatrix + model

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &shadowPC;
        vkCreatePipelineLayout(device, &plci, nullptr, &m_shadowLayout);

        if (m_shadowShader && m_shadowShader->getVertexModule()) {
            m_shadowPipeline = createGraphicsPipeline(
                device, m_shadowMap->getRenderPass(), m_shadowLayout,
                m_shadowShader->getVertexModule(), m_shadowShader->getFragmentModule(),
                0, true, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                getVertexBindings(), getVertexAttributes(), true, true);
        }
    }

    // GBuffer pipeline (3 color attachments + depth)
    {
        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &meshPC;
        vkCreatePipelineLayout(device, &plci, nullptr, &m_gbufferLayout);

        if (m_gbufferShader && m_gbufferShader->getVertexModule()) {
            m_gbufferPipeline = createGraphicsPipeline(
                device, m_gbuffer->getRenderPass(), m_gbufferLayout,
                m_gbufferShader->getVertexModule(), m_gbufferShader->getFragmentModule(),
                3, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                getVertexBindings(), getVertexAttributes(), true, true);
        }
    }

    // Lighting pipeline (fullscreen quad, reads G-buffer textures via descriptors)
    {
        VkPushConstantRange lightPC{};
        lightPC.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        lightPC.offset = 0;
        lightPC.size = sizeof(glm::vec4) * 3 + sizeof(glm::mat4); // lightPos, viewPos, padding, lightSpaceMatrix

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &m_lightingDSLayout; // will be set after createDescriptors()
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &lightPC;
        // Defer actual creation until after descriptors are set up
    }

    // PostFX pipeline (fullscreen quad)
    {
        VkPushConstantRange postPC{};
        postPC.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        postPC.offset = 0;
        postPC.size = sizeof(float) * 4; // flags

        // Defer until after descriptors
    }

    // Particle pipeline (point rendering in swapchain render pass)
    {
        VkPushConstantRange particlePC{};
        particlePC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        particlePC.offset = 0;
        particlePC.size = sizeof(glm::mat4); // viewProj

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &particlePC;
        vkCreatePipelineLayout(device, &plci, nullptr, &m_particleLayout);

        if (m_particleShader && m_particleShader->getVertexModule()) {
            // Particle pipeline renders into the swapchain render pass
            auto particleRaster = VkPipelineRasterizationStateCreateInfo{};
            particleRaster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            particleRaster.polygonMode = VK_POLYGON_MODE_FILL;
            particleRaster.lineWidth = 1.0f;
            particleRaster.cullMode = VK_CULL_MODE_NONE;

            m_particlePipeline = createGraphicsPipeline(
                device, m_vkBase->GetRenderPass(), m_particleLayout,
                m_particleShader->getVertexModule(), m_particleShader->getFragmentModule(),
                1, false, VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
                getParticleBindings(), getParticleAttributes(), false, false);
        }
    }
}

void DemoScene::createDescriptors() {
    VkDevice device = m_vkBase->GetDevice();

    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10},
    };
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 4;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool);

    // Lighting descriptor set layout (4 samplers: position, normal, albedo, shadowMap)
    {
        VkDescriptorSetLayoutBinding bindings[4]{};
        for (int i = 0; i < 4; ++i) {
            bindings[i].binding = static_cast<uint32_t>(i);
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 4;
        layoutInfo.pBindings = bindings;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_lightingDSLayout);
    }

    // PostFX descriptor set layout (2 samplers: scene, ssao)
    {
        VkDescriptorSetLayoutBinding bindings[2]{};
        for (int i = 0; i < 2; ++i) {
            bindings[i].binding = static_cast<uint32_t>(i);
            bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 2;
        layoutInfo.pBindings = bindings;
        vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_postfxDSLayout);
    }

    // Allocate descriptor sets
    {
        VkDescriptorSetLayout layouts[] = {m_lightingDSLayout, m_postfxDSLayout};
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = 2;
        allocInfo.pSetLayouts = layouts;
        VkDescriptorSet sets[2]{};
        vkAllocateDescriptorSets(device, &allocInfo, sets);
        m_lightingDS = sets[0];
        m_postfxDS = sets[1];
    }

    // Now create the deferred pipeline layouts that use these descriptor set layouts

    // Lighting pipeline layout
    {
        VkPushConstantRange lightPC{};
        lightPC.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        lightPC.offset = 0;
        lightPC.size = sizeof(glm::vec4) * 3 + sizeof(glm::mat4);

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &m_lightingDSLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &lightPC;
        vkCreatePipelineLayout(device, &plci, nullptr, &m_lightingLayout);

        if (m_lightingShader && m_lightingShader->getVertexModule()) {
            m_lightingPipeline = createGraphicsPipeline(
                device, m_outputFBO->getRenderPass(), m_lightingLayout,
                m_lightingShader->getVertexModule(), m_lightingShader->getFragmentModule(),
                1, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                getVertexBindings(), getVertexAttributes(), false, false);
        }
    }

    // PostFX pipeline layout
    {
        VkPushConstantRange postPC{};
        postPC.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        postPC.offset = 0;
        postPC.size = sizeof(float) * 4;

        VkPipelineLayoutCreateInfo plci{};
        plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plci.setLayoutCount = 1;
        plci.pSetLayouts = &m_postfxDSLayout;
        plci.pushConstantRangeCount = 1;
        plci.pPushConstantRanges = &postPC;
        vkCreatePipelineLayout(device, &plci, nullptr, &m_postfxLayout);

        if (m_postFXShader && m_postFXShader->getVertexModule()) {
            m_postfxPipeline = createGraphicsPipeline(
                device, m_vkBase->GetRenderPass(), m_postfxLayout,
                m_postFXShader->getVertexModule(), m_postFXShader->getFragmentModule(),
                1, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                getVertexBindings(), getVertexAttributes(), false, false);
        }
    }
}

void DemoScene::updateDescriptors() {
    // Update lighting descriptor set with G-buffer + shadow map textures
    VkDescriptorImageInfo imageInfos[4]{};
    // Position
    imageInfos[0].sampler = m_gbuffer->getSampler();
    imageInfos[0].imageView = m_gbuffer->getPositionView();
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Normal
    imageInfos[1].sampler = m_gbuffer->getSampler();
    imageInfos[1].imageView = m_gbuffer->getNormalView();
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Albedo
    imageInfos[2].sampler = m_gbuffer->getSampler();
    imageInfos[2].imageView = m_gbuffer->getAlbedoView();
    imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // Shadow map
    imageInfos[3].sampler = m_shadowMap->getSampler();
    imageInfos[3].imageView = m_shadowMap->getDepthView();
    imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet writes[4]{};
    for (int i = 0; i < 4; ++i) {
        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[i].dstSet = m_lightingDS;
        writes[i].dstBinding = static_cast<uint32_t>(i);
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[i].pImageInfo = &imageInfos[i];
    }
    vkUpdateDescriptorSets(m_vkBase->GetDevice(), 4, writes, 0, nullptr);

    // Update postfx descriptor set
    VkDescriptorImageInfo postfxImageInfos[2]{};
    // Scene (output FBO color)
    postfxImageInfos[0].sampler = m_outputFBO->getSampler();
    postfxImageInfos[0].imageView = m_outputFBO->getColorView();
    postfxImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    // SSAO result
    postfxImageInfos[1].sampler = m_ssao->getSampler();
    postfxImageInfos[1].imageView = m_ssao->getResultView();
    postfxImageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkWriteDescriptorSet postWrites[2]{};
    for (int i = 0; i < 2; ++i) {
        postWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        postWrites[i].dstSet = m_postfxDS;
        postWrites[i].dstBinding = static_cast<uint32_t>(i);
        postWrites[i].descriptorCount = 1;
        postWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        postWrites[i].pImageInfo = &postfxImageInfos[i];
    }
    vkUpdateDescriptorSets(m_vkBase->GetDevice(), 2, postWrites, 0, nullptr);
}

void DemoScene::cleanupPipelines() {
    if (!m_vkBase) return;
    VkDevice device = m_vkBase->GetDevice();
    if (device == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(device);

    auto destroyPL = [&](VkPipeline& p, VkPipelineLayout& l) {
        if (p) { vkDestroyPipeline(device, p, nullptr); p = VK_NULL_HANDLE; }
        if (l) { vkDestroyPipelineLayout(device, l, nullptr); l = VK_NULL_HANDLE; }
    };

    destroyPL(m_shadowPipeline, m_shadowLayout);
    destroyPL(m_gbufferPipeline, m_gbufferLayout);
    destroyPL(m_lightingPipeline, m_lightingLayout);
    destroyPL(m_postfxPipeline, m_postfxLayout);
    destroyPL(m_particlePipeline, m_particleLayout);

    if (m_lightingDSLayout) { vkDestroyDescriptorSetLayout(device, m_lightingDSLayout, nullptr); m_lightingDSLayout = VK_NULL_HANDLE; }
    if (m_postfxDSLayout)   { vkDestroyDescriptorSetLayout(device, m_postfxDSLayout, nullptr); m_postfxDSLayout = VK_NULL_HANDLE; }
    if (m_descriptorPool)   { vkDestroyDescriptorPool(device, m_descriptorPool, nullptr); m_descriptorPool = VK_NULL_HANDLE; }
}

void DemoScene::resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_camera.setAspect((float)width / (float)height);

    if (m_gbuffer) {
        m_gbuffer->resize(width, height);
    }
    if (m_ssao) {
        m_ssao->resize(width, height);
    }
    if (m_outputFBO) {
        m_outputFBO->resize(width, height);
    }

    // Recreate descriptors after resize since image views change
    updateDescriptors();
}

void DemoScene::update(float deltaTime) {
    deltaTime = std::max(0.0f, std::min(deltaTime, 0.1f));
    m_deltaTime = deltaTime;
    m_rotationAngle += deltaTime * 30.0f;
    m_lightRotation += deltaTime * 20.0f;
    m_camera.update(deltaTime);
}

// --------------- Vulkan command recording for each pass ---------------

void DemoScene::recordShadowPass(VkCommandBuffer cmd) {
    if (!m_shadowPipeline || !m_shadowMap || !m_cubeMesh) return;

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_shadowMap->getRenderPass();
    rpInfo.framebuffer = m_shadowMap->getFramebuffer();
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {static_cast<uint32_t>(m_shadowMap->getWidth()),
                                 static_cast<uint32_t>(m_shadowMap->getHeight())};
    VkClearValue clearDepth{};
    clearDepth.depthStencil = {1.0f, 0};
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearDepth;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shadowPipeline);

    VkViewport viewport{0, 0, (float)m_shadowMap->getWidth(), (float)m_shadowMap->getHeight(), 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0,0}, {(uint32_t)m_shadowMap->getWidth(), (uint32_t)m_shadowMap->getHeight()}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    glm::vec3 lightPos = glm::vec3(
        cos(glm::radians(m_lightRotation)) * 10.0f,
        10.0f,
        sin(glm::radians(m_lightRotation)) * 10.0f);
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - lightPos);
    glm::mat4 lightSpaceMatrix = m_shadowMap->getLightSpaceMatrix(lightPos, lightDir, 20.0f);

    // Floor
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));

    struct { glm::mat4 lightSpace; glm::mat4 model; } shadowPush;
    shadowPush.lightSpace = lightSpaceMatrix;
    shadowPush.model = model;
    vkCmdPushConstants(cmd, m_shadowLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadowPush), &shadowPush);
    m_cubeMesh->render(cmd);

    // Rotating cubes
    for (int i = 0; i < 10; ++i) {
        float angle = m_rotationAngle + i * 36.0f;
        float radius = 3.0f + (i % 3) * 2.0f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cos(glm::radians(angle)) * radius, 2.0f + (i % 2) * 2.0f, sin(glm::radians(angle)) * radius));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f + (i % 3) * 0.2f));
        shadowPush.model = model;
        vkCmdPushConstants(cmd, m_shadowLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(shadowPush), &shadowPush);
        m_cubeMesh->render(cmd);
    }

    vkCmdEndRenderPass(cmd);
}

void DemoScene::recordGBufferPass(VkCommandBuffer cmd) {
    if (!m_gbufferPipeline || !m_gbuffer) return;

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_gbuffer->getRenderPass();
    rpInfo.framebuffer = m_gbuffer->getFramebuffer();
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {static_cast<uint32_t>(m_gbuffer->getWidth()),
                                 static_cast<uint32_t>(m_gbuffer->getHeight())};
    VkClearValue clearValues[4]{};
    clearValues[0].color = {{0, 0, 0, 0}};
    clearValues[1].color = {{0, 0, 0, 0}};
    clearValues[2].color = {{0, 0, 0, 0}};
    clearValues[3].depthStencil = {1.0f, 0};
    rpInfo.clearValueCount = 4;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_gbufferPipeline);

    VkViewport viewport{0, 0, (float)m_width, (float)m_height, 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0,0}, {(uint32_t)m_width, (uint32_t)m_height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 proj = m_camera.getProjectionMatrix();

    struct { glm::mat4 model; glm::mat4 view; glm::mat4 proj; } gbufPush;
    gbufPush.view = view;
    gbufPush.proj = proj;

    // Floor
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(10.0f, 0.5f, 10.0f));
    gbufPush.model = model;
    vkCmdPushConstants(cmd, m_gbufferLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(gbufPush), &gbufPush);
    if (m_cubeMesh) m_cubeMesh->render(cmd);

    // Rotating cubes
    for (int i = 0; i < 10; ++i) {
        float angle = m_rotationAngle + i * 36.0f;
        float radius = 3.0f + (i % 3) * 2.0f;
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(cos(glm::radians(angle)) * radius, 2.0f + (i % 2) * 2.0f, sin(glm::radians(angle)) * radius));
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f + (i % 3) * 0.2f));
        gbufPush.model = model;
        vkCmdPushConstants(cmd, m_gbufferLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(gbufPush), &gbufPush);
        if (m_cubeMesh) m_cubeMesh->render(cmd);
    }

    // Central sphere
    model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
    gbufPush.model = model;
    vkCmdPushConstants(cmd, m_gbufferLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(gbufPush), &gbufPush);
    if (m_sphereMesh) m_sphereMesh->render(cmd);

    vkCmdEndRenderPass(cmd);
}

void DemoScene::recordLightingPass(VkCommandBuffer cmd) {
    if (!m_lightingPipeline || !m_outputFBO) return;

    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = m_outputFBO->getRenderPass();
    rpInfo.framebuffer = m_outputFBO->getFramebuffer();
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = {static_cast<uint32_t>(m_outputFBO->getWidth()),
                                 static_cast<uint32_t>(m_outputFBO->getHeight())};
    VkClearValue clearValues[2]{};
    clearValues[0].color = {{0.1f, 0.1f, 0.15f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    rpInfo.clearValueCount = 2;
    rpInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingPipeline);

    VkViewport viewport{0, 0, (float)m_width, (float)m_height, 0, 1};
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{{0,0}, {(uint32_t)m_width, (uint32_t)m_height}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind G-buffer + shadow map descriptor set
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_lightingLayout,
                            0, 1, &m_lightingDS, 0, nullptr);

    // Push light data
    glm::vec3 lightPos = glm::vec3(
        cos(glm::radians(m_lightRotation)) * 10.0f, 10.0f,
        sin(glm::radians(m_lightRotation)) * 10.0f);
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.0f) - lightPos);
    glm::mat4 lightSpaceMatrix = m_shadowMap->getLightSpaceMatrix(lightPos, lightDir, 20.0f);

    struct {
        glm::vec4 lightPos;
        glm::vec4 viewPos;
        glm::vec4 padding;
        glm::mat4 lightSpaceMatrix;
    } lightPush;
    lightPush.lightPos = glm::vec4(lightPos, 1.0f);
    lightPush.viewPos = glm::vec4(m_camera.getPosition(), 1.0f);
    lightPush.padding = glm::vec4(0.0f);
    lightPush.lightSpaceMatrix = lightSpaceMatrix;
    vkCmdPushConstants(cmd, m_lightingLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(lightPush), &lightPush);

    // Render fullscreen quad
    if (m_quadMesh) m_quadMesh->render(cmd);

    vkCmdEndRenderPass(cmd);
}

void DemoScene::recordPostFXPass(VkCommandBuffer cmd) {
    // The postfx pass renders into the current render pass (swapchain),
    // which is already begun by the caller.

    if (m_postfxPipeline && m_quadMesh) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postfxPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_postfxLayout,
                                0, 1, &m_postfxDS, 0, nullptr);

        float flags[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        vkCmdPushConstants(cmd, m_postfxLayout, VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(flags), flags);

        m_quadMesh->render(cmd);
    }

    // Render particles on top
    if (m_particlePipeline && m_particles) {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipeline);

        glm::mat4 vp = m_camera.getViewProjectionMatrix();
        m_particles->render(cmd, m_particleLayout, vp);

        static int frameCount = 0;
        if (frameCount++ < 3) {
            glm::vec3 cameraPos = m_camera.getPosition();
            std::cout << "Rendering " << m_particles->getMaxParticles() << " particles" << std::endl;
            std::cout << "Camera pos: (" << cameraPos.x << ", " << cameraPos.y << ", " << cameraPos.z << ")" << std::endl;
        }
    }
}

// --------------- CPU tasks (unchanged) ---------------

void DemoScene::performFrustumCulling() {
    // Simulate frustum culling work - CPU intensive
    volatile int dummy = 0;
    for (int i = 0; i < 50000; ++i) {
        dummy += i * 2;
        dummy %= 1000;
    }
    (void)dummy;
}

void DemoScene::updateMeshes() {
    // Simulate mesh update work - CPU intensive
    volatile int dummy = 0;
    for (int i = 0; i < 40000; ++i) {
        dummy += i * 3;
        dummy %= 1000;
    }
    (void)dummy;
}

void DemoScene::updatePhysics() {
    // Simulate physics update work - CPU intensive
    volatile int dummy = 0;
    for (int i = 0; i < 60000; ++i) {
        dummy += i * 4;
        dummy %= 1000;
    }
    (void)dummy;
}

void DemoScene::updateParticles(float deltaTime, void* cudaStream) {
    if (m_particles && deltaTime > 0.0f && deltaTime < 1.0f) {
        m_particles->update(deltaTime, m_camera.getViewMatrix(), cudaStream);
    }
}
