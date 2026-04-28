#include "UIRenderer.hpp"
#include "../Core/Logger.hpp"
#include "../Core/Application.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>

namespace GameEngine {

    bool UIRenderer::s_Initialized = false;

    void UIRenderer::Init() {
        if (s_Initialized) {
            GE_CORE_WARN("UIRenderer already initialized");
            return;
        }

        // Get window from application
        GLFWwindow* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        Init(window);
    }

    void UIRenderer::Init(GLFWwindow* window) {
        if (s_Initialized) {
            GE_CORE_WARN("UIRenderer already initialized");
            return;
        }

        if (!window) {
            GE_CORE_ERROR("UIRenderer::Init called with null window");
            return;
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        // Note: Docking requires imgui with docking branch
        // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer bindings. The Vulkan backend's full Init
        // (instance/device/queue/etc.) is handled by the engine's render
        // path; here we only set up the GLFW platform binding so ImGui
        // input + window events flow.
        ImGui_ImplGlfw_InitForVulkan(window, true);

        s_Initialized = true;
        GE_CORE_INFO("UIRenderer initialized (GLFW Vulkan platform binding)");
    }

    void UIRenderer::Shutdown() {
        if (!s_Initialized) {
            return;
        }

        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        s_Initialized = false;
        GE_CORE_INFO("UIRenderer shutdown");
    }

    void UIRenderer::BeginFrame() {
        if (!s_Initialized) {
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void UIRenderer::EndFrame() {
        if (!s_Initialized) {
            return;
        }

        // The Vulkan backend's RenderDrawData call needs an active command
        // buffer; the engine's render path issues that call.
        ImGui::Render();
    }

}
