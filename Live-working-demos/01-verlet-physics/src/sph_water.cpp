// Single-file 3D SPH Water (CPU) + Vulkan billboard rendering via VulkanBase
#include <VulkanBase.hpp>
#include <EmbeddedShaders.hpp>

#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifdef HAS_SHADERC
#include <shaderc/shaderc.hpp>
#endif

static int   WIN_W = 1280, WIN_H = 800;
static bool  paused = false;

// ---------- Camera ----------
struct Camera {
    glm::vec3 target = glm::vec3(0.0f, 0.6f, 0.0f);
    float dist = 2.2f;
    float yaw = 1.9f, pitch = -0.25f;
    float fov = 45.0f;
    glm::vec3 eye() const {
        float cy = std::cos(yaw), sy = std::sin(yaw);
        float cp = std::cos(pitch), sp = std::sin(pitch);
        glm::vec3 d = {cp * cy, sp, cp * sy};
        return target - d * dist;
    }
} cam;

static bool lmb = false, rmb = false;
static double lastX = 0, lastY = 0;

// ---------- SPH ----------
struct SPH {
    int   N = 18000;
    float h = 0.045f;
    float rest = 1000.0f;
    float k = 1.6f;
    float visc = 0.15f;
    float dt = 0.004f;
    glm::vec3 gravity = {0, -9.81f, 0};
    float radius = 0.014f;
    float bounce = 0.4f;

    glm::vec3 minB = {-0.6f, 0.0f, -0.3f};
    glm::vec3 maxB = { 0.6f, 1.0f,  0.3f};

    std::vector<glm::vec3> x, v, f;
    std::vector<float> rho, P;

    int gx, gy, gz;
    float cell;
    std::vector<int> head, next;

    SPH() { init(); }

    void init() {
        int nx = 36, ny = 25, nz = 18;
        N = nx * ny * nz;
        x.resize(N);
        v.assign(N, glm::vec3(0));
        f.assign(N, glm::vec3(0));
        rho.resize(N, rest);
        P.resize(N);

        cell = h;
        gx = (int)std::ceil((maxB.x - minB.x) / cell);
        gy = (int)std::ceil((maxB.y - minB.y) / cell);
        gz = (int)std::ceil((maxB.z - minB.z) / cell);
        head.assign(gx * gy * gz, -1);
        next.resize(N);

        glm::vec3 start = {minB.x + 0.1f, minB.y + 0.1f, minB.z + 0.08f};
        glm::vec3 step = {0.028f, 0.028f, 0.028f};
        int i = 0;
        for (int iz = 0; iz < nz; ++iz)
        for (int iy = 0; iy < ny; ++iy)
        for (int ix = 0; ix < nx; ++ix) {
            x[i] = {start.x + ix * step.x, start.y + iy * step.y, start.z + iz * step.z};
            ++i;
        }
    }

    inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
    inline int hash3(int ix, int iy, int iz) {
        ix = clampi(ix, 0, gx - 1);
        iy = clampi(iy, 0, gy - 1);
        iz = clampi(iz, 0, gz - 1);
        return ix + iy * gx + iz * gx * gy;
    }
    inline void cellOf(const glm::vec3& p, int& ix, int& iy, int& iz) {
        ix = (int)std::floor((p.x - minB.x) / cell);
        iy = (int)std::floor((p.y - minB.y) / cell);
        iz = (int)std::floor((p.z - minB.z) / cell);
    }
    void buildGrid() {
        std::fill(head.begin(), head.end(), -1);
        for (int i = 0; i < N; ++i) {
            int ix, iy, iz;
            cellOf(x[i], ix, iy, iz);
            int hsh = hash3(ix, iy, iz);
            next[i] = head[hsh];
            head[hsh] = i;
        }
    }

    float Wpoly6(float r2) {
        float c = (315.0f / (64.0f * (float)M_PI * std::pow(h, 9)));
        float t = h * h - r2;
        if (t <= 0) return 0.0f;
        return c * t * t * t;
    }
    glm::vec3 gradSpiky(const glm::vec3& rij, float r) {
        if (r <= 0 || r >= h) return glm::vec3(0);
        float c = -45.0f / ((float)M_PI * std::pow(h, 6));
        float s = c * ((h - r) * (h - r)) / r;
        return rij * s;
    }
    float lapVisc(float r) {
        if (r >= h) return 0.0f;
        float c = 45.0f / ((float)M_PI * std::pow(h, 6));
        return c * (h - r);
    }

    void step() {
        buildGrid();

        // density
        std::fill(rho.begin(), rho.end(), 0.0f);
        for (int i = 0; i < N; ++i) {
            int ix, iy, iz;
            cellOf(x[i], ix, iy, iz);
            for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int hsh = hash3(ix + dx, iy + dy, iz + dz);
                for (int j = head[hsh]; j != -1; j = next[j]) {
                    glm::vec3 rij = x[i] - x[j];
                    float r2 = glm::dot(rij, rij);
                    rho[i] += Wpoly6(r2);
                }
            }
            rho[i] = std::max(rho[i], rest * 0.5f);
            P[i] = k * (rho[i] - rest);
        }

        // forces
        std::fill(f.begin(), f.end(), glm::vec3(0));
        for (int i = 0; i < N; ++i) {
            int ix, iy, iz;
            cellOf(x[i], ix, iy, iz);
            glm::vec3 fP(0), fV(0);
            for (int dz = -1; dz <= 1; ++dz)
            for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                int hsh = hash3(ix + dx, iy + dy, iz + dz);
                for (int j = head[hsh]; j != -1; j = next[j]) {
                    if (j == i) continue;
                    glm::vec3 rij = x[i] - x[j];
                    float r = glm::length(rij);
                    if (r >= h || r < 1e-6f) continue;
                    glm::vec3 gW = gradSpiky(rij, r);
                    float viscL = lapVisc(r);
                    fP = fP - gW * ((P[i] + P[j]) / (2.0f * rho[j]));
                    fV = fV + (v[j] - v[i]) * (visc * viscL / rho[j]);
                }
            }
            f[i] = fP + fV + gravity;
        }

        // integrate + collisions
        const float dtc = dt;
        for (int i = 0; i < N; ++i) {
            v[i] = v[i] + f[i] * dtc;
            x[i] = x[i] + v[i] * dtc;

            if (x[i].x < minB.x) { x[i].x = minB.x; v[i].x *= -bounce; }
            if (x[i].x > maxB.x) { x[i].x = maxB.x; v[i].x *= -bounce; }
            if (x[i].y < minB.y) { x[i].y = minB.y; v[i].y *= -bounce; }
            if (x[i].y > maxB.y) { x[i].y = maxB.y; v[i].y *= -bounce; }
            if (x[i].z < minB.z) { x[i].z = minB.z; v[i].z *= -bounce; }
            if (x[i].z > maxB.z) { x[i].z = maxB.z; v[i].z *= -bounce; }
        }
    }
} sph;

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

// ---------- Push constants for point rendering ----------
struct PointPushConstants {
    glm::mat4 mvp;
    float pointSize;
};

// ---------- Point pipeline (for particles) ----------
static VkPipeline CreatePointPipeline(vkdemo::VulkanBase& vk,
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
    attrs[0].location = 0; attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[0].offset = 0;
    attrs[1].location = 1; attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT; attrs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ia{};
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

    VkPipelineViewportStateCreateInfo vpState{};
    vpState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpState.viewportCount = 1; vpState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo raster{};
    raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
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
    blendAttach.blendEnable = VK_TRUE;
    blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;
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
    ci.pViewportState = &vpState;
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

// ---------- Vertex for particle (pos + color) ----------
struct ParticleVertex {
    glm::vec3 pos;
    glm::vec3 color;
};

int main() {
    vkdemo::VulkanBase vk;
    vkdemo::AppConfig config;
    config.Title = "SPH Water (Vulkan) - LMB orbit | RMB pan | Scroll zoom | Space pause";
    config.Width = WIN_W;
    config.Height = WIN_H;
    config.EnableImGui = false;
    config.EnableValidation = true;
    vk.Init(config);

    // ---- Callbacks ----
    vk.OnKey = [&](GLFWwindow* w, int key, int, int action, int) {
        if (action != GLFW_PRESS) return;
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, 1);
        if (key == GLFW_KEY_SPACE) paused = !paused;
    };
    vk.OnMouseButton = [&](GLFWwindow*, int b, int a, int) {
        if (b == GLFW_MOUSE_BUTTON_LEFT) lmb = (a != GLFW_RELEASE);
        if (b == GLFW_MOUSE_BUTTON_RIGHT) rmb = (a != GLFW_RELEASE);
    };
    vk.OnMouseMove = [&](GLFWwindow*, double x, double y) {
        double dx = x - lastX, dy = y - lastY;
        lastX = x; lastY = y;
        if (lmb) {
            cam.yaw -= (float)dx * 0.005f;
            cam.pitch -= (float)dy * 0.005f;
            cam.pitch = std::max(-1.55f, std::min(1.2f, cam.pitch));
        }
        if (rmb) {
            glm::vec3 fwd = {std::cos(cam.yaw), 0, std::sin(cam.yaw)};
            glm::vec3 rt = {-fwd.z, 0, fwd.x};
            cam.target = cam.target + rt * (float)(-dx * 0.002f * cam.dist) + fwd * (float)(dy * 0.002f * cam.dist);
        }
    };
    vk.OnScroll = [&](GLFWwindow*, double, double o) {
        cam.dist = std::max(0.5f, std::min(6.0f, cam.dist * (float)std::pow(0.9, o)));
    };
    vk.OnResize = [&](int w, int h) {
        WIN_W = w; WIN_H = h;
    };

    // ---- Compile shaders ----
#ifdef HAS_SHADERC
    auto vertSpv = CompileGLSL(vkdemo::kPointVertexShader, shaderc_vertex_shader, "point.vert");
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
    pushRange.size = sizeof(PointPushConstants);
    VkPipelineLayout pipelineLayout = vk.CreatePipelineLayout({}, {pushRange});

    VkPipeline pointPipeline = CreatePointPipeline(vk, vertModule, fragModule, pipelineLayout);

    // We need to recreate on resize
    auto recreatePipeline = [&]() {
        vkDestroyPipeline(vk.GetDevice(), pointPipeline, nullptr);
        pointPipeline = CreatePointPipeline(vk, vertModule, fragModule, pipelineLayout);
    };
    vk.OnResize = [&](int w, int h) {
        WIN_W = w; WIN_H = h;
        recreatePipeline();
    };

    // ---- Vertex buffer for particles ----
    VkDeviceSize bufSize = sph.N * sizeof(ParticleVertex);
    std::vector<vkdemo::GPUBuffer> particleBufs(2);
    for (int i = 0; i < 2; ++i) {
        particleBufs[i] = vk.CreateBuffer(bufSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    // Pre-allocate vertex data
    std::vector<ParticleVertex> particleVerts(sph.N);

    while (true) {
        if (!vk.BeginFrame()) break;

        // Physics
        if (!paused) {
            const int sub = 2;
            for (int s = 0; s < sub; ++s)
                sph.step();
        }

        // Build camera matrices
        glm::vec3 eye = cam.eye();
        glm::mat4 V = glm::lookAt(eye, cam.target, glm::vec3(0, 1, 0));
        glm::mat4 P = glm::perspective(glm::radians(cam.fov),
            (float)WIN_W / (float)WIN_H, 0.02f, 20.0f);
        P[1][1] *= -1.0f; // Vulkan Y flip
        glm::mat4 mvp = P * V;

        // Color particles: deep blue fading to lighter at surface
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.6f, 0.8f, 0.3f));
        for (int i = 0; i < sph.N; ++i) {
            particleVerts[i].pos = sph.x[i];
            // Simple depth-based coloring
            float ht = (sph.x[i].y - sph.minB.y) / (sph.maxB.y - sph.minB.y);
            glm::vec3 deepBlue = {0.02f, 0.10f, 0.9f};
            glm::vec3 lightBlue = {0.3f, 0.6f, 1.0f};
            particleVerts[i].color = glm::mix(deepBlue, lightBlue, ht);
        }

        VkCommandBuffer cmd = vk.GetCurrentCommandBuffer();
        uint32_t frameIdx = vk.GetFrameIndex();

        // Upload
        vk.CopyToBuffer(particleBufs[frameIdx], particleVerts.data(), bufSize);

        PointPushConstants pc;
        pc.mvp = mvp;
        pc.pointSize = std::max(1.0f, sph.radius * 1400.0f / cam.dist);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline);
        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &particleBufs[frameIdx].Buffer, &offset);
        vkCmdDraw(cmd, sph.N, 1, 0, 0);

        vk.EndFrame();
    }

    vkDeviceWaitIdle(vk.GetDevice());

    for (int i = 0; i < 2; ++i)
        vk.DestroyBuffer(particleBufs[i]);
    vkDestroyPipeline(vk.GetDevice(), pointPipeline, nullptr);
    vkDestroyPipelineLayout(vk.GetDevice(), pipelineLayout, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), vertModule, nullptr);
    vkDestroyShaderModule(vk.GetDevice(), fragModule, nullptr);

    return 0;
}
