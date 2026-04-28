/**
 * Particle System Demo — Vulkan port
 *
 * Originally written with FreeGLUT + OpenGL immediate mode.
 * Rewritten to use vkdemo::VulkanBase (GLFW + Vulkan) with
 * dynamic vertex buffers, push-constant MVP, and runtime
 * GLSL-to-SPIR-V compilation via shaderc.
 *
 * Particle / Floor / Line logic is unchanged.
 */

#include "VulkanBase.hpp"
#include "EmbeddedShaders.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// shaderc for runtime GLSL compilation
#if HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

#include <ctime>
#include <cmath>
#include <cstring>
#include <list>
#include <vector>
#include <array>
#include <iostream>
#include <algorithm>
#include <stdexcept>

#include "Particle.h"
#include "Floor.h"
#include "Line.h"

using namespace std;

// ============================================================================
// Embedded GLSL shaders (compiled at runtime via shaderc)
// ============================================================================

// Vertex shader: position(vec3) + color(vec4), push constant MVP
static const std::string kParticleVertSrc = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    float pointSize;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    gl_PointSize = pc.pointSize;
    fragColor = inColor;
}
)";

// Fragment shader: pass-through with alpha
static const std::string kParticleFragSrc = R"(
#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;
}
)";

// ============================================================================
// Compile GLSL to SPIR-V at runtime
// ============================================================================

static std::vector<uint32_t> CompileGLSL(const std::string& source,
                                          const std::string& name,
                                          bool isVertex)
{
#if HAS_SHADERC
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
    auto result = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        throw std::runtime_error("Shader compilation failed (" + name + "): " +
                                  result.GetErrorMessage());
    }
    return {result.cbegin(), result.cend()};
#else
    (void)source; (void)name; (void)isVertex;
    throw std::runtime_error(
        "shaderc not available — cannot compile shaders at runtime. "
        "Please install the Vulkan SDK with shaderc support.");
#endif
}

// ============================================================================
// Vertex format: interleaved position (vec3) + color (vec4)
// ============================================================================

struct Vertex {
    float px, py, pz;
    float cr, cg, cb, ca;
};

// ============================================================================
// Push constant block — must match shader layout
// ============================================================================

struct PushConstantBlock {
    glm::mat4 mvp;
    float pointSize;
    float _pad[3]; // padding to 16-byte alignment
};

// ============================================================================
// Global state (mirrors original)
// ============================================================================

// Window / camera
int height = 800;
int width  = 800;
double yRotate = 212.50;
int    xRotate = 25;
int    zoom    = 50;

// Particle properties
float scaleFactor      = 0.25f;
bool  particlePaths    = false;
bool  particleBumping  = false;
int   cArr[3][3]       = { {0, 162, 211}, {250, 224, 20}, {224, 8, 133} };

// Environment
double gravity         = 0.1;
double friction        = 0.2;
int    numFloors       = 5;
bool   removeParticles = true;
int    particleCount   = 0;
float  firePosition[3] = { 0, 15, 0 };

// Animation
bool animationPause = false;

// Cannon
double spreadRandomness = 0.2;
bool   constantFire     = false;
bool   randSpeed        = false;

list<Particle> listParticles;
list<Floor>    listFloors;

// ============================================================================
// Particle / physics logic (unchanged from original)
// ============================================================================

void addParticle() {
    Particle p = Particle(firePosition, spreadRandomness, scaleFactor,
                          ++particleCount, randSpeed);
    listParticles.push_back(p);
}

float floorCollision(Particle& p) {
    float y = p.getPos()[1], x = p.getPos()[0], z = p.getPos()[2];
    for (auto f = listFloors.begin(); f != listFloors.end(); ++f) {
        if ((y < (f->getPos() + (5 * p.getSize())))
            && (x > (-f->getSize() - (5 * p.getSize())))
            && (x < ( f->getSize() + (5 * p.getSize())))
            && (z > (-f->getSize() - (5 * p.getSize())))
            && (z < ( f->getSize() + (5 * p.getSize())))) {
            return f->getPos();
        }
    }
    return 0;
}

void particleCollision(Particle& p) {
    for (auto q = listParticles.begin(); q != listParticles.end(); ++q) {
        if (p.getId() != q->getId()) {
            if (q->getCol() == 2 && p.getCol() == 1) {
                double dx = pow((double)q->getPos()[0] - (double)p.getPos()[0], 2);
                double dy = pow((double)q->getPos()[1] - (double)p.getPos()[1], 2);
                double dz = pow((double)q->getPos()[2] - (double)p.getPos()[2], 2);
                double d  = sqrt(dx + dy + dz);
                if (d <= ((p.getSize() * 5) + (q->getSize() * 5))) {
                    bool bounceX = (p.getPos()[0] > q->getPos()[0]);
                    bool bounceY = (p.getPos()[1] > q->getPos()[1]);
                    bool bounceZ = (p.getPos()[2] > q->getPos()[2]);
                    p.changeDirection(bounceX, bounceZ, bounceY);
                }
            }
        }
    }
}

void moveParticles() {
    for (auto p = listParticles.begin(); p != listParticles.end(); ++p) {
        if (p->getSpeed() != 0) {
            p->move(gravity);
        }
        float collisionPosition = floorCollision(*p);
        if (numFloors != 0) {
            if (collisionPosition != 0) {
                if (p->getCol() == 0) { p->changeColor(); }
                p->bounce(collisionPosition, friction);
                if (p->checkDead(gravity)) {
                    if (p->getCol() == 1) { p->changeColor(); }
                }
            }
            p->checkOffPyramid(removeParticles, listFloors.back().getPos());
        }
        if (particleBumping) { particleCollision(*p); }
    }
}

bool recordRemovalPredicate(Particle& p) { return (p.getLife() <= 0); }
void removeRecord() { listParticles.remove_if(recordRemovalPredicate); }

void addFloor(int k) {
    for (double i = -5.0, j = 5.0; k != 0; i -= 2.5, j += 5, k--) {
        listFloors.push_back(Floor(static_cast<float>(i), static_cast<float>(j)));
    }
}

void reset() {
    scaleFactor = 0.25f; gravity = 0.1; friction = 0.2;
    spreadRandomness = 0.2; removeParticles = true;
    constantFire = false; randSpeed = false;
    animationPause = false; particleBumping = false; particlePaths = false;
    yRotate = 212.50; xRotate = 25; zoom = 50;
    listParticles.clear(); listFloors.clear();
    firePosition[0] = 0; firePosition[2] = 0; numFloors = 5; addFloor(5);
}

void printVariables() {
    cout << "Number of Particles: " << listParticles.size() << endl;
    cout << "Current Gravity: " << gravity << endl;
    cout << "Current Friction: " << friction << endl;
}

// ============================================================================
// Geometry helpers — build vertex data each frame
// ============================================================================

// Generate a unit cube (36 verts, 12 triangles) centered at origin with the
// given half-extent and RGBA color.
static void generateCubeVertices(float halfSize, float r, float g, float b, float a,
                                  std::vector<Vertex>& out)
{
    float s = halfSize;
    // 6 faces, 2 triangles each, 3 verts per tri = 36 verts
    float positions[][3] = {
        // Front
        {-s,-s, s}, { s,-s, s}, { s, s, s}, {-s,-s, s}, { s, s, s}, {-s, s, s},
        // Back
        { s,-s,-s}, {-s,-s,-s}, {-s, s,-s}, { s,-s,-s}, {-s, s,-s}, { s, s,-s},
        // Left
        {-s,-s,-s}, {-s,-s, s}, {-s, s, s}, {-s,-s,-s}, {-s, s, s}, {-s, s,-s},
        // Right
        { s,-s, s}, { s,-s,-s}, { s, s,-s}, { s,-s, s}, { s, s,-s}, { s, s, s},
        // Top
        {-s, s, s}, { s, s, s}, { s, s,-s}, {-s, s, s}, { s, s,-s}, {-s, s,-s},
        // Bottom
        {-s,-s,-s}, { s,-s,-s}, { s,-s, s}, {-s,-s,-s}, { s,-s, s}, {-s,-s, s},
    };
    for (int i = 0; i < 36; i++) {
        out.push_back({positions[i][0], positions[i][1], positions[i][2],
                       r, g, b, a});
    }
}

// ============================================================================
// Main
// ============================================================================

int main() {
    // ----- Seed RNG -----
    srand(static_cast<unsigned int>(time(nullptr)));

    // ----- Setup particle / floor state -----
    listParticles = list<Particle>();
    listFloors    = list<Floor>();
    addFloor(5);

    // ----- Print controls -----
    cout << "Vulkan Particle System Demo" << endl;
    cout << "Press or hold 'F' to fire particles." << endl;
    cout << "Press 'C' to toggle constant fire." << endl;
    cout << "Press '1'/'2' to rotate camera left/right." << endl;
    cout << "Press '3'/'4' to tilt camera up/down." << endl;
    cout << "Press '5'/'6' to zoom in/out." << endl;
    cout << "Press '7'/'8'/'9'/'0' to move cannon x/z." << endl;
    cout << "Press 'Q'/'W' to remove/add a floor." << endl;
    cout << "Press 'P' to toggle pause." << endl;
    cout << "Press 'L' to toggle path drawing." << endl;
    cout << "Press 'B' to toggle inter-particle collision." << endl;
    cout << "Press 'R' to reset." << endl;
    cout << "Press 'G' to print diagnostics." << endl;
    cout << "Press Escape to quit." << endl;

    // ----- Initialize Vulkan via VulkanBase -----
    vkdemo::VulkanBase vk;
    vkdemo::AppConfig config;
    config.Title  = "Particle System (Vulkan)";
    config.Width  = width;
    config.Height = height;
    config.EnableValidation = true;
    config.EnableImGui = false;
    vk.Init(config);

    // ----- Compile shaders -----
    auto vertSpirv = CompileGLSL(kParticleVertSrc, "particle.vert", true);
    auto fragSpirv = CompileGLSL(kParticleFragSrc, "particle.frag", false);

    VkShaderModule vertModule = vk.CreateShaderModule(vertSpirv);
    VkShaderModule fragModule = vk.CreateShaderModule(fragSpirv);

    // ----- Pipeline layout (push constants) -----
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pcRange.offset = 0;
    pcRange.size = sizeof(PushConstantBlock);

    VkPipelineLayout pipelineLayout = vk.CreatePipelineLayout({}, {pcRange});

    // ----- Vertex input binding -----
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding   = 0;
    bindingDesc.stride    = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 2> attrDescs{};
    // location 0: position (vec3)
    attrDescs[0].binding  = 0;
    attrDescs[0].location = 0;
    attrDescs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset   = offsetof(Vertex, px);
    // location 1: color (vec4)
    attrDescs[1].binding  = 0;
    attrDescs[1].location = 1;
    attrDescs[1].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
    attrDescs[1].offset   = offsetof(Vertex, cr);

    // ----- Helper to create a graphics pipeline for a given topology -----
    auto createPipeline = [&](VkPrimitiveTopology topology,
                              bool enableBlending,
                              bool depthWriteEnable) -> VkPipeline
    {
        // Shader stages
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

        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        // Vertex input
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount   = 1;
        vertexInput.pVertexBindingDescriptions      = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount  = static_cast<uint32_t>(attrDescs.size());
        vertexInput.pVertexAttributeDescriptions     = attrDescs.data();

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport/scissor (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth   = 1.0f;
        rasterizer.cullMode    = VK_CULL_MODE_NONE;
        rasterizer.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // Multisample
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth/stencil
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable  = VK_TRUE;
        depthStencil.depthWriteEnable = depthWriteEnable ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS_OR_EQUAL;

        // Color blend
        VkPipelineColorBlendAttachmentState blendAttachment{};
        blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if (enableBlending) {
            blendAttachment.blendEnable         = VK_TRUE;
            blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            blendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
            blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            blendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
        } else {
            blendAttachment.blendEnable = VK_FALSE;
        }

        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments    = &blendAttachment;

        // Dynamic state
        VkDynamicState dynStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates    = dynStates;

        // Create pipeline
        VkGraphicsPipelineCreateInfo pipelineCI{};
        pipelineCI.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineCI.stageCount          = 2;
        pipelineCI.pStages             = stages;
        pipelineCI.pVertexInputState   = &vertexInput;
        pipelineCI.pInputAssemblyState = &inputAssembly;
        pipelineCI.pViewportState      = &viewportState;
        pipelineCI.pRasterizationState = &rasterizer;
        pipelineCI.pMultisampleState   = &multisampling;
        pipelineCI.pDepthStencilState  = &depthStencil;
        pipelineCI.pColorBlendState    = &colorBlend;
        pipelineCI.pDynamicState       = &dynamicState;
        pipelineCI.layout              = pipelineLayout;
        pipelineCI.renderPass          = vk.GetRenderPass();
        pipelineCI.subpass             = 0;

        VkPipeline pipeline = VK_NULL_HANDLE;
        if (vkCreateGraphicsPipelines(vk.GetDevice(), VK_NULL_HANDLE, 1, &pipelineCI,
                                       nullptr, &pipeline) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }
        return pipeline;
    };

    // Create three pipelines: triangles (floors), points (particles), lines (paths)
    VkPipeline trianglePipeline = createPipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true);
    VkPipeline pointPipeline    = createPipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST,    true, false);
    VkPipeline linePipeline     = createPipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST,     true, false);

    // ----- Dynamic vertex buffer (host-visible, re-uploaded each frame) -----
    // We size it generously; it grows if needed.
    const VkDeviceSize kInitialBufferSize = 1024 * 1024; // 1 MB
    vkdemo::GPUBuffer dynamicVB = vk.CreateBuffer(
        kInitialBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    VkDeviceSize currentBufferCapacity = kInitialBufferSize;

    // ----- Recreate pipelines on swapchain resize -----
    vk.OnResize = [&](int /*w*/, int /*h*/) {
        vkDestroyPipeline(vk.GetDevice(), trianglePipeline, nullptr);
        vkDestroyPipeline(vk.GetDevice(), pointPipeline, nullptr);
        vkDestroyPipeline(vk.GetDevice(), linePipeline, nullptr);
        trianglePipeline = createPipeline(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true);
        pointPipeline    = createPipeline(VK_PRIMITIVE_TOPOLOGY_POINT_LIST,    true, false);
        linePipeline     = createPipeline(VK_PRIMITIVE_TOPOLOGY_LINE_LIST,     true, false);
    };

    // ----- Keyboard callback -----
    vk.OnKey = [&](GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key) {
                case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
                case GLFW_KEY_F:  addParticle(); break;
                case GLFW_KEY_G:  printVariables(); break;
                case GLFW_KEY_P:  animationPause = !animationPause; break;
                case GLFW_KEY_R:  reset(); break;
                case GLFW_KEY_C:  constantFire = !constantFire; break;
                case GLFW_KEY_L:  particlePaths = !particlePaths; break;
                case GLFW_KEY_B:  particleBumping = !particleBumping; break;
                case GLFW_KEY_1:  yRotate = (yRotate >= 359.9) ? 0.0 : yRotate + 0.1; break;
                case GLFW_KEY_2:  yRotate = (yRotate <= 0.0) ? 359.9 : yRotate - 0.1; break;
                case GLFW_KEY_3:  xRotate += 1; break;
                case GLFW_KEY_4:  xRotate -= 1; break;
                case GLFW_KEY_5:  zoom++; break;
                case GLFW_KEY_6:  zoom--; break;
                case GLFW_KEY_7:  firePosition[0]++; break;
                case GLFW_KEY_8:  firePosition[0]--; break;
                case GLFW_KEY_9:  firePosition[2]++; break;
                case GLFW_KEY_0:  firePosition[2]--; break;
                case GLFW_KEY_Q: {
                    listFloors.clear();
                    addFloor(numFloors = (numFloors > 0) ? numFloors - 1 : 0);
                    break;
                }
                case GLFW_KEY_W: {
                    listFloors.clear();
                    addFloor(numFloors = (numFloors < 10) ? numFloors + 1 : 10);
                    break;
                }
                default: break;
            }
        }
    };

    // ----- Main loop -----
    while (vk.BeginFrame()) {
        VkCommandBuffer cmd = vk.GetCurrentCommandBuffer();

        // --- Simulation tick ---
        if (constantFire && !animationPause) {
            addParticle();
        }
        if (!animationPause) {
            moveParticles();
        }
        removeRecord();

        // --- Build camera MVP ---
        float aspect = static_cast<float>(vk.GetWidth()) /
                        static_cast<float>(std::max(vk.GetHeight(), 1));
        glm::mat4 proj = glm::perspective(glm::radians(74.0f), aspect, 2.0f, 300.0f);
        // Flip Y for Vulkan coordinate system
        proj[1][1] *= -1.0f;

        double xCam = zoom * cos(yRotate);
        double zCam = zoom * sin(yRotate);
        glm::mat4 view = glm::lookAt(
            glm::vec3(static_cast<float>(xCam), static_cast<float>(xRotate), static_cast<float>(zCam)),
            glm::vec3(0.0f, -20.0f, 0.0f),
            glm::vec3(0.0f,  1.0f,  0.0f));

        glm::mat4 vp = proj * view;

        // --- Collect all geometry into vertex arrays ---
        std::vector<Vertex> floorVerts;
        std::vector<Vertex> particleVerts;
        std::vector<Vertex> lineVerts;

        // Floors (cubes scaled and translated)
        {
            int i = numFloors - 1;
            for (auto f = listFloors.begin(); f != listFloors.end(); ++f, --i) {
                double blend = 255.0 - (static_cast<double>(i) / static_cast<double>(listFloors.size())) * 255.0;
                float r = 186.0f / 255.0f;
                float g = 126.0f / 255.0f;
                float b = 207.0f / 255.0f;
                float a = static_cast<float>(blend) / 255.0f;

                // Generate a unit cube, then scale/translate it via vertex positions
                // Original: glTranslatef(0, f->getPos(), 0); glScalef(f->getSize(), 0.1, f->getSize());
                // glutSolidCube(2) -> cube from -1 to +1
                float sx = f->getSize();
                float sy = 0.1f;
                float sz = f->getSize();
                float ty = f->getPos();

                // 36 verts for a cube
                std::vector<Vertex> cubeVerts;
                generateCubeVertices(1.0f, r, g, b, a, cubeVerts);
                for (auto& v : cubeVerts) {
                    v.px *= sx;
                    v.py = v.py * sy + ty;
                    v.pz *= sz;
                    floorVerts.push_back(v);
                }
            }
        }

        // Particles (as points)
        for (auto p = listParticles.begin(); p != listParticles.end(); ++p) {
            if (p->getLife() > 0) {
                double blend = (static_cast<double>(p->getLife()) / 100.0) * 255.0;
                float r = static_cast<float>(cArr[p->getCol()][0]) / 255.0f;
                float g = static_cast<float>(cArr[p->getCol()][1]) / 255.0f;
                float b = static_cast<float>(cArr[p->getCol()][2]) / 255.0f;
                float a = static_cast<float>(blend) / 255.0f;

                particleVerts.push_back({
                    p->getPos()[0], p->getPos()[1], p->getPos()[2],
                    r, g, b, a
                });

                // Path lines
                if (particlePaths) {
                    float lr = 1.0f, lg = 1.0f, lb = 1.0f;
                    float la = a;
                    list<Line> path = p->getPath();
                    if (path.size() >= 2) {
                        auto prev = path.begin();
                        auto cur  = prev;
                        ++cur;
                        for (; cur != path.end(); ++cur) {
                            lineVerts.push_back({
                                prev->getPos()[0], prev->getPos()[1], prev->getPos()[2],
                                lr, lg, lb, la
                            });
                            lineVerts.push_back({
                                cur->getPos()[0], cur->getPos()[1], cur->getPos()[2],
                                lr, lg, lb, la
                            });
                            prev = cur;
                        }
                    }
                }
            }
        }

        // --- Upload vertex data to dynamic buffer ---
        size_t totalVerts = floorVerts.size() + particleVerts.size() + lineVerts.size();
        VkDeviceSize requiredSize = totalVerts * sizeof(Vertex);

        if (requiredSize > currentBufferCapacity && requiredSize > 0) {
            // Grow buffer
            vk.DestroyBuffer(dynamicVB);
            VkDeviceSize newCap = requiredSize * 2;
            dynamicVB = vk.CreateBuffer(
                newCap,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            currentBufferCapacity = newCap;
        }

        if (totalVerts > 0) {
            // Map and write all vertex data contiguously
            void* mapped = nullptr;
            vkMapMemory(vk.GetDevice(), dynamicVB.Memory, 0, requiredSize, 0, &mapped);
            auto* dst = reinterpret_cast<Vertex*>(mapped);

            size_t offset = 0;
            if (!floorVerts.empty()) {
                memcpy(dst + offset, floorVerts.data(), floorVerts.size() * sizeof(Vertex));
                offset += floorVerts.size();
            }
            if (!particleVerts.empty()) {
                memcpy(dst + offset, particleVerts.data(), particleVerts.size() * sizeof(Vertex));
                offset += particleVerts.size();
            }
            if (!lineVerts.empty()) {
                memcpy(dst + offset, lineVerts.data(), lineVerts.size() * sizeof(Vertex));
            }

            vkUnmapMemory(vk.GetDevice(), dynamicVB.Memory);
        }

        // --- Record draw commands ---
        VkBuffer vertexBuffers[] = {dynamicVB.Buffer};
        VkDeviceSize offsets[]   = {0};

        uint32_t floorOffset    = 0;
        uint32_t particleOffset = static_cast<uint32_t>(floorVerts.size());
        uint32_t lineOffset     = particleOffset + static_cast<uint32_t>(particleVerts.size());

        PushConstantBlock pc{};
        pc.mvp = vp;
        pc.pointSize = 5.0f; // base point size

        // Draw floors (triangles, blended, depth-write on)
        if (!floorVerts.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
            pc.pointSize = 1.0f;
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(PushConstantBlock), &pc);
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdDraw(cmd, static_cast<uint32_t>(floorVerts.size()), 1, floorOffset, 0);
        }

        // Draw particles (points)
        if (!particleVerts.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline);
            // Point size proportional to scale factor — match the original sphere diameter
            pc.pointSize = scaleFactor * 5.0f * 10.0f;
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(PushConstantBlock), &pc);
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdDraw(cmd, static_cast<uint32_t>(particleVerts.size()), 1, particleOffset, 0);
        }

        // Draw paths (lines)
        if (!lineVerts.empty()) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
            pc.pointSize = 1.0f;
            vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(PushConstantBlock), &pc);
            vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
            vkCmdDraw(cmd, static_cast<uint32_t>(lineVerts.size()), 1, lineOffset, 0);
        }

        vk.EndFrame();
    }

    // ----- Cleanup -----
    vkDeviceWaitIdle(vk.GetDevice());

    vk.DestroyBuffer(dynamicVB);
    vkDestroyPipeline(vk.GetDevice(), trianglePipeline, nullptr);
    vkDestroyPipeline(vk.GetDevice(), pointPipeline, nullptr);
    vkDestroyPipeline(vk.GetDevice(), linePipeline, nullptr);
    vkDestroyPipelineLayout(vk.GetDevice(), pipelineLayout, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), vertModule, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), fragModule, nullptr);

    vk.Shutdown();
    return 0;
}
