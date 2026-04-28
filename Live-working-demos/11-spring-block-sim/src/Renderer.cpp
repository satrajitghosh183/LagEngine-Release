#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <shaderc/shaderc.hpp>
#include <iostream>
#include <algorithm>
#include <array>
#include <stdexcept>

// ─────────────────────────────────────────────────────────────────────────────
// Cube vertex data  (pos, normal, uv)
// ─────────────────────────────────────────────────────────────────────────────
static const float CUBE_VERTS[] = {
    // positions          normals            UVs
    // -Z face
    -1,-1,-1,  0, 0,-1,  0,0,
     1,-1,-1,  0, 0,-1,  1,0,
     1, 1,-1,  0, 0,-1,  1,1,
    -1, 1,-1,  0, 0,-1,  0,1,
    // +Z
    -1,-1, 1,  0, 0, 1,  0,0,
     1,-1, 1,  0, 0, 1,  1,0,
     1, 1, 1,  0, 0, 1,  1,1,
    -1, 1, 1,  0, 0, 1,  0,1,
    // -X
    -1,-1,-1, -1, 0, 0,  0,0,
    -1, 1,-1, -1, 0, 0,  1,0,
    -1, 1, 1, -1, 0, 0,  1,1,
    -1,-1, 1, -1, 0, 0,  0,1,
    // +X
     1,-1,-1,  1, 0, 0,  0,0,
     1, 1,-1,  1, 0, 0,  1,0,
     1, 1, 1,  1, 0, 0,  1,1,
     1,-1, 1,  1, 0, 0,  0,1,
    // -Y
    -1,-1,-1,  0,-1, 0,  0,0,
     1,-1,-1,  0,-1, 0,  1,0,
     1,-1, 1,  0,-1, 0,  1,1,
    -1,-1, 1,  0,-1, 0,  0,1,
    // +Y
    -1, 1,-1,  0, 1, 0,  0,0,
     1, 1,-1,  0, 1, 0,  1,0,
     1, 1, 1,  0, 1, 0,  1,1,
    -1, 1, 1,  0, 1, 0,  0,1,
};
static const uint32_t CUBE_IDX[] = {
     0, 1, 2,  0, 2, 3,
     4, 5, 6,  4, 6, 7,
     8, 9,10,  8,10,11,
    12,13,14, 12,14,15,
    16,17,18, 16,18,19,
    20,21,22, 20,22,23,
};

// ─────────────────────────────────────────────────────────────────────────────
// Ground quad (large flat plane)
// ─────────────────────────────────────────────────────────────────────────────
static const float GROUND_VERTS[] = {
    // pos           normal     uv
    -50, 0,-50,  0,1,0,   0,  0,
     50, 0,-50,  0,1,0,  50,  0,
     50, 0, 50,  0,1,0,  50, 50,
    -50, 0, 50,  0,1,0,   0, 50,
};
static const uint32_t GROUND_IDX[] = { 0,1,2, 0,2,3 };

// ─────────────────────────────────────────────────────────────────────────────
// Embedded GLSL shaders (Vulkan GLSL #version 450)
// ─────────────────────────────────────────────────────────────────────────────

static const std::string PBR_VERT = R"(
#version 450

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
    vec4 albedo;       // xyz=albedo, w=ao
    vec4 params;       // x=metallic, y=roughness, z=selectedBrightness
    vec4 camPos;
    vec4 lightDir;
    vec4 lightColor;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos   = pc.model * vec4(inPos, 1.0);
    fragWorldPos    = worldPos.xyz;
    fragNormal      = normalize(mat3(transpose(inverse(pc.model))) * inNormal);
    fragTexCoord    = inTexCoord;
    gl_Position     = pc.mvp * vec4(inPos, 1.0);
}
)";

static const std::string PBR_FRAG = R"(
#version 450

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
    vec4 albedoAO;     // xyz=albedo, w=ao
    vec4 params;       // x=metallic, y=roughness, z=selectedBrightness
    vec4 camPos;
    vec4 lightDir;
    vec4 lightColor;
} pc;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float rough) {
    float a  = rough * rough;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float rough) {
    float r = rough + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float rough) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, rough) * GeometrySchlickGGX(NdotL, rough);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 evalLight(vec3 N, vec3 V, vec3 L, vec3 radiance, vec3 F0,
               vec3 albedo, float metallic, float roughness) {
    vec3  H     = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    if (NdotL <= 0.0) return vec3(0.0);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3  numerator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * NdotL + 0.0001;
    vec3  specular    = numerator / denominator;

    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
    vec3  albedo    = pc.albedoAO.xyz;
    float ao        = pc.albedoAO.w;
    float metallic  = pc.params.x;
    float roughness = pc.params.y;
    float selected  = pc.params.z;

    vec3 N  = normalize(fragNormal);
    vec3 V  = normalize(pc.camPos.xyz - fragWorldPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);

    // Directional light
    {
        vec3 L        = normalize(-pc.lightDir.xyz);
        vec3 radiance = pc.lightColor.xyz;
        Lo += evalLight(N, V, L, radiance, F0, albedo, metallic, roughness);
    }

    // Ambient
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color   = ambient + Lo;

    // Selection rim glow
    if (selected > 0.0) {
        float rim = 1.0 - max(dot(N, V), 0.0);
        rim = pow(rim, 3.0);
        color += rim * vec3(0.3, 0.7, 1.0) * selected * 2.0;
    }

    // Simple tonemap + gamma (replaces the multi-pass bloom/tonemap)
    color *= 1.1;  // exposure
    color = color / (color + vec3(1.0));  // Reinhard
    color = pow(color, vec3(1.0 / 2.2));  // gamma

    outColor = vec4(color, 1.0);
}
)";

static const std::string GROUND_VERT = R"(
#version 450

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
    vec4 albedo;
    vec4 params;
    vec4 camPos;
    vec4 lightDir;
    vec4 lightColor;
} pc;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragWorldPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    fragWorldPos  = worldPos.xyz;
    fragNormal    = vec3(0.0, 1.0, 0.0);
    fragTexCoord  = inTexCoord;
    gl_Position   = pc.mvp * vec4(inPos, 1.0);
}
)";

static const std::string GROUND_FRAG = R"(
#version 450

layout(push_constant) uniform PC {
    mat4 mvp;
    mat4 model;
    vec4 albedoAO;
    vec4 params;
    vec4 camPos;
    vec4 lightDir;
    vec4 lightColor;
} pc;

layout(location = 0) in vec3 fragWorldPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

vec3 checkerAlbedo(vec2 uv) {
    float scale = 1.0;
    vec2  grid  = floor(uv * scale);
    float c     = mod(grid.x + grid.y, 2.0);
    return mix(vec3(0.18), vec3(0.28), c);
}

float DistGGX(vec3 N, vec3 H, float r) {
    float a = r*r; float a2 = a*a;
    float d = max(dot(N,H),0.0); d = d*d;
    float b = d*(a2-1.0)+1.0;
    return a2/(PI*b*b);
}

float GeoSmith(float ndotv, float ndotl, float r) {
    float k = (r+1.0)*(r+1.0)/8.0;
    float gv = ndotv/(ndotv*(1.0-k)+k);
    float gl = ndotl/(ndotl*(1.0-k)+k);
    return gv*gl;
}

vec3 FresnelS(float c, vec3 F0) {
    return F0 + (1.0-F0)*pow(clamp(1.0-c,0.0,1.0),5.0);
}

void main() {
    vec3  N         = vec3(0.0, 1.0, 0.0);
    vec3  V         = normalize(pc.camPos.xyz - fragWorldPos);
    vec3  L         = normalize(-pc.lightDir.xyz);
    vec3  H         = normalize(V + L);

    vec3  albedo    = checkerAlbedo(fragWorldPos.xz);
    float roughness = 0.8;
    float metallic  = 0.0;

    vec3  F0    = mix(vec3(0.04), albedo, metallic);
    float NdotL = max(dot(N, L), 0.0);

    float NDF   = DistGGX(N, H, roughness);
    float G     = GeoSmith(max(dot(N,V),0.0), NdotL, roughness);
    vec3  F     = FresnelS(max(dot(H,V),0.0), F0);
    vec3  spec  = NDF*G*F / (4.0*max(dot(N,V),0.0)*NdotL + 0.0001);
    vec3  kD    = (1.0-F)*(1.0-metallic);
    vec3  diffuse = kD * albedo / PI;

    vec3 radiance = pc.lightColor.xyz;
    vec3 color    = (diffuse + spec) * radiance * NdotL + vec3(0.02)*albedo;

    // Simple tonemap + gamma
    color *= 1.1;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
)";

// ─────────────────────────────────────────────────────────────────────────────
// Runtime GLSL -> SPIR-V via shaderc
// ─────────────────────────────────────────────────────────────────────────────
VkShaderModule Renderer::compileGLSL(const std::string& source,
                                     const std::string& name,
                                     bool isVertex) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);

    auto kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;
    auto result = compiler.CompileGlslToSpv(source, kind, name.c_str(), options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << "[Renderer] Shader compilation failed (" << name << "):\n"
                  << result.GetErrorMessage() << "\n";
        throw std::runtime_error("Shader compilation failed: " + name);
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    return m_vkBase->CreateShaderModule(spirv);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pipeline creation helper
// ─────────────────────────────────────────────────────────────────────────────
static VkPipeline createGraphicsPipeline(
    VkDevice device,
    VkRenderPass renderPass,
    VkPipelineLayout layout,
    VkShaderModule vertModule,
    VkShaderModule fragModule,
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL)
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

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    // Vertex input: pos(3) + normal(3) + uv(2) = 8 floats, stride 32
    VkVertexInputBindingDescription binding{};
    binding.binding   = 0;
    binding.stride    = 8 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 3> attrs{};
    // location 0: position (vec3)
    attrs[0].binding  = 0;
    attrs[0].location = 0;
    attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset   = 0;
    // location 1: normal (vec3)
    attrs[1].binding  = 0;
    attrs[1].location = 1;
    attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset   = 3 * sizeof(float);
    // location 2: texcoord (vec2)
    attrs[2].binding  = 0;
    attrs[2].location = 2;
    attrs[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset   = 6 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount   = 1;
    vertexInput.pVertexBindingDescriptions      = &binding;
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
    vertexInput.pVertexAttributeDescriptions    = attrs.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = polygonMode;
    raster.cullMode    = VK_CULL_MODE_BACK_BIT;
    raster.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    raster.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthTestEnable  = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp   = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState blendAtt{};
    blendAtt.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blend{};
    blend.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend.attachmentCount = 1;
    blend.pAttachments    = &blendAtt;

    std::array<VkDynamicState, 2> dynStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynState{};
    dynState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynState.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynState.pDynamicStates    = dynStates.data();

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount          = 2;
    pci.pStages             = stages;
    pci.pVertexInputState   = &vertexInput;
    pci.pInputAssemblyState = &inputAssembly;
    pci.pViewportState      = &viewportState;
    pci.pRasterizationState = &raster;
    pci.pMultisampleState   = &msaa;
    pci.pDepthStencilState  = &depth;
    pci.pColorBlendState    = &blend;
    pci.pDynamicState       = &dynState;
    pci.layout              = layout;
    pci.renderPass          = renderPass;
    pci.subpass             = 0;

    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pci, nullptr, &pipeline) != VK_SUCCESS)
        throw std::runtime_error("Failed to create graphics pipeline");
    return pipeline;
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::createPipelines() {
    VkDevice device = m_vkBase->GetDevice();

    // Push constant range: all 208 bytes visible to both vert + frag
    VkPushConstantRange pcRange{};
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pcRange.offset     = 0;
    pcRange.size       = sizeof(PBRPushConstants);

    m_pbrPipelineLayout = m_vkBase->CreatePipelineLayout({}, { pcRange });

    // Compile shaders
    VkShaderModule pbrVert    = compileGLSL(PBR_VERT,    "pbr.vert",    true);
    VkShaderModule pbrFrag    = compileGLSL(PBR_FRAG,    "pbr.frag",    false);
    VkShaderModule groundVert = compileGLSL(GROUND_VERT, "ground.vert", true);
    VkShaderModule groundFrag = compileGLSL(GROUND_FRAG, "ground.frag", false);

    // PBR pipeline (filled)
    m_pbrPipeline = createGraphicsPipeline(
        device, m_vkBase->GetRenderPass(), m_pbrPipelineLayout,
        pbrVert, pbrFrag, VK_POLYGON_MODE_FILL);

    // PBR pipeline (wireframe)
    m_pbrWireframePipeline = createGraphicsPipeline(
        device, m_vkBase->GetRenderPass(), m_pbrPipelineLayout,
        pbrVert, pbrFrag, VK_POLYGON_MODE_LINE);

    // Ground pipeline (same layout, different shaders)
    m_groundPipeline = createGraphicsPipeline(
        device, m_vkBase->GetRenderPass(), m_pbrPipelineLayout,
        groundVert, groundFrag, VK_POLYGON_MODE_FILL);

    // Cleanup shader modules (no longer needed after pipeline creation)
    vkDestroyShaderModule(device, pbrVert,    nullptr);
    vkDestroyShaderModule(device, pbrFrag,    nullptr);
    vkDestroyShaderModule(device, groundVert, nullptr);
    vkDestroyShaderModule(device, groundFrag, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::initCubeBuffers() {
    m_cubeVertexBuffer = m_vkBase->CreateVertexBuffer(CUBE_VERTS, sizeof(CUBE_VERTS));
    m_cubeIndexBuffer  = m_vkBase->CreateIndexBuffer(CUBE_IDX,   sizeof(CUBE_IDX));
}

void Renderer::initGroundBuffers() {
    m_groundVertexBuffer = m_vkBase->CreateVertexBuffer(GROUND_VERTS, sizeof(GROUND_VERTS));
    m_groundIndexBuffer  = m_vkBase->CreateIndexBuffer(GROUND_IDX,   sizeof(GROUND_IDX));
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::init(vkdemo::VulkanBase* vkBase) {
    m_vkBase = vkBase;
    m_w = vkBase->GetWidth();
    m_h = vkBase->GetHeight();

    createPipelines();
    initCubeBuffers();
    initGroundBuffers();
}

void Renderer::resize(int width, int height) {
    m_w = width;
    m_h = height;
    // Pipelines use dynamic viewport/scissor, so no recreation needed.
    // But we need to recreate pipelines if the render pass changed.
    VkDevice device = m_vkBase->GetDevice();
    vkDestroyPipeline(device, m_pbrPipeline, nullptr);
    vkDestroyPipeline(device, m_pbrWireframePipeline, nullptr);
    vkDestroyPipeline(device, m_groundPipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pbrPipelineLayout, nullptr);
    createPipelines();
}

void Renderer::shutdown() {
    VkDevice device = m_vkBase->GetDevice();
    vkDeviceWaitIdle(device);

    m_springMeshes.clear();

    m_vkBase->DestroyBuffer(m_cubeVertexBuffer);
    m_vkBase->DestroyBuffer(m_cubeIndexBuffer);
    m_vkBase->DestroyBuffer(m_groundVertexBuffer);
    m_vkBase->DestroyBuffer(m_groundIndexBuffer);

    vkDestroyPipeline(device, m_pbrPipeline, nullptr);
    vkDestroyPipeline(device, m_pbrWireframePipeline, nullptr);
    vkDestroyPipeline(device, m_groundPipeline, nullptr);
    vkDestroyPipelineLayout(device, m_pbrPipelineLayout, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw blocks
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawBlocks(VkCommandBuffer cmd, const PhysicsWorld& world,
                          const Camera& cam, int selBlock) {
    float aspect = (float)m_w / (float)m_h;
    glm::mat4 view = cam.viewMatrix(aspect);
    glm::mat4 proj = cam.projMatrix(aspect);
    // Vulkan clip space: flip Y
    proj[1][1] *= -1.0f;
    glm::mat4 vp = proj * view;

    VkPipeline pipeline = cfg.wireframe ? m_pbrWireframePipeline : m_pbrPipeline;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_cubeVertexBuffer.Buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_cubeIndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

    for (int bi = 0; bi < (int)world.blocks.size(); ++bi) {
        const Block& b = world.blocks[bi];
        glm::mat4 model = glm::translate(glm::mat4(1), b.position);
        model = glm::scale(model, b.halfExtents);

        PBRPushConstants pc{};
        pc.mvp        = vp * model;
        pc.model      = model;
        pc.albedo     = glm::vec4(b.albedo, b.ao);
        pc.params     = glm::vec4(b.metallic, b.roughness,
                                  (bi == selBlock) ? 1.0f : 0.0f, 0.0f);
        pc.camPos     = glm::vec4(cam.position(), 0.0f);
        pc.lightDir   = glm::vec4(m_lightDir, 0.0f);
        pc.lightColor = glm::vec4(m_lightColor, 0.0f);

        vkCmdPushConstants(cmd, m_pbrPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PBRPushConstants), &pc);

        vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw springs
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawSprings(VkCommandBuffer cmd, const PhysicsWorld& world,
                           const Camera& cam, int selSpring) {
    float aspect = (float)m_w / (float)m_h;
    glm::mat4 view = cam.viewMatrix(aspect);
    glm::mat4 proj = cam.projMatrix(aspect);
    proj[1][1] *= -1.0f;
    glm::mat4 vp = proj * view;

    VkPipeline pipeline = cfg.wireframe ? m_pbrWireframePipeline : m_pbrPipeline;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    resizeSpringMeshes((int)world.springs.size());
    for (int si = 0; si < (int)world.springs.size(); ++si) {
        const Spring& s = world.springs[si];
        glm::vec3 pA = world.springEndA(si);
        glm::vec3 pB = world.springEndB(si);
        m_springMeshes[si].rebuild(pA, pB);

        if (!m_springMeshes[si].ready()) continue;

        glm::mat4 identity = glm::mat4(1);

        PBRPushConstants pc{};
        pc.mvp        = vp;  // model is identity; spring verts are in world space
        pc.model      = identity;
        pc.albedo     = glm::vec4(s.albedo, 1.0f);
        pc.params     = glm::vec4(s.metallic, s.roughness,
                                  (si == selSpring) ? 1.0f : 0.0f, 0.0f);
        pc.camPos     = glm::vec4(cam.position(), 0.0f);
        pc.lightDir   = glm::vec4(m_lightDir, 0.0f);
        pc.lightColor = glm::vec4(m_lightColor, 0.0f);

        vkCmdPushConstants(cmd, m_pbrPipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0, sizeof(PBRPushConstants), &pc);

        m_springMeshes[si].draw(cmd);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw ground
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::drawGround(VkCommandBuffer cmd, const Camera& cam) {
    float aspect = (float)m_w / (float)m_h;
    glm::mat4 view = cam.viewMatrix(aspect);
    glm::mat4 proj = cam.projMatrix(aspect);
    proj[1][1] *= -1.0f;
    glm::mat4 vp = proj * view;

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_groundPipeline);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_groundVertexBuffer.Buffer, &offset);
    vkCmdBindIndexBuffer(cmd, m_groundIndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

    glm::mat4 identity = glm::mat4(1);

    PBRPushConstants pc{};
    pc.mvp        = vp;
    pc.model      = identity;
    pc.albedo     = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);  // unused by ground shader
    pc.params     = glm::vec4(0.0f);
    pc.camPos     = glm::vec4(cam.position(), 0.0f);
    pc.lightDir   = glm::vec4(m_lightDir, 0.0f);
    pc.lightColor = glm::vec4(m_lightColor, 0.0f);

    vkCmdPushConstants(cmd, m_pbrPipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0, sizeof(PBRPushConstants), &pc);

    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Main render entry
// ─────────────────────────────────────────────────────────────────────────────
void Renderer::render(VkCommandBuffer cmd, const PhysicsWorld& world,
                      const Camera& cam, int selectedBlock, int selectedSpring) {
    drawGround(cmd, cam);
    drawBlocks(cmd, world, cam, selectedBlock);
    drawSprings(cmd, world, cam, selectedSpring);
}

// ─────────────────────────────────────────────────────────────────────────────
void Renderer::resizeSpringMeshes(int n) {
    while ((int)m_springMeshes.size() < n) {
        m_springMeshes.emplace_back();
        m_springMeshes.back().init(m_vkBase, 10, 14, 0.12f, 0.035f);
    }
}
