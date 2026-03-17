#include "UIRenderer.hpp"
#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/Logger.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

namespace GameEngine {

    void UIRenderer::Init() {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();

        // Use a dedicated layout file so the old imgui.ini (with all windows
        // saved as floating) does not override the dock layout on startup.
        io.IniFilename = "editor_layout.ini";

        // Enable docking and viewports (if available)
#ifdef IMGUI_HAS_DOCK
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef IMGUI_HAS_VIEWPORT
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

        // Setup style
        SetDarkTheme();

        // When viewports are enabled, tweak WindowRounding for OS windows
        ImGuiStyle& style = ImGui::GetStyle();
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            // Keep translucent alpha for docked panels; only force opaque for OS windows
            style.Colors[ImGuiCol_WindowBg].w = 0.92f;
        }
#endif

        // Setup Platform/Renderer backends
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 420");

        GE_CORE_INFO("UIRenderer initialized");
    }

    void UIRenderer::Shutdown() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        GE_CORE_INFO("UIRenderer shutdown");
    }

    void UIRenderer::BeginFrame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void UIRenderer::EndFrame() {
        ImGuiIO& io = ImGui::GetIO();
        auto& window = Application::Get().GetWindow();
        io.DisplaySize = ImVec2(static_cast<float>(window.GetWidth()),
                               static_cast<float>(window.GetHeight()));

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and render additional platform windows
#ifdef IMGUI_HAS_VIEWPORT
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif
    }

    void UIRenderer::SetDarkTheme() {
        // Dark glass theme: #1a1a2e base, #FF6B35 orange accent, translucent panels
        ImVec4* colors = ImGui::GetStyle().Colors;

        // Base palette
        const ImVec4 bg          = ImVec4(0.102f, 0.102f, 0.180f, 0.85f);  // #1a1a2e
        const ImVec4 bgSolid     = ImVec4(0.102f, 0.102f, 0.180f, 1.00f);
        const ImVec4 bgDark      = ImVec4(0.067f, 0.067f, 0.130f, 0.90f);  // darker variant
        const ImVec4 surface     = ImVec4(0.140f, 0.140f, 0.230f, 0.85f);  // panels/cards
        const ImVec4 surfaceHi   = ImVec4(0.180f, 0.180f, 0.280f, 0.90f);
        const ImVec4 accent      = ImVec4(1.000f, 0.420f, 0.210f, 1.00f);  // #FF6B35
        const ImVec4 accentDim   = ImVec4(0.800f, 0.336f, 0.168f, 1.00f);
        const ImVec4 accentHover = ImVec4(1.000f, 0.500f, 0.300f, 1.00f);
        const ImVec4 textPrimary = ImVec4(0.920f, 0.920f, 0.950f, 1.00f);
        const ImVec4 textDim     = ImVec4(0.500f, 0.500f, 0.580f, 1.00f);
        const ImVec4 borderCol   = ImVec4(0.220f, 0.220f, 0.320f, 0.40f);

        colors[ImGuiCol_Text]                   = textPrimary;
        colors[ImGuiCol_TextDisabled]           = textDim;
        colors[ImGuiCol_WindowBg]               = bg;
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.090f, 0.090f, 0.160f, 0.95f);
        colors[ImGuiCol_Border]                 = borderCol;
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
        colors[ImGuiCol_FrameBg]                = bgDark;
        colors[ImGuiCol_FrameBgHovered]         = surface;
        colors[ImGuiCol_FrameBgActive]          = surfaceHi;
        colors[ImGuiCol_TitleBg]                = bgSolid;
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.120f, 0.120f, 0.200f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.067f, 0.067f, 0.130f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.120f, 0.120f, 0.200f, 0.90f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.05f, 0.05f, 0.10f, 0.50f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.30f, 0.30f, 0.40f, 0.60f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.50f, 0.70f);
        colors[ImGuiCol_ScrollbarGrabActive]    = accent;
        colors[ImGuiCol_CheckMark]              = accent;
        colors[ImGuiCol_SliderGrab]             = accentDim;
        colors[ImGuiCol_SliderGrabActive]       = accent;
        colors[ImGuiCol_Button]                 = surface;
        colors[ImGuiCol_ButtonHovered]          = surfaceHi;
        colors[ImGuiCol_ButtonActive]           = accent;
        colors[ImGuiCol_Header]                 = surface;
        colors[ImGuiCol_HeaderHovered]          = surfaceHi;
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.200f, 0.200f, 0.300f, 0.80f);
        colors[ImGuiCol_Separator]              = borderCol;
        colors[ImGuiCol_SeparatorHovered]       = accentDim;
        colors[ImGuiCol_SeparatorActive]        = accent;
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.30f, 0.30f, 0.40f, 0.30f);
        colors[ImGuiCol_ResizeGripHovered]      = accentDim;
        colors[ImGuiCol_ResizeGripActive]       = accent;
        colors[ImGuiCol_Tab]                    = bgDark;
        colors[ImGuiCol_TabHovered]             = surfaceHi;
        colors[ImGuiCol_TabActive]              = surface;
        colors[ImGuiCol_TabUnfocused]           = bgDark;
        colors[ImGuiCol_TabUnfocusedActive]     = surface;
#ifdef IMGUI_HAS_DOCK
        colors[ImGuiCol_DockingPreview]         = accent;
        colors[ImGuiCol_DockingEmptyBg]         = bgSolid;
#endif
        colors[ImGuiCol_PlotLines]              = accent;
        colors[ImGuiCol_PlotLinesHovered]       = accentHover;
        colors[ImGuiCol_PlotHistogram]          = accent;
        colors[ImGuiCol_PlotHistogramHovered]   = accentHover;
        colors[ImGuiCol_TableHeaderBg]          = bgDark;
        colors[ImGuiCol_TableBorderStrong]      = borderCol;
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.20f, 0.20f, 0.28f, 0.25f);
        colors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.00f, 0.42f, 0.21f, 0.30f);
        colors[ImGuiCol_DragDropTarget]         = accent;
        colors[ImGuiCol_NavHighlight]           = accent;
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 0.42f, 0.21f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.10f, 0.10f, 0.18f, 0.60f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.05f, 0.05f, 0.10f, 0.50f);

        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowPadding                     = ImVec2(10.00f, 10.00f);
        style.FramePadding                      = ImVec2(6.00f, 4.00f);
        style.CellPadding                       = ImVec2(6.00f, 4.00f);
        style.ItemSpacing                       = ImVec2(8.00f, 6.00f);
        style.ItemInnerSpacing                  = ImVec2(6.00f, 6.00f);
        style.TouchExtraPadding                 = ImVec2(0.00f, 0.00f);
        style.IndentSpacing                     = 22;
        style.ScrollbarSize                     = 13;
        style.GrabMinSize                       = 10;
        style.WindowBorderSize                  = 1;
        style.ChildBorderSize                   = 1;
        style.PopupBorderSize                   = 1;
        style.FrameBorderSize                   = 0;
        style.TabBorderSize                     = 0;
        style.WindowRounding                    = 8;
        style.ChildRounding                     = 6;
        style.FrameRounding                     = 6;
        style.PopupRounding                     = 6;
        style.ScrollbarRounding                 = 9;
        style.GrabRounding                      = 4;
        style.LogSliderDeadzone                 = 4;
        style.TabRounding                       = 6;
    }

    void UIRenderer::SetLightTheme() {
        ImGui::StyleColorsLight();
    }

    ImGuiContext* UIRenderer::GetContext() {
        return ImGui::GetCurrentContext();
    }

    bool UIRenderer::Button(const char* label, const ImVec2& size) {
        return ImGui::Button(label, size);
    }

    bool UIRenderer::Checkbox(const char* label, bool* value) {
        return ImGui::Checkbox(label, value);
    }

    bool UIRenderer::SliderFloat(const char* label, float* value, float min, float max) {
        return ImGui::SliderFloat(label, value, min, max);
    }

    bool UIRenderer::SliderInt(const char* label, int* value, int min, int max) {
        return ImGui::SliderInt(label, value, min, max);
    }

    bool UIRenderer::InputText(const char* label, char* buffer, size_t bufferSize) {
        return ImGui::InputText(label, buffer, bufferSize);
    }

    bool UIRenderer::ColorEdit3(const char* label, float* color) {
        return ImGui::ColorEdit3(label, color);
    }

    bool UIRenderer::ColorEdit4(const char* label, float* color) {
        return ImGui::ColorEdit4(label, color);
    }

    bool UIRenderer::Vec3Control(const char* label, glm::vec3& value, float resetValue, float columnWidth) {
        ImGuiIO& io = ImGui::GetIO();
        auto boldFont = io.Fonts->Fonts[0];

        bool modified = false;

        ImGui::PushID(label);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, columnWidth);
        ImGui::Text("%s", label);
        ImGui::NextColumn();

        ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        float lineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        // X
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize)) {
            value.x = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##X", &value.x, 0.1f);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Y
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize)) {
            value.y = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##Y", &value.y, 0.1f);
        ImGui::PopItemWidth();
        ImGui::SameLine();

        // Z
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize)) {
            value.z = resetValue;
            modified = true;
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        modified |= ImGui::DragFloat("##Z", &value.z, 0.1f);
        ImGui::PopItemWidth();

        ImGui::PopStyleVar();

        ImGui::Columns(1);

        ImGui::PopID();

        return modified;
    }

    void UIRenderer::Image(uint32_t textureID, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1) {
        ImGui::Image((ImTextureID)textureID, size, uv0, uv1);
    }

    void UIRenderer::BeginDockspace() {
        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
#ifdef IMGUI_HAS_DOCK
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoUndocking;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
#else
        // Docking not available - use regular window
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar;

        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);
#endif
    }

    void UIRenderer::EndDockspace() {
        ImGui::End();
    }

}

