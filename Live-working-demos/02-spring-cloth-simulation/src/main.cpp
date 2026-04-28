// ============================================================================
// Spring Cloth (Flag) Simulation -- Vulkan port
//
// Original: SDL + GLEW + OpenGL 3.3 + AntTweakBar
// Ported to: VulkanBase (GLFW + Vulkan) + ImGui
//
// All cloth physics / simulation logic is preserved exactly.
// ============================================================================

#include <VulkanBase.hpp>

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/random.hpp>

#include <shaderc/shaderc.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// Compile GLSL to SPIR-V at runtime via shaderc
// ============================================================================
static std::vector<uint32_t> compileGLSL(const std::string& source,
                                         shaderc_shader_kind kind,
                                         const char* name) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto result = compiler.CompileGlslToSpv(source, kind, name, options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "Shader compilation error (" << name << "):\n"
                  << result.GetErrorMessage() << std::endl;
        throw std::runtime_error("Shader compilation failed");
    }
    return {result.cbegin(), result.cend()};
}

// ============================================================================
// TrackballCamera (pure math, no GL/SDL dependency)
// ============================================================================
class TrackballCamera {
public:
    TrackballCamera() : m_fDistance(5.f), m_fAngleX(0.f), m_fAngleY(0.f) {}

    void moveFront(float delta) {
        m_fDistance += delta;
        m_fDistance = glm::max(0.1f, m_fDistance);
    }

    void rotateLeft(float degrees) { m_fAngleY += degrees * 0.5f; }
    void rotateUp(float degrees)   { m_fAngleX += degrees * 0.5f; }

    glm::mat4 getViewMatrix() const {
        glm::mat4 V = glm::lookAt(glm::vec3(0, 0, m_fDistance),
                                   glm::vec3(0.f), glm::vec3(0, 1, 0));
        V = glm::rotate(V, m_fAngleX, glm::vec3(1, 0, 0));
        V = glm::rotate(V, m_fAngleY, glm::vec3(0, 1, 0));
        return V;
    }

private:
    float m_fDistance;
    float m_fAngleX, m_fAngleY;
};

// ============================================================================
// SphereHandler  (simple POD container)
// ============================================================================
struct SphereHandler {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<float>     radius;
};

// ============================================================================
// Octree  (header-only template, kept from original)
// ============================================================================
template <typename T>
class Octree {
private:
    int _depth;
    glm::vec3 _position;
    glm::vec3 _dimension;
    std::vector<Octree> _children;
    Octree* _madre;
    std::vector<T> _values;
    bool _initChildren;

    void initChildren() {
        if (_initChildren || _depth == 0) return;
        glm::vec3 offset = _dimension / 4.f;
        glm::vec3 childDimension = _dimension / 2.f;
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x + offset.x, _position.y + offset.y, _position.z + offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x + offset.x, _position.y + offset.y, _position.z - offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x - offset.x, _position.y + offset.y, _position.z + offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x - offset.x, _position.y + offset.y, _position.z - offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x + offset.x, _position.y - offset.y, _position.z + offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x + offset.x, _position.y - offset.y, _position.z - offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x - offset.x, _position.y - offset.y, _position.z + offset.z), childDimension, this));
        _children.push_back(Octree<T>(_depth-1, glm::vec3(_position.x - offset.x, _position.y - offset.y, _position.z - offset.z), childDimension, this));
        _initChildren = true;
    }

    void cleanRecursive() {
        bool allChildrenEmpty = true;
        for (auto& child : _children) {
            if (child._initChildren) { allChildrenEmpty = false; break; }
        }
        if (allChildrenEmpty) { _children.clear(); _initChildren = false; }
        if (_madre != nullptr) { _madre->cleanRecursive(); return; }
    }

public:
    Octree(int depth, const glm::vec3& position, const glm::vec3& dimension, Octree<T>* madre = nullptr)
        : _depth(depth), _position(position), _dimension(dimension), _madre(madre), _initChildren(false) {}

    void add(const T& value, const glm::vec3& position) {
        if (_depth == 0 && contains(position)) { _values.push_back(value); return; }
        if (contains(position)) {
            initChildren();
            for (auto& octree : _children) {
                if (octree.contains(position)) { octree.add(value, position); return; }
            }
        }
        // Silently ignore out-of-bounds instead of throwing (robustness)
    }

    void remove(const T& value, const glm::vec3& position) {
        if (_depth == 0 && contains(position)) {
            for (int i = 0; i < (int)_values.size(); ++i) {
                if (_values[i] == value) { _values.erase(_values.begin() + i); --i; }
            }
            if (_values.empty()) cleanRecursive();
            return;
        }
        if (contains(position)) {
            for (auto& octree : _children) {
                if (octree.contains(position)) { octree.remove(value, position); }
            }
        }
    }

    std::vector<T>& get(const glm::vec3& position) {
        if (_depth == 0 && contains(position)) return _values;
        if (!_initChildren) return _values;
        if (contains(position)) {
            for (auto& octree : _children) {
                if (octree.contains(position)) return octree.get(position);
            }
        }
        return _values; // fallback
    }

    bool contains(const glm::vec3& position) {
        return !(position.x > _position.x + _dimension.x / 2.f ||
                 position.x < _position.x - _dimension.x / 2.f ||
                 position.y > _position.y + _dimension.y / 2.f ||
                 position.y < _position.y - _dimension.y / 2.f ||
                 position.z > _position.z + _dimension.z / 2.f ||
                 position.z < _position.z - _dimension.z / 2.f);
    }
};

// ============================================================================
// Physics: force helpers (EXACT originals)
// ============================================================================
inline glm::vec3 hookForce(float K, float L, const glm::vec3& P1, const glm::vec3& P2) {
    static const float epsilon = 0.0001f;
    return K * (1 - (L / std::max(glm::distance(P1, P2), epsilon))) * (P2 - P1);
}

inline glm::vec3 repulseForce(float dst, const glm::vec3& P1, const glm::vec3& P2) {
    glm::vec3 direction = glm::normalize(P1 - P2);
    return direction * (1.f / (1.f + glm::pow(dst, 2.f)));
}

inline glm::vec3 brakeForce(float V, float dt, const glm::vec3& v1, const glm::vec3& v2) {
    return V * ((v2 - v1) / dt);
}

inline glm::vec3 sphereCollisionForce(float distanceToCenter,
                                      const glm::vec3& sphereCenter,
                                      float /*sphereRadius*/,
                                      const glm::vec3 particlePosition,
                                      const glm::vec3& /*forceParticle*/) {
    glm::vec3 direction = glm::normalize(particlePosition - sphereCenter);
    return direction * (1.f / (1.f + glm::pow(distanceToCenter, 2.f)));
}

// ============================================================================
// Flag struct (ALL simulation logic preserved exactly)
// ============================================================================
struct Flag {
    int gridWidth, gridHeight;
    std::vector<glm::vec3> positionArray;
    std::vector<glm::vec3> velocityArray;
    std::vector<float> massArray;
    std::vector<glm::vec3> forceArray;
    int nbParticles;

    glm::vec2 L0;
    float L1;
    glm::vec2 L2;
    float K0, K1, K2;
    float V0, V1, V2;

    Flag(float mass, float width, float height, int gridWidth, int gridHeight)
        : gridWidth(gridWidth), gridHeight(gridHeight),
          positionArray(gridWidth * gridHeight),
          velocityArray(gridWidth * gridHeight, glm::vec3(0.f)),
          massArray(gridWidth * gridHeight, mass / (gridWidth * gridHeight)),
          forceArray(gridWidth * gridHeight, glm::vec3(0.f)) {

        glm::vec3 origin(-0.5f * width, 0.f, 0.f);
        glm::vec3 scale(width / (gridWidth - 1), height / (gridHeight - 1), 1.f);

        nbParticles = gridWidth * gridHeight;
        for (int j = 0; j < gridHeight; ++j) {
            for (int i = 0; i < gridWidth; ++i) {
                int k = i + j * gridWidth;
                positionArray[k] = origin + glm::vec3(i, j, origin.z) * scale;
                massArray[k] = 1 - (i / (2 * (gridHeight * gridWidth)));
            }
        }

        L0.x = scale.x;
        L0.y = scale.y;
        L1 = glm::length(L0);
        L2 = 4.f * L0;

        K0 = 1; K1 = 1; K2 = 1;
        V0 = 0.08f; V1 = 0.02f; V2 = 0.06f;
    }

    void applyInternalForces(float dt) {
        std::vector<glm::ivec2> neighbors(4);
        for (int i = 0; i < gridWidth; ++i) {
            for (int j = 0; j < gridHeight - 1; ++j) {
                int currentK = j * gridWidth + i;

                // TOPOLOGY 1
                neighbors[0] = glm::ivec2(i + 1, j);
                neighbors[1] = glm::ivec2(i - 1, j);
                neighbors[2] = glm::ivec2(i, j - 1);
                neighbors[3] = glm::ivec2(i, j + 1);

                int tmpI = 0;
                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K0, tmpI < 2 ? L0.x : L0.y,
                                                      positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V0, dt,
                                                       velocityArray[currentK], velocityArray[k]);
                    ++tmpI;
                }

                // TOPOLOGY 2
                neighbors[0] = glm::ivec2(i - 1, j - 1);
                neighbors[1] = glm::ivec2(i + 1, j - 1);
                neighbors[2] = glm::ivec2(i + 1, j + 1);
                neighbors[3] = glm::ivec2(i - 1, j + 1);

                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K1, L1,
                                                      positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V1, dt,
                                                       velocityArray[currentK], velocityArray[k]);
                }

                // TOPOLOGY 3
                neighbors[0] = glm::ivec2(i - 2, j);
                neighbors[1] = glm::ivec2(i + 2, j);
                neighbors[2] = glm::ivec2(i, j - 2);
                neighbors[3] = glm::ivec2(i, j + 2);

                tmpI = 0;
                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K2, tmpI < 2 ? L2.x : L2.y,
                                                      positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V2, dt,
                                                       velocityArray[currentK], velocityArray[k]);
                    ++tmpI;
                }
            }
        }
    }

    void applyRepulseForces(Octree<glm::vec3>& octree, float maxDst, float multRepulse) {
        for (int i = 0; i < gridWidth; ++i) {
            for (int j = 0; j < gridHeight - 1; ++j) {
                int k = j * gridWidth + i;
                auto& pos = positionArray[k];
                auto& inSameVoxel = octree.get(pos);
                if (inSameVoxel.size() < 2) continue;
                for (auto& v : inSameVoxel) {
                    float dst = glm::distance(v, pos);
                    if (dst > maxDst || pos == v) continue;
                    forceArray[k] += repulseForce(dst, pos, v) * multRepulse;
                }
            }
        }
    }

    void applyExternalForce(const glm::vec3& F) {
        for (int i = 0; i < nbParticles; ++i) {
            if (i > nbParticles - gridWidth - 1) continue;
            forceArray[i] += F;
        }
    }

    void applySphereCollision(const SphereHandler& sphereHandler, float multiplier, float radiusDelta) {
        for (int i = 0; i < nbParticles; ++i) {
            if (i > nbParticles - gridWidth - 1) continue;
            for (size_t j = 0; j < sphereHandler.positions.size(); ++j) {
                float dist = glm::distance(sphereHandler.positions[j], positionArray[i]);
                if (dist < sphereHandler.radius[j] + radiusDelta) {
                    forceArray[i] += sphereCollisionForce(dist, sphereHandler.positions[j],
                                                          sphereHandler.radius[j],
                                                          positionArray[i], forceArray[i]) * multiplier;
                }
            }
        }
    }

    void update(float dt) {
        for (int i = 0; i < nbParticles; ++i) {
            velocityArray[i] += dt * (forceArray[i] / massArray[i]);
            positionArray[i] += dt * velocityArray[i];
            forceArray[i] = glm::vec3(0);
        }
    }
};

// ============================================================================
// Sphere mesh generator (for collision-sphere rendering)
// ============================================================================
struct SphereVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

static void buildSphereMesh(float r, int discLat, int discLong,
                            std::vector<SphereVertex>& outVerts) {
    float rcpLat = 1.f / discLat, rcpLong = 1.f / discLong;
    float dPhi = 2.f * glm::pi<float>() * rcpLat;
    float dTheta = glm::pi<float>() * rcpLong;

    std::vector<SphereVertex> data;
    for (int j = 0; j <= discLong; ++j) {
        float cosTheta = cos(-glm::pi<float>() / 2 + j * dTheta);
        float sinTheta = sin(-glm::pi<float>() / 2 + j * dTheta);
        for (int i = 0; i <= discLat; ++i) {
            SphereVertex v;
            v.normal.x = sin(i * dPhi) * cosTheta;
            v.normal.y = sinTheta;
            v.normal.z = cos(i * dPhi) * cosTheta;
            v.position = r * v.normal;
            data.push_back(v);
        }
    }

    outVerts.clear();
    for (int j = 0; j < discLong; ++j) {
        int offset = j * (discLat + 1);
        for (int i = 0; i < discLat; ++i) {
            outVerts.push_back(data[offset + i]);
            outVerts.push_back(data[offset + (i + 1)]);
            outVerts.push_back(data[offset + discLat + 1 + (i + 1)]);
            outVerts.push_back(data[offset + i]);
            outVerts.push_back(data[offset + discLat + 1 + (i + 1)]);
            outVerts.push_back(data[offset + i + discLat + 1]);
        }
    }
}

// ============================================================================
// Cloth vertex (position + normal)
// ============================================================================
struct ClothVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// ============================================================================
// GLSL shaders for the cloth simulation demo
// ============================================================================

// --- Cloth vertex shader: uses push-constant MVP + MV ---
static const std::string kClothVertShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    mat4 mv;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;

void main() {
    fragPosition = vec3(pc.mvp * vec4(inPosition, 1.0));
    fragNormal   = vec3(pc.mv  * vec4(inNormal, 0.0));
    gl_Position  = pc.mvp * vec4(inPosition, 1.0);
}
)";

// --- Cloth fragment shader: same lighting as original ---
static const std::string kClothFragShader = R"(
#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = vec3(abs(dot(normalize(fragPosition), normalize(fragNormal))));
    outColor = vec4(color, 1.0);
}
)";

// --- Sphere vertex shader: MVP + color (fits in 128 bytes) ---
static const std::string kSphereVertShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 mvp;
    vec4 color;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(location = 0) out vec3 fragPositionCS;
layout(location = 1) out vec3 fragNormalCS;
layout(location = 2) out vec3 fragColor;

void main() {
    gl_Position    = pc.mvp * vec4(inPosition, 1.0);
    fragPositionCS = gl_Position.xyz;
    fragNormalCS   = vec3(pc.mvp * vec4(inNormal, 0.0));
    fragColor      = pc.color.rgb;
}
)";

// --- Sphere fragment shader: colored diffuse ---
static const std::string kSphereFragShader = R"(
#version 450

layout(location = 0) in vec3 fragPositionCS;
layout(location = 1) in vec3 fragNormalCS;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 color = fragColor * vec3(abs(dot(normalize(fragPositionCS), normalize(fragNormalCS))));
    outColor = vec4(color, 1.0);
}
)";

// ============================================================================
// Push constant structures (must match shaders)
// ============================================================================
struct ClothPushConstants {
    glm::mat4 mvp;
    glm::mat4 mv;
};

struct SpherePushConstants {
    glm::mat4 mvp;
    glm::vec4 color;
};

// ============================================================================
// Application state
// ============================================================================
static const int WINDOW_WIDTH  = 900;
static const int WINDOW_HEIGHT = 700;

struct AppState {
    // VulkanBase
    vkdemo::VulkanBase vk;

    // Camera
    TrackballCamera camera;
    double mouseLastX = 0, mouseLastY = 0;
    bool leftButtonDown = false;

    // Cloth pipeline
    VkPipeline       clothPipelineFill = VK_NULL_HANDLE;
    VkPipeline       clothPipelineWire = VK_NULL_HANDLE;
    VkPipelineLayout clothPipelineLayout = VK_NULL_HANDLE;

    // Sphere pipeline
    VkPipeline       spherePipeline = VK_NULL_HANDLE;
    VkPipelineLayout spherePipelineLayout = VK_NULL_HANDLE;

    // Cloth GPU buffers (dynamic, updated every frame)
    vkdemo::GPUBuffer clothVB;
    vkdemo::GPUBuffer clothIB;
    uint32_t clothIndexCount = 0;

    // Sphere GPU buffer (static geometry)
    vkdemo::GPUBuffer sphereVB;
    uint32_t sphereVertexCount = 0;

    // Shader modules (kept for cleanup)
    VkShaderModule clothVertModule = VK_NULL_HANDLE;
    VkShaderModule clothFragModule = VK_NULL_HANDLE;
    VkShaderModule sphereVertModule = VK_NULL_HANDLE;
    VkShaderModule sphereFragModule = VK_NULL_HANDLE;
};

// ============================================================================
// Create a graphics pipeline
// ============================================================================
static VkPipeline createPipeline(
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    VkShaderModule vertModule,
    VkShaderModule fragModule,
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes,
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL,
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
{
    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName  = "main";
    stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName  = "main";

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions      = bindings.data();
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions    = attributes.data();

    VkPipelineInputAssemblyStateCreateInfo inputAsm{};
    inputAsm.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsm.topology = topology;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = polygonMode;
    raster.lineWidth   = 1.0f;
    raster.cullMode    = VK_CULL_MODE_NONE;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable  = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAttach{};
    blendAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments    = &blendAttach;

    VkDynamicState dynStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates    = dynStates;

    VkGraphicsPipelineCreateInfo ci{};
    ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount          = 2;
    ci.pStages             = stages;
    ci.pVertexInputState   = &vertexInput;
    ci.pInputAssemblyState = &inputAsm;
    ci.pViewportState      = &viewportState;
    ci.pRasterizationState = &raster;
    ci.pMultisampleState   = &multisample;
    ci.pDepthStencilState  = &depthStencil;
    ci.pColorBlendState    = &colorBlend;
    ci.pDynamicState       = &dynamicState;
    ci.layout              = layout;
    ci.renderPass          = renderPass;
    ci.subpass             = 0;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &ci, nullptr, &pipeline) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }
    return pipeline;
}

// ============================================================================
// Initialization
// ============================================================================
static void initVulkanResources(AppState& app) {
    VkDevice device = app.vk.GetDevice();

    // ---- Compile shaders ----
    auto clothVertSPV  = compileGLSL(kClothVertShader,  shaderc_vertex_shader,   "cloth.vert");
    auto clothFragSPV  = compileGLSL(kClothFragShader,  shaderc_fragment_shader, "cloth.frag");
    auto sphereVertSPV = compileGLSL(kSphereVertShader, shaderc_vertex_shader,   "sphere.vert");
    auto sphereFragSPV = compileGLSL(kSphereFragShader, shaderc_fragment_shader, "sphere.frag");

    app.clothVertModule  = app.vk.CreateShaderModule(clothVertSPV);
    app.clothFragModule  = app.vk.CreateShaderModule(clothFragSPV);
    app.sphereVertModule = app.vk.CreateShaderModule(sphereVertSPV);
    app.sphereFragModule = app.vk.CreateShaderModule(sphereFragSPV);

    // ---- Vertex input descriptions ----
    // Cloth and sphere share the same layout: vec3 pos, vec3 normal
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(ClothVertex); // same as SphereVertex
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].binding  = 0; attrs[0].location = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset   = offsetof(ClothVertex, position);
    attrs[1].binding  = 0; attrs[1].location = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset   = offsetof(ClothVertex, normal);

    std::vector<VkVertexInputBindingDescription> bindings = { binding };
    std::vector<VkVertexInputAttributeDescription> attributes = { attrs[0], attrs[1] };

    // ---- Cloth pipeline layout (push constants = 2 x mat4 = 128 bytes) ----
    VkPushConstantRange clothPC{};
    clothPC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    clothPC.offset     = 0;
    clothPC.size       = sizeof(ClothPushConstants);
    app.clothPipelineLayout = app.vk.CreatePipelineLayout({}, {clothPC});

    // ---- Sphere pipeline layout (push constants = mat4 + vec4 = 80 bytes) ----
    VkPushConstantRange spherePC{};
    spherePC.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    spherePC.offset     = 0;
    spherePC.size       = sizeof(SpherePushConstants);
    app.spherePipelineLayout = app.vk.CreatePipelineLayout({}, {spherePC});

    // ---- Cloth pipelines (fill + wireframe) ----
    app.clothPipelineFill = createPipeline(device, app.vk.GetRenderPass(),
        app.clothPipelineLayout, app.clothVertModule, app.clothFragModule,
        bindings, attributes, VK_POLYGON_MODE_FILL);

    app.clothPipelineWire = createPipeline(device, app.vk.GetRenderPass(),
        app.clothPipelineLayout, app.clothVertModule, app.clothFragModule,
        bindings, attributes, VK_POLYGON_MODE_LINE);

    // ---- Sphere pipeline ----
    app.spherePipeline = createPipeline(device, app.vk.GetRenderPass(),
        app.spherePipelineLayout, app.sphereVertModule, app.sphereFragModule,
        bindings, attributes, VK_POLYGON_MODE_FILL);
}

static void initClothBuffers(AppState& app, const Flag& flag) {
    // Build index buffer (static topology)
    std::vector<uint32_t> indices;
    for (int j = 0; j < flag.gridHeight - 1; ++j) {
        for (int i = 0; i < flag.gridWidth - 1; ++i) {
            indices.push_back(i + j * flag.gridWidth);
            indices.push_back((i + 1) + j * flag.gridWidth);
            indices.push_back((i + 1) + (j + 1) * flag.gridWidth);
            indices.push_back(i + j * flag.gridWidth);
            indices.push_back((i + 1) + (j + 1) * flag.gridWidth);
            indices.push_back(i + (j + 1) * flag.gridWidth);
        }
    }
    app.clothIndexCount = static_cast<uint32_t>(indices.size());

    // Create index buffer (device local, uploaded once)
    app.clothIB = app.vk.CreateIndexBuffer(indices.data(),
        indices.size() * sizeof(uint32_t));

    // Create vertex buffer (host visible, updated every frame)
    VkDeviceSize vbSize = flag.gridWidth * flag.gridHeight * sizeof(ClothVertex);
    app.clothVB = app.vk.CreateBuffer(vbSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

static void initSphereBuffer(AppState& app) {
    std::vector<SphereVertex> sphereVerts;
    buildSphereMesh(1.f, 64, 32, sphereVerts);
    app.sphereVertexCount = static_cast<uint32_t>(sphereVerts.size());
    app.sphereVB = app.vk.CreateVertexBuffer(sphereVerts.data(),
        sphereVerts.size() * sizeof(SphereVertex));
}

// ============================================================================
// Update cloth vertex buffer (compute normals, upload)
// ============================================================================
static void updateClothVertexBuffer(AppState& app, const Flag& flag) {
    int gw = flag.gridWidth;
    int gh = flag.gridHeight;
    std::vector<ClothVertex> verts(gw * gh);

    for (int j = 0; j < gh; ++j) {
        for (int i = 0; i < gw; ++i) {
            int idx = i + j * gw;
            verts[idx].position = flag.positionArray[idx];

            // Compute smooth normal from surrounding triangles (same as original)
            glm::vec3 N(0.f);
            glm::vec3 A = flag.positionArray[idx];

            auto addTriNormal = [&](int ai, int aj, int bi, int bj) {
                if (ai < 0 || aj < 0 || ai >= gw || aj >= gh) return;
                if (bi < 0 || bj < 0 || bi >= gw || bj >= gh) return;
                glm::vec3 B = flag.positionArray[ai + aj * gw];
                glm::vec3 C = flag.positionArray[bi + bj * gw];
                glm::vec3 BxC = glm::cross(B - A, C - A);
                float l = glm::length(BxC);
                if (l > 0.0001f) N += BxC / l;
            };

            // 8 surrounding triangles (exact same pattern as original)
            if (i > 0 && j > 0) {
                addTriNormal(i - 1, j, i - 1, j - 1);
                addTriNormal(i - 1, j - 1, i, j - 1);
            }
            if (i < gw - 1 && j > 0) {
                addTriNormal(i, j - 1, i + 1, j - 1);
                addTriNormal(i + 1, j - 1, i + 1, j);
            }
            if (i < gw - 1 && j < gh - 1) {
                addTriNormal(i + 1, j, i + 1, j + 1);
                addTriNormal(i + 1, j + 1, i, j + 1);
            }
            if (i > 0 && j < gh - 1) {
                addTriNormal(i, j + 1, i - 1, j + 1);
                addTriNormal(i - 1, j + 1, i - 1, j);
            }

            verts[idx].normal = (N != glm::vec3(0.f)) ? glm::normalize(N) : glm::vec3(0.f);
        }
    }

    app.vk.CopyToBuffer(app.clothVB, verts.data(),
                         verts.size() * sizeof(ClothVertex));
}

// ============================================================================
// Cleanup
// ============================================================================
static void cleanupVulkanResources(AppState& app) {
    VkDevice device = app.vk.GetDevice();
    vkDeviceWaitIdle(device);

    app.vk.DestroyBuffer(app.clothVB);
    app.vk.DestroyBuffer(app.clothIB);
    app.vk.DestroyBuffer(app.sphereVB);

    vkDestroyPipeline(device, app.clothPipelineFill, nullptr);
    vkDestroyPipeline(device, app.clothPipelineWire, nullptr);
    vkDestroyPipeline(device, app.spherePipeline, nullptr);
    vkDestroyPipelineLayout(device, app.clothPipelineLayout, nullptr);
    vkDestroyPipelineLayout(device, app.spherePipelineLayout, nullptr);

    vkDestroyShaderModule(device, app.clothVertModule, nullptr);
    vkDestroyShaderModule(device, app.clothFragModule, nullptr);
    vkDestroyShaderModule(device, app.sphereVertModule, nullptr);
    vkDestroyShaderModule(device, app.sphereFragModule, nullptr);
}

// ============================================================================
// Recreate pipelines on swapchain resize
// ============================================================================
static void recreatePipelines(AppState& app) {
    VkDevice device = app.vk.GetDevice();

    vkDestroyPipeline(device, app.clothPipelineFill, nullptr);
    vkDestroyPipeline(device, app.clothPipelineWire, nullptr);
    vkDestroyPipeline(device, app.spherePipeline, nullptr);

    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = sizeof(ClothVertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].binding  = 0; attrs[0].location = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset   = offsetof(ClothVertex, position);
    attrs[1].binding  = 0; attrs[1].location = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset   = offsetof(ClothVertex, normal);

    std::vector<VkVertexInputBindingDescription> bindings = { binding };
    std::vector<VkVertexInputAttributeDescription> attributes = { attrs[0], attrs[1] };

    app.clothPipelineFill = createPipeline(device, app.vk.GetRenderPass(),
        app.clothPipelineLayout, app.clothVertModule, app.clothFragModule,
        bindings, attributes, VK_POLYGON_MODE_FILL);

    app.clothPipelineWire = createPipeline(device, app.vk.GetRenderPass(),
        app.clothPipelineLayout, app.clothVertModule, app.clothFragModule,
        bindings, attributes, VK_POLYGON_MODE_LINE);

    app.spherePipeline = createPipeline(device, app.vk.GetRenderPass(),
        app.spherePipelineLayout, app.sphereVertModule, app.sphereFragModule,
        bindings, attributes, VK_POLYGON_MODE_FILL);
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    // ---- Scene setup (same as original) ----
    SphereHandler sphereHandler;
    sphereHandler.colors    = {glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0)};
    sphereHandler.positions = {glm::vec3(0, -3, 2), glm::vec3(1.5, -3.5, 0.7), glm::vec3(3, -2, -1.5)};
    sphereHandler.radius    = {2.f, 1.5f, 0.8f};
    float sphereCollisionMultiplier = 1.5f;
    float radiusDelta = 0.15f;
    glm::ivec2 flagGrid = glm::ivec2(70, 30);
    glm::ivec2 flagSize = glm::ivec2(8, 3);
    float flagMass = 1.f;

    Flag flag(flagMass, static_cast<float>(flagSize.x), static_cast<float>(flagSize.y),
              flagGrid.x, flagGrid.y);
    glm::vec3 G(0.f, -0.05f, 0.f);

    float maxDstRepulseForce  = 0.17f;
    float multRepulseForce    = 0.1f;
    bool activeSpheres        = true;
    bool activeAutoCollisions = true;
    bool wireframe            = false;

    Octree<glm::vec3> octree(10, glm::vec3(0, 0, 0), glm::vec3(200.f));

    float windVelocity    = 0.025f;
    float newWindVelocity = windVelocity;

    // Smooth-interpolation targets for sphere movement (same pattern as original)
    float spherePosZ = sphereHandler.positions[0].z;
    float spherePosY = sphereHandler.positions[0].y;
    float moveStep   = 1.f;

    // ---- VulkanBase init ----
    AppState app;

    vkdemo::AppConfig config;
    config.Title           = "Flag Simulation (Vulkan)";
    config.Width           = WINDOW_WIDTH;
    config.Height          = WINDOW_HEIGHT;
    config.EnableValidation = true;
    config.EnableImGui      = true;

    app.vk.Init(config);

    // Camera setup
    app.camera.moveFront(12);

    // Projection matrix (will be updated on resize)
    auto computeProjection = [&]() -> glm::mat4 {
        float w = static_cast<float>(app.vk.GetWidth());
        float h = static_cast<float>(app.vk.GetHeight());
        if (h < 1.f) h = 1.f;
        return glm::perspective(glm::radians(70.f), w / h, 0.1f, 100.f);
    };

    // ---- GLFW callbacks ----
    app.vk.OnKey = [&](GLFWwindow* /*win*/, int key, int /*scancode*/, int action, int /*mods*/) {
        if (action != GLFW_PRESS) return;
        if (key == GLFW_KEY_SPACE) wireframe = !wireframe;
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(app.vk.GetWindow(), GLFW_TRUE);
        if (key == GLFW_KEY_KP_ADD)      { newWindVelocity += 0.02f; }
        if (key == GLFW_KEY_KP_SUBTRACT) { newWindVelocity -= 0.02f; newWindVelocity = glm::max(newWindVelocity, 0.f); }
        if (key == GLFW_KEY_UP)    { spherePosY += moveStep; }
        if (key == GLFW_KEY_DOWN)  { spherePosY -= moveStep; }
        if (key == GLFW_KEY_RIGHT) { spherePosZ -= moveStep; }
        if (key == GLFW_KEY_LEFT)  { spherePosZ += moveStep; }
    };

    app.vk.OnMouseButton = [&](GLFWwindow* win, int button, int action, int /*mods*/) {
        // Let ImGui capture first
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            app.leftButtonDown = (action == GLFW_PRESS);
            if (action == GLFW_PRESS) {
                glfwGetCursorPos(win, &app.mouseLastX, &app.mouseLastY);
            }
        }
    };

    app.vk.OnMouseMove = [&](GLFWwindow* /*win*/, double xpos, double ypos) {
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (app.leftButtonDown) {
            float dX = static_cast<float>(xpos - app.mouseLastX);
            float dY = static_cast<float>(ypos - app.mouseLastY);
            app.camera.rotateLeft(glm::radians(dX));
            app.camera.rotateUp(glm::radians(dY));
            app.mouseLastX = xpos;
            app.mouseLastY = ypos;
        }
    };

    app.vk.OnScroll = [&](GLFWwindow* /*win*/, double /*xoffset*/, double yoffset) {
        if (ImGui::GetIO().WantCaptureMouse) return;
        app.camera.moveFront(static_cast<float>(-yoffset) * 0.4f);
    };

    app.vk.OnResize = [&](int /*w*/, int /*h*/) {
        recreatePipelines(app);
    };

    // ---- Create Vulkan resources ----
    initVulkanResources(app);
    initClothBuffers(app, flag);
    initSphereBuffer(app);

    // ---- Time tracking ----
    auto lastTime = std::chrono::high_resolution_clock::now();

    // ============================================================
    // Main loop
    // ============================================================
    while (app.vk.BeginFrame()) {
        // Time delta
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        dt = std::min(dt, 0.05f); // cap to avoid physics spiral
        lastTime = now;

        VkCommandBuffer cmd = app.vk.GetCurrentCommandBuffer();

        // ---- ImGui ----
        app.vk.ImGuiNewFrame();

        ImGui::Begin("Parameters");
        ImGui::Text("Spring Constants");
        ImGui::SliderFloat("K0", &flag.K0, 0.f, 5.f);
        ImGui::SliderFloat("K1", &flag.K1, 0.f, 5.f);
        ImGui::SliderFloat("K2", &flag.K2, 0.f, 5.f);
        ImGui::Separator();
        ImGui::Text("Damping");
        ImGui::SliderFloat("V0", &flag.V0, 0.f, 1.f);
        ImGui::SliderFloat("V1", &flag.V1, 0.f, 1.f);
        ImGui::SliderFloat("V2", &flag.V2, 0.f, 1.f);
        ImGui::Separator();
        ImGui::Text("Sphere Collision");
        ImGui::SliderFloat("Collision Mult", &sphereCollisionMultiplier, 0.f, 5.f);
        ImGui::SliderFloat("Sphere Radius", &sphereHandler.radius[0], 0.1f, 5.f);
        ImGui::SliderFloat("Sphere X Pos", &sphereHandler.positions[0].x, -10.f, 10.f);
        ImGui::SliderFloat("Radius Delta", &radiusDelta, 0.f, 2.f);
        ImGui::Separator();
        ImGui::Text("Self-Collision");
        ImGui::SliderFloat("Max Repulse Dist", &maxDstRepulseForce, 0.01f, 1.f);
        ImGui::SliderFloat("Repulse Mult", &multRepulseForce, 0.f, 1.f);
        ImGui::Separator();
        ImGui::SliderFloat("Wind Velocity", &newWindVelocity, 0.f, 0.5f);
        ImGui::Checkbox("Spheres Active", &activeSpheres);
        ImGui::Checkbox("Auto-Collisions", &activeAutoCollisions);
        ImGui::Checkbox("Wireframe", &wireframe);

        if (ImGui::Button("Reset")) {
            Flag tmp(flagMass, static_cast<float>(flagSize.x), static_cast<float>(flagSize.y),
                     flagGrid.x, flagGrid.y);
            tmp.K0 = flag.K0; tmp.K1 = flag.K1; tmp.K2 = flag.K2;
            tmp.V0 = flag.V0; tmp.V1 = flag.V1; tmp.V2 = flag.V2;
            flag = tmp;
        }
        ImGui::End();

        // ---- Physics simulation (EXACT same logic as original) ----
        if (dt > 0.f) {
            flag.applyExternalForce(G);
            flag.applyExternalForce(glm::sphericalRand(windVelocity));
            flag.applyInternalForces(dt);

            if (activeSpheres)
                flag.applySphereCollision(sphereHandler, sphereCollisionMultiplier, radiusDelta);

            for (auto& pos : flag.positionArray)
                octree.add(pos, pos);

            if (activeAutoCollisions)
                flag.applyRepulseForces(octree, maxDstRepulseForce, multRepulseForce);

            for (auto& pos : flag.positionArray)
                octree.remove(pos, pos);

            flag.update(dt);
        }

        // Smooth interpolation towards target values (same as original)
        sphereHandler.positions[0].z = glm::mix(sphereHandler.positions[0].z, spherePosZ, 0.08f);
        sphereHandler.positions[0].y = glm::mix(sphereHandler.positions[0].y, spherePosY, 0.08f);
        windVelocity = glm::mix(windVelocity, newWindVelocity, 0.08f);

        // ---- Update cloth vertex buffer ----
        updateClothVertexBuffer(app, flag);

        // ---- Compute matrices ----
        glm::mat4 projection = computeProjection();
        glm::mat4 view = app.camera.getViewMatrix();
        glm::mat4 mvp = projection * view;
        glm::mat4 mv  = view;

        // ---- Draw cloth ----
        {
            VkPipeline pipeline = wireframe ? app.clothPipelineWire : app.clothPipelineFill;
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

            ClothPushConstants pc;
            pc.mvp = mvp;
            pc.mv  = mv;
            vkCmdPushConstants(cmd, app.clothPipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &app.clothVB.Buffer, &offset);
            vkCmdBindIndexBuffer(cmd, app.clothIB.Buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cmd, app.clothIndexCount, 1, 0, 0, 0);
        }

        // ---- Draw spheres ----
        if (activeSpheres) {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, app.spherePipeline);

            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &app.sphereVB.Buffer, &offset);

            for (size_t i = 0; i < sphereHandler.positions.size(); ++i) {
                glm::mat4 modelMatrix = glm::translate(glm::mat4(1.f), sphereHandler.positions[i]);
                modelMatrix = glm::scale(modelMatrix, glm::vec3(sphereHandler.radius[i]));

                SpherePushConstants pc;
                pc.mvp   = projection * view * modelMatrix;
                pc.color = glm::vec4(sphereHandler.colors[i], 1.f);

                vkCmdPushConstants(cmd, app.spherePipelineLayout,
                                   VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
                vkCmdDraw(cmd, app.sphereVertexCount, 1, 0, 0);
            }
        }

        // ---- ImGui render ----
        app.vk.ImGuiRender(cmd);

        app.vk.EndFrame();
    }

    // ---- Cleanup ----
    cleanupVulkanResources(app);
    app.vk.Shutdown();

    return EXIT_SUCCESS;
}
