//  SpringBlockSim – main.cpp (Vulkan)
//  Hyperrealistic spring-block simulator with PBR rendering
//  ─────────────────────────────────────────────────────────────────────────
//  Controls:
//    Right-click drag   – orbit camera
//    Middle-click drag  – pan camera
//    Scroll wheel       – zoom
//    Left click         – select block (ray-cast)
//    Left drag on block – move block (horizontal plane)
// ─────────────────────────────────────────────────────────────────────────────

#include <VulkanBase.hpp>

#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>

#include "Physics.h"
#include "Renderer.h"
#include "Camera.h"
#include "UI.h"

// ─────────────────────────────────────────────────────────────────────────────
// Globals (simple for a demo)
// ─────────────────────────────────────────────────────────────────────────────
static int  g_width  = 1600;
static int  g_height = 900;

static Camera       g_cam;
static PhysicsWorld g_world;
static Renderer     g_renderer;
static UI           g_ui;

static int  g_selBlock  = -1;
static int  g_selSpring = -1;

// Mouse state
static bool  g_rightDown  = false;
static bool  g_midDown    = false;
static bool  g_leftDown   = false;
static double g_prevX = 0, g_prevY = 0;

// Drag (block dragging)
static bool g_draggingBlock = false;
static float g_dragPlaneY  = 0.0f;

// ─────────────────────────────────────────────────────────────────────────────
// Ray -> AABB intersection (slab method)
// Returns t (distance along ray) or -1 if no hit
// ─────────────────────────────────────────────────────────────────────────────
static float rayAABB(const glm::vec3& ro, const glm::vec3& rd,
                     const glm::vec3& bmin, const glm::vec3& bmax) {
    float tmin = 0.001f, tmax = 1e30f;
    for (int i = 0; i < 3; ++i) {
        if (std::abs(rd[i]) < 1e-8f) {
            if (ro[i] < bmin[i] || ro[i] > bmax[i]) return -1.0f;
        } else {
            float t1 = (bmin[i] - ro[i]) / rd[i];
            float t2 = (bmax[i] - ro[i]) / rd[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            if (tmin > tmax) return -1.0f;
        }
    }
    return tmin;
}

// Unproject a screen pixel into a world-space ray
static void screenRay(double px, double py,
                      glm::vec3& rayOrigin, glm::vec3& rayDir) {
    float aspect = (float)g_width / (float)g_height;
    glm::mat4 proj = g_cam.projMatrix(aspect);
    glm::mat4 view = g_cam.viewMatrix(aspect);
    glm::mat4 invVP = glm::inverse(proj * view);

    float ndcX =  (float)(2.0 * px / g_width  - 1.0);
    float ndcY = -(float)(2.0 * py / g_height - 1.0);

    glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 farPt  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);
    nearPt /= nearPt.w;
    farPt  /= farPt.w;

    rayOrigin = glm::vec3(nearPt);
    rayDir    = glm::normalize(glm::vec3(farPt - nearPt));
}

// Pick the closest block under the cursor; returns index or -1
static int pickBlock(double px, double py) {
    glm::vec3 ro, rd;
    screenRay(px, py, ro, rd);

    float bestT  = 1e30f;
    int   bestIdx = -1;
    for (int i = 0; i < (int)g_world.blocks.size(); ++i) {
        const Block& b = g_world.blocks[i];
        glm::vec3 bmin = b.position - b.halfExtents;
        glm::vec3 bmax = b.position + b.halfExtents;
        float t = rayAABB(ro, rd, bmin, bmax);
        if (t > 0 && t < bestT) { bestT = t; bestIdx = i; }
    }
    return bestIdx;
}

// Project mouse position onto horizontal plane at Y = planeY
static glm::vec3 rayPlaneHit(double px, double py, float planeY) {
    glm::vec3 ro, rd;
    screenRay(px, py, ro, rd);
    if (std::abs(rd.y) < 1e-6f) return ro;
    float t = (planeY - ro.y) / rd.y;
    return ro + rd * t;
}

// ─────────────────────────────────────────────────────────────────────────────
// Build a default scene: two hanging masses on springs
// ─────────────────────────────────────────────────────────────────────────────
static void buildDefaultScene() {
    g_world.blocks.clear();
    g_world.springs.clear();

    // Anchor block (fixed, acts as ceiling mount)
    {
        Block anchor;
        anchor.name        = "Ceiling Anchor";
        anchor.position    = {0, 6, 0};
        anchor.isAnchored  = true;
        anchor.halfExtents = {0.4f, 0.15f, 0.4f};
        anchor.albedo      = {0.3f, 0.3f, 0.3f};
        anchor.metallic    = 0.5f;
        anchor.roughness   = 0.6f;
        g_world.addBlock(anchor);
    }

    // First hanging mass
    {
        Block b;
        b.name        = "Mass A";
        b.position    = {0, 3, 0};
        b.mass        = 2.0f;
        b.halfExtents = {0.45f, 0.45f, 0.45f};
        b.albedo      = {0.12f, 0.45f, 0.80f};
        b.metallic    = 0.95f;
        b.roughness   = 0.15f;
        g_world.addBlock(b);
    }

    // Second hanging mass
    {
        Block b;
        b.name        = "Mass B";
        b.position    = {0, 0.5f, 0};
        b.mass        = 1.5f;
        b.halfExtents = {0.35f, 0.35f, 0.35f};
        b.albedo      = {0.85f, 0.25f, 0.15f};
        b.metallic    = 0.9f;
        b.roughness   = 0.2f;
        g_world.addBlock(b);
    }

    // Side block (resting on ground)
    {
        Block b;
        b.name        = "Floor Block";
        b.position    = {3.0f, 0.5f, 0};
        b.mass        = 3.0f;
        b.halfExtents = {0.5f, 0.5f, 0.5f};
        b.albedo      = {0.6f, 0.9f, 0.3f};
        b.metallic    = 0.8f;
        b.roughness   = 0.3f;
        g_world.addBlock(b);
    }

    // Spring 0: Anchor (block 0) -> Mass A (block 1)
    {
        Spring s;
        s.name       = "Spring (Anchor->A)";
        s.blockA     = 0;
        s.blockB     = 1;
        s.k          = 120.0f;
        s.damping    = 4.0f;
        s.restLength = 2.5f;
        g_world.addSpring(s);
    }

    // Spring 1: Mass A -> Mass B
    {
        Spring s;
        s.name       = "Spring (A->B)";
        s.blockA     = 1;
        s.blockB     = 2;
        s.k          = 80.0f;
        s.damping    = 3.0f;
        s.restLength = 2.0f;
        g_world.addSpring(s);
    }

    // Spring 2: World anchor above floor block -> floor block
    {
        Spring s;
        s.name       = "Spring (World->Floor)";
        s.blockA     = -1;          // fixed anchor
        s.anchorA    = {3.0f, 5.0f, 0};
        s.blockB     = 3;
        s.k          = 60.0f;
        s.damping    = 2.5f;
        s.restLength = 3.5f;
        g_world.addSpring(s);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Process UIActions after ImGui draw
// ─────────────────────────────────────────────────────────────────────────────
static void processUIActions(const UIActions& act) {
    if (act.addBlock) {
        Block b;
        static int cnt = 1;
        b.name = "Block " + std::to_string(cnt++);
        b.position    = {0, 4, 0};
        b.mass        = 1.0f;
        b.albedo      = {0.6f, 0.6f, 0.8f};
        g_selBlock    = g_world.addBlock(b);
        g_selSpring   = -1;
    }
    if (act.addSpring) {
        Spring s;
        static int cnt = 1;
        s.name    = "Spring " + std::to_string(cnt++);
        s.blockA  = (g_selBlock >= 0) ? g_selBlock : -1;
        s.anchorA = {0, 5, 0};
        s.blockB  = -1;
        s.anchorB = {0, 0, 0};
        g_selSpring = g_world.addSpring(s);
        g_selBlock  = -1;
    }
    if (act.deleteBlock && g_selBlock >= 0) {
        g_world.removeBlock(g_selBlock);
        g_selBlock = -1;
    }
    if (act.deleteSpring && g_selSpring >= 0) {
        g_world.removeSpring(g_selSpring);
        g_selSpring = -1;
    }
    if (act.resetSim) {
        buildDefaultScene();
        g_selBlock = g_selSpring = -1;
    }
    if (act.stepOnce) {
        bool wasRunning = g_world.running;
        g_world.running = true;
        g_world.step(1.0f / 60.0f);
        g_world.running = wasRunning;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    vkdemo::VulkanBase vkBase;

    vkdemo::AppConfig config;
    config.Title           = "Spring & Block Simulator (Vulkan)";
    config.Width           = g_width;
    config.Height          = g_height;
    config.EnableValidation = true;
    config.EnableImGui     = true;

    try {
        vkBase.Init(config);
    } catch (const std::exception& e) {
        std::cerr << "VulkanBase init failed: " << e.what() << "\n";
        return 1;
    }

    g_width  = vkBase.GetWidth();
    g_height = vkBase.GetHeight();

    // ImGui style customization
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding  = 6.0f;
    style.FrameRounding   = 4.0f;
    style.ItemSpacing     = {8, 6};
    style.Colors[ImGuiCol_WindowBg]       = {0.10f, 0.11f, 0.14f, 0.96f};
    style.Colors[ImGuiCol_Header]         = {0.20f, 0.40f, 0.60f, 1.00f};
    style.Colors[ImGuiCol_HeaderHovered]  = {0.25f, 0.50f, 0.75f, 1.00f};
    style.Colors[ImGuiCol_Button]         = {0.20f, 0.40f, 0.60f, 1.00f};
    style.Colors[ImGuiCol_ButtonHovered]  = {0.30f, 0.55f, 0.80f, 1.00f};
    style.Colors[ImGuiCol_FrameBg]        = {0.16f, 0.17f, 0.22f, 1.00f};

    // Callbacks (VulkanBase dispatches these via std::function)
    vkBase.OnScroll = [](GLFWwindow*, double /*dx*/, double dy) {
        if (ImGui::GetIO().WantCaptureMouse) return;
        g_cam.zoom((float)dy);
    };

    vkBase.OnMouseButton = [](GLFWwindow* win, int button, int action, int /*mods*/) {
        if (ImGui::GetIO().WantCaptureMouse) return;

        double cx, cy;
        glfwGetCursorPos(win, &cx, &cy);
        g_prevX = cx; g_prevY = cy;

        if (button == GLFW_MOUSE_BUTTON_RIGHT)
            g_rightDown = (action == GLFW_PRESS);
        if (button == GLFW_MOUSE_BUTTON_MIDDLE)
            g_midDown = (action == GLFW_PRESS);

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            g_leftDown = (action == GLFW_PRESS);
            if (action == GLFW_PRESS) {
                int picked = pickBlock(cx, cy);
                if (picked >= 0) {
                    g_selBlock  = picked;
                    g_selSpring = -1;
                    g_draggingBlock = true;
                    g_dragPlaneY = g_world.blocks[picked].position.y;
                } else {
                    g_selBlock  = -1;
                    g_selSpring = -1;
                    g_draggingBlock = false;
                }
            } else {
                g_draggingBlock = false;
            }
        }
    };

    vkBase.OnMouseMove = [](GLFWwindow*, double cx, double cy) {
        if (ImGui::GetIO().WantCaptureMouse) {
            g_prevX = cx; g_prevY = cy;
            return;
        }
        double dx = cx - g_prevX;
        double dy = cy - g_prevY;
        g_prevX = cx; g_prevY = cy;

        if (g_rightDown) g_cam.orbit((float)dx, (float)dy);
        if (g_midDown)   g_cam.pan  ((float)dx, (float)dy);

        if (g_draggingBlock && g_selBlock >= 0 && g_selBlock < (int)g_world.blocks.size()) {
            Block& b = g_world.blocks[g_selBlock];
            if (!b.isAnchored) {
                glm::vec3 hit = rayPlaneHit(cx, cy, g_dragPlaneY);
                b.position.x  = hit.x;
                b.position.z  = hit.z;
                b.velocity    = {0, 0, 0};
            }
        }
    };

    vkBase.OnResize = [](int w, int h) {
        g_width  = w;
        g_height = h;
        g_renderer.resize(w, h);
    };

    // Renderer + scene
    g_renderer.init(&vkBase);
    buildDefaultScene();

    // Camera start position
    g_cam.yaw      = 35.0f;
    g_cam.pitch    = 20.0f;
    g_cam.distance = 14.0f;
    g_cam.target   = {0.5f, 2.0f, 0.0f};

    double prevTime = glfwGetTime();

    // Main loop
    while (true) {
        if (!vkBase.BeginFrame())
            break;

        double now = glfwGetTime();
        float  dt  = std::min((float)(now - prevTime), 0.05f);
        prevTime   = now;

        // Physics
        g_world.step(dt);

        // ImGui frame
        vkBase.ImGuiNewFrame();

        // Clamp selection indices
        if (g_selBlock  >= (int)g_world.blocks.size())  g_selBlock  = -1;
        if (g_selSpring >= (int)g_world.springs.size()) g_selSpring = -1;

        g_ui.draw(g_world, g_renderer.cfg, g_selBlock, g_selSpring);
        processUIActions(g_ui.actions);

        // Record draw commands into the current command buffer
        VkCommandBuffer cmd = vkBase.GetCurrentCommandBuffer();

        // Update width/height in case of resize
        g_width  = vkBase.GetWidth();
        g_height = vkBase.GetHeight();

        g_renderer.render(cmd, g_world, g_cam, g_selBlock, g_selSpring);

        // ImGui render (on top, into the same render pass)
        vkBase.ImGuiRender(cmd);

        vkBase.EndFrame();
    }

    // Cleanup
    g_renderer.shutdown();
    vkBase.Shutdown();
    return 0;
}
