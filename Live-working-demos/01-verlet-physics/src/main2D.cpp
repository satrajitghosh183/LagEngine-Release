// main2D.cpp -- 2D Verlet Cloth + Cannon demo, Vulkan rendering via VulkanBase
#include <VulkanBase.hpp>
#include <EmbeddedShaders.hpp>

#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <chrono>
#include <string>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "engine/scene/SceneManager2D.hpp"
#include "engine/objects/Ball2D.hpp"
#include "engine/objects/Cloth2D.hpp"
#include "engine/objects/Cannon2D.hpp"

#ifdef HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

using namespace engine;

// ---------- Shader compilation helpers ----------
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

// ---- helpers to build Vulkan pipeline ----
struct PushConstants2D {
    glm::mat4 mvp;
};

static VkPipeline CreateTrianglePipeline(vkdemo::VulkanBase& vk,
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
    binding.stride = sizeof(float) * 6; // pos3 + color3
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
    ci.stageCount = 2;
    ci.pStages = stages;
    ci.pVertexInputState = &vertexInput;
    ci.pInputAssemblyState = &inputAssembly;
    ci.pViewportState = &viewportState;
    ci.pRasterizationState = &raster;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &depth;
    ci.pColorBlendState = &blend;
    ci.pDynamicState = &dynState;
    ci.layout = layout;
    ci.renderPass = vk.GetRenderPass();
    ci.subpass = 0;

    VkPipeline pipeline;
    vkCreateGraphicsPipeline(vk.GetDevice(), VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);
    return pipeline;
}

static VkPipeline CreateLinePipeline(vkdemo::VulkanBase& vk,
                                      VkShaderModule vertModule,
                                      VkShaderModule fragModule,
                                      VkPipelineLayout layout) {
    // Same as triangle pipeline but with LINE_LIST topology
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
    binding.stride = sizeof(float) * 6;
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                 VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

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
    ci.stageCount = 2;
    ci.pStages = stages;
    ci.pVertexInputState = &vertexInput;
    ci.pInputAssemblyState = &inputAssembly;
    ci.pViewportState = &viewportState;
    ci.pRasterizationState = &raster;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &depth;
    ci.pColorBlendState = &blend;
    ci.pDynamicState = &dynState;
    ci.layout = layout;
    ci.renderPass = vk.GetRenderPass();
    ci.subpass = 0;

    VkPipeline pipeline;
    vkCreateGraphicsPipeline(vk.GetDevice(), VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline);
    return pipeline;
}

// ---------- Respawn cloth helper ----------
void respawnCloth(scene::Scene2D* scene, std::vector<objects::Ball2D*>* projectiles,
                  objects::Cloth2D** cloth, const glm::vec2& position = {300, 50}) {
    objects::Cloth2D* newCloth = new objects::Cloth2D(20, 15, 20.0f, position);
    newCloth->setProjectiles(projectiles);
    scene->add(newCloth);

    if (*cloth) {
        (*cloth)->active = false;
    }
    *cloth = newCloth;

    std::cout << "Cloth respawned at position (" << position.x << ", " << position.y << ")" << std::endl;
}

int main() {
    // ---- VulkanBase init ----
    vkdemo::VulkanBase vk;
    vkdemo::AppConfig config;
    config.Title = "2D Physics Simulation (Vulkan)";
    config.Width = 1280;
    config.Height = 720;
    config.EnableImGui = true;
    config.EnableValidation = true;
    vk.Init(config);

    // ---- Compile shaders ----
#ifdef HAS_SHADERC
    auto vertSpv = CompileGLSL(vkdemo::kBasicVertexShader, shaderc_vertex_shader, "basic.vert");
    auto fragSpv = CompileGLSL(vkdemo::kBasicFragmentShader, shaderc_fragment_shader, "basic.frag");
#else
    // Fallback: use pre-compiled SPIR-V (must be provided)
    std::cerr << "shaderc not available -- provide pre-compiled SPIR-V\n";
    return 1;
#endif

    VkShaderModule vertModule = vk.CreateShaderModule(vertSpv);
    VkShaderModule fragModule = vk.CreateShaderModule(fragSpv);

    // ---- Pipeline layout (push constant = MVP mat4) ----
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(PushConstants2D);

    VkPipelineLayout pipelineLayout = vk.CreatePipelineLayout({}, {pushRange});

    // ---- Create pipelines ----
    VkPipeline triPipeline = CreateTrianglePipeline(vk, vertModule, fragModule, pipelineLayout);
    VkPipeline linePipeline = CreateLinePipeline(vk, vertModule, fragModule, pipelineLayout);

    // ---- Allocate dynamic vertex buffers (host-visible for frequent updates) ----
    const VkDeviceSize maxVertBufSize = 256 * 1024; // 256KB
    std::vector<vkdemo::GPUBuffer> triBuffers(2), lineBuffers(2);
    for (int i = 0; i < 2; ++i) {
        triBuffers[i] = vk.CreateBuffer(maxVertBufSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        lineBuffers[i] = vk.CreateBuffer(maxVertBufSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // ---- Setup scene ----
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::mt19937 rng(std::rand());
    std::uniform_real_distribution<float> colorDist(0.4f, 1.0f);

    scene::SceneManager2D sceneManager;
    scene::Scene2D* scene = sceneManager.get2D();

    std::vector<objects::Ball2D*> projectiles;

    auto cloth = new objects::Cloth2D(20, 15, 20.0f, {300, 50});
    cloth->setProjectiles(&projectiles);

    auto cannon = new objects::Cannon2D({300, 650});

    scene->add(cloth);
    scene->add(cannon);

    bool spacePressed = false;
    bool upPressed = false;
    bool downPressed = false;
    bool rKeyPressed = false;
    int ballsFired = 0;

    // ---- Keyboard callback ----
    vk.OnKey = [&](GLFWwindow* w, int key, int, int action, int) {
        if (action == GLFW_PRESS) {
            if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
            if (key == GLFW_KEY_LEFT) cannon->rotate(-5.0f);
            if (key == GLFW_KEY_RIGHT) cannon->rotate(5.0f);
            if (key == GLFW_KEY_UP && !upPressed) { upPressed = true; cannon->adjustPower(50.0f); }
            if (key == GLFW_KEY_DOWN && !downPressed) { downPressed = true; cannon->adjustPower(-50.0f); }
            if (key == GLFW_KEY_SPACE && !spacePressed) {
                spacePressed = true;
                glm::vec2 muzzlePos = cannon->getMuzzlePosition();
                glm::vec2 velocity = cannon->getFiringVelocity();
                glm::vec3 ballColor(colorDist(rng), colorDist(rng), colorDist(rng));
                auto ball = new objects::Ball2D(muzzlePos, velocity, 10.0f, 0.8f, ballColor);
                scene->add(ball);
                projectiles.push_back(ball);
                ballsFired++;
                std::cout << "Ball fired! Total: " << ballsFired << std::endl;
            }
            if (key == GLFW_KEY_R && !rKeyPressed) {
                rKeyPressed = true;
                respawnCloth(scene, &projectiles, &cloth);
            }
        }
        if (action == GLFW_RELEASE) {
            if (key == GLFW_KEY_SPACE) spacePressed = false;
            if (key == GLFW_KEY_UP) upPressed = false;
            if (key == GLFW_KEY_DOWN) downPressed = false;
            if (key == GLFW_KEY_R) rKeyPressed = false;
        }
    };

    // ---- Recreate pipelines on swapchain resize ----
    vk.OnResize = [&](int, int) {
        vkDestroyPipeline(vk.GetDevice(), triPipeline, nullptr);
        vkDestroyPipeline(vk.GetDevice(), linePipeline, nullptr);
        triPipeline = CreateTrianglePipeline(vk, vertModule, fragModule, pipelineLayout);
        linePipeline = CreateLinePipeline(vk, vertModule, fragModule, pipelineLayout);
    };

    // ---- Main loop ----
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (true) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        if (!vk.BeginFrame()) break;

        // ---- Update physics ----
        sceneManager.update2D(dt);

        // Clean up inactive projectiles
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                [](objects::Ball2D* ball) { return !ball || !ball->active; }),
            projectiles.end());

        // ---- Gather geometry ----
        auto lineVerts = sceneManager.gatherLineVertices();
        auto triVerts = sceneManager.gatherTriangleVertices();

        // ---- Build orthographic MVP (pixel coords -> NDC) ----
        float w = (float)vk.GetWidth();
        float h = (float)vk.GetHeight();
        glm::mat4 ortho = glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, -1.0f, 1.0f);
        // Vulkan clip-space Y is flipped vs GL
        glm::mat4 flipY = glm::mat4(1.0f);
        // Actually ortho already maps correctly for 2D if we keep the y-down convention

        PushConstants2D pc;
        pc.mvp = ortho;

        VkCommandBuffer cmd = vk.GetCurrentCommandBuffer();
        uint32_t frameIdx = vk.GetFrameIndex();

        // ---- Upload and draw triangles ----
        if (!triVerts.empty()) {
            VkDeviceSize dataSize = triVerts.size() * sizeof(scene::Object2D::Vertex2D);
            if (dataSize <= maxVertBufSize) {
                vk.CopyToBuffer(triBuffers[frameIdx], triVerts.data(), dataSize);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, triPipeline);
                vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &triBuffers[frameIdx].Buffer, &offset);
                vkCmdDraw(cmd, static_cast<uint32_t>(triVerts.size()), 1, 0, 0);
            }
        }

        // ---- Upload and draw lines ----
        if (!lineVerts.empty()) {
            VkDeviceSize dataSize = lineVerts.size() * sizeof(scene::Object2D::Vertex2D);
            if (dataSize <= maxVertBufSize) {
                vk.CopyToBuffer(lineBuffers[frameIdx], lineVerts.data(), dataSize);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
                vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
                VkDeviceSize offset = 0;
                vkCmdBindVertexBuffers(cmd, 0, 1, &lineBuffers[frameIdx].Buffer, &offset);
                vkCmdDraw(cmd, static_cast<uint32_t>(lineVerts.size()), 1, 0, 0);
            }
        }

        // ---- ImGui overlay ----
        vk.ImGuiNewFrame();
        ImGui::Begin("2D Physics Info");
        ImGui::Text("FPS: %.1f", 1.0f / dt);
        ImGui::Text("Balls Active: %d", (int)projectiles.size());
        ImGui::Text("Balls Fired: %d", ballsFired);
        ImGui::Text("Cannon Angle: %.0f deg", cannon->angle);
        ImGui::Text("Cannon Power: %.0f", cannon->power);
        ImGui::Separator();
        ImGui::TextWrapped("Controls: Left/Right Arrow - Rotate Cannon\n"
                           "Up/Down Arrow - Adjust Power\n"
                           "Space - Fire Ball | R - Respawn Cloth");
        ImGui::End();
        vk.ImGuiRender(cmd);

        vk.EndFrame();
    }

    // ---- Cleanup ----
    vkDeviceWaitIdle(vk.GetDevice());

    for (int i = 0; i < 2; ++i) {
        vk.DestroyBuffer(triBuffers[i]);
        vk.DestroyBuffer(lineBuffers[i]);
    }
    vkDestroyPipeline(vk.GetDevice(), triPipeline, nullptr);
    vkDestroyPipeline(vk.GetDevice(), linePipeline, nullptr);
    vkDestroyPipelineLayout(vk.GetDevice(), pipelineLayout, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), vertModule, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), fragModule, nullptr);

    projectiles.clear();

    std::cout << "Application terminated successfully" << std::endl;
    return 0;
}
