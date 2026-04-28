// main3D.cpp -- 3D Cloth + Balls demo, Vulkan rendering via VulkanBase
#include <VulkanBase.hpp>
#include <EmbeddedShaders.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/scene/SceneManager3D.hpp"
#include "engine/objects/Ball3D.hpp"
#include "engine/physics/ClothSolver3D.hpp"
#include "engine/core/Logger.hpp"
#include "engine/core/Time.hpp"

#include <memory>
#include <vector>
#include <cstring>
#include <iostream>
#include <chrono>
#include <cmath>

#ifdef HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

using namespace engine;
using namespace engine::core::log;

const unsigned int SCR_WIDTH  = 1280;
const unsigned int SCR_HEIGHT = 720;

// ---------- Shader compilation ----------
#ifdef HAS_SHADERC
static std::vector<uint32_t> CompileGLSL(const std::string& source,
                                          shaderc_shader_kind kind,
                                          const char* name) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    auto result = compiler.CompileGlslToSpv(source, kind, name, options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader compile error: " << result.GetErrorMessage() << std::endl;
        std::abort();
    }
    return {result.cbegin(), result.cend()};
}
#endif

// ---------- Push constants ----------
struct PushConstants3D {
    glm::mat4 mvp;
};

// ---------- Vertex structure for cloth (position + color) ----------
struct VertexPC {
    glm::vec3 position;
    glm::vec3 color;
};

// ---------- Pipeline creation ----------
static VkPipeline CreateFillPipeline(vkdemo::VulkanBase& vk,
                                      VkShaderModule vertModule,
                                      VkShaderModule fragModule,
                                      VkPipelineLayout layout) {
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(VertexPC);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0; attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[0].offset = offsetof(VertexPC, position);
    attrs[1].location = 1; attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[1].offset = offsetof(VertexPC, color);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vp{};
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.viewportCount = 1; vp.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{};
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = 0xF;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments = &blendAttach;

    VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = 2;
    dynState.pDynamicStates = dynStates;

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount = 2; ci.pStages = stages;
    ci.pVertexInputState = &vertexInput;
    ci.pInputAssemblyState = &ia;
    ci.pViewportState = &vp;
    ci.pRasterizationState = &raster;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &ds;
    ci.pColorBlendState = &blend;
    ci.pDynamicState = &dynState;
    ci.layout = layout;
    ci.renderPass = vk.GetRenderPass();
    ci.subpass = 0;

    VkPipeline pipeline;
    vkCreateGraphicsPipeline(vk.GetDevice(), VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);
    return pipeline;
}

// ---------- Generate sphere vertices ----------
static std::vector<VertexPC> GenerateSphere(const glm::vec3& center, float radius, const glm::vec3& color, int stacks = 12, int slices = 16) {
    std::vector<VertexPC> verts;

    for (int i = 0; i < stacks; ++i) {
        float phi0 = glm::pi<float>() * float(i) / stacks - glm::half_pi<float>();
        float phi1 = glm::pi<float>() * float(i + 1) / stacks - glm::half_pi<float>();

        for (int j = 0; j < slices; ++j) {
            float theta0 = 2.0f * glm::pi<float>() * float(j) / slices;
            float theta1 = 2.0f * glm::pi<float>() * float(j + 1) / slices;

            auto P = [&](float phi, float theta) -> glm::vec3 {
                return center + radius * glm::vec3(
                    std::cos(phi) * std::cos(theta),
                    std::sin(phi),
                    std::cos(phi) * std::sin(theta));
            };

            glm::vec3 p00 = P(phi0, theta0);
            glm::vec3 p10 = P(phi1, theta0);
            glm::vec3 p01 = P(phi0, theta1);
            glm::vec3 p11 = P(phi1, theta1);

            // Shade based on normal for simple lighting
            auto shade = [&](const glm::vec3& p) -> glm::vec3 {
                glm::vec3 n = glm::normalize(p - center);
                float d = glm::max(glm::dot(n, glm::normalize(glm::vec3(1, 1, 0.5f))), 0.0f);
                return color * (0.2f + 0.8f * d);
            };

            verts.push_back({p00, shade(p00)});
            verts.push_back({p10, shade(p10)});
            verts.push_back({p01, shade(p01)});

            verts.push_back({p01, shade(p01)});
            verts.push_back({p10, shade(p10)});
            verts.push_back({p11, shade(p11)});
        }
    }
    return verts;
}

int main() {
    Logger::log("Starting 3D Engine (Vulkan)...", LogLevel::Info);

    // ---- VulkanBase init ----
    vkdemo::VulkanBase vk;
    vkdemo::AppConfig config;
    config.Title = "3D Cloth + Balls (Vulkan)";
    config.Width = SCR_WIDTH;
    config.Height = SCR_HEIGHT;
    config.EnableImGui = false;
    config.EnableValidation = true;
    vk.Init(config);

    // ---- Compile shaders ----
#ifdef HAS_SHADERC
    auto vertSpv = CompileGLSL(vkdemo::kBasicVertexShader, shaderc_vertex_shader, "basic.vert");
    auto fragSpv = CompileGLSL(vkdemo::kBasicFragmentShader, shaderc_fragment_shader, "basic.frag");
#else
    std::cerr << "shaderc not available\n";
    return 1;
#endif

    VkShaderModule vertModule = vk.CreateShaderModule(vertSpv);
    VkShaderModule fragModule = vk.CreateShaderModule(fragSpv);

    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants3D);
    VkPipelineLayout pipelineLayout = vk.CreatePipelineLayout({}, {pushRange});

    VkPipeline fillPipeline = CreateFillPipeline(vk, vertModule, fragModule, pipelineLayout);

    // ---- Dynamic vertex buffers ----
    const VkDeviceSize maxBufSize = 4 * 1024 * 1024; // 4MB
    std::vector<vkdemo::GPUBuffer> vertBufs(2);
    for (int i = 0; i < 2; ++i) {
        vertBufs[i] = vk.CreateBuffer(maxBufSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // ---- Create scene ----
    auto sceneManager = std::make_shared<scene::SceneManager3D>(
        float(SCR_WIDTH) / float(SCR_HEIGHT));
    Logger::log("SceneManager3D created.", LogLevel::Info);

    auto phys = sceneManager->getPhysicsWorld();

    // Add extra cloth
    {
        auto extraCloth = std::make_shared<physics::ClothSolver3D>(
            *phys, 20, 15, 0.2f);
        extraCloth->createCloth(
            glm::vec3(1.0f, 2.5f, 0.0f),
            glm::vec3(1, 0, 0),
            glm::vec3(0, -1, 0));
        phys->addCloth(extraCloth);
    }

    // Add extra balls
    {
        auto b1 = std::make_shared<objects::Ball3D>(glm::vec3(-1.0f, 4.0f, 0.0f), 0.3f, 1.0f);
        b1->setVelocity(glm::vec3(2.0f, 0.0f, -1.0f));
        phys->addBall(b1);

        auto b2 = std::make_shared<objects::Ball3D>(glm::vec3(1.0f, 4.0f, 0.0f), 0.4f, 2.0f);
        b2->applyForce(glm::vec3(0.0f, -9.81f * b2->getMass(), 0.0f));
        phys->addBall(b2);
    }

    // ---- Input callback ----
    vk.OnKey = [&](GLFWwindow* w, int key, int, int action, int) {
        if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(w, 1);
    };

    vk.OnResize = [&](int, int) {
        vkDestroyPipeline(vk.GetDevice(), fillPipeline, nullptr);
        fillPipeline = CreateFillPipeline(vk, vertModule, fragModule, pipelineLayout);
    };

    // ---- Main loop ----
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        if (!vk.BeginFrame()) break;

        // Step simulation
        sceneManager->update(dt);

        auto currentScene = sceneManager->getCurrentScene();

        // Build MVP
        glm::mat4 view = currentScene->camera->getViewMatrix();
        glm::mat4 proj = currentScene->camera->getProjectionMatrix();
        // Vulkan clip-space fix: flip Y
        proj[1][1] *= -1.0f;
        glm::mat4 mvp = proj * view;

        PushConstants3D pc;
        pc.mvp = mvp;

        VkCommandBuffer cmd = vk.GetCurrentCommandBuffer();
        uint32_t frameIdx = vk.GetFrameIndex();

        // ---- Build cloth geometry ----
        std::vector<VertexPC> allVerts;

        // Cloth triangles with simple shading
        const auto& clothVerts = currentScene->clothVertices;
        const auto& clothIdx = currentScene->clothIndices;
        glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 0.5f));
        glm::vec3 clothBaseColor = currentScene->clothMaterial.diffuse;

        for (size_t i = 0; i + 2 < clothIdx.size(); i += 3) {
            for (int k = 0; k < 3; ++k) {
                uint32_t idx = clothIdx[i + k];
                if (idx < clothVerts.size()) {
                    glm::vec3 n = clothVerts[idx].normal;
                    float d = glm::max(glm::dot(n, lightDir), 0.0f);
                    glm::vec3 col = clothBaseColor * (0.2f + 0.8f * d);
                    allVerts.push_back({clothVerts[idx].position, col});
                }
            }
        }

        // Balls
        for (auto& ball : phys->getBalls()) {
            auto sphereVerts = GenerateSphere(
                ball->getPosition(), ball->getRadius(),
                glm::vec3(0.9f, 0.3f, 0.2f));
            allVerts.insert(allVerts.end(), sphereVerts.begin(), sphereVerts.end());
        }

        // ---- Draw ----
        if (!allVerts.empty()) {
            VkDeviceSize dataSize = allVerts.size() * sizeof(VertexPC);
            if (dataSize <= maxBufSize) {
                vk.CopyToBuffer(vertBufs[frameIdx], allVerts.data(), dataSize);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, fillPipeline);
                vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &vertBufs[frameIdx].Buffer, &offset);
                vkCmdDraw(cmd, static_cast<uint32_t>(allVerts.size()), 1, 0, 0);
            }
        }

        vk.EndFrame();
    }

    // ---- Cleanup ----
    vkDeviceWaitIdle(vk.GetDevice());

    for (int i = 0; i < 2; ++i)
        vk.DestroyBuffer(vertBufs[i]);
    vkDestroyPipeline(vk.GetDevice(), fillPipeline, nullptr);
    vkDestroyPipelineLayout(vk.GetDevice(), pipelineLayout, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), vertModule, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), fragModule, nullptr);

    return 0;
}
