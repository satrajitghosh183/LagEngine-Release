#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Graphics/Framebuffer.hpp"
#include <imgui.h>
#include <functional>

namespace GameEngine {

    /**
     * @brief ImGui-based UI renderer
     * 
     * Immediate mode GUI system for editor
     * Moved from Engine/UI to Editor/UI to keep runtime free of ImGui dependencies
     */
    class UIRenderer {
    public:
        /**
         * @brief Initialize UI system
         */
        static void Init();

        /**
         * @brief Shutdown UI system
         */
        static void Shutdown();

        /**
         * @brief Begin new frame
         */
        static void BeginFrame();

        /**
         * @brief End frame and render
         */
        static void EndFrame();

        /**
         * @brief Set dark theme
         */
        static void SetDarkTheme();

        /**
         * @brief Set light theme
         */
        static void SetLightTheme();

        /**
         * @brief Get ImGui context
         */
        static ImGuiContext* GetContext();

        /**
         * @brief Common UI elements
         */
        static bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
        static bool Checkbox(const char* label, bool* value);
        static bool SliderFloat(const char* label, float* value, float min, float max);
        static bool SliderInt(const char* label, int* value, int min, int max);
        static bool InputText(const char* label, char* buffer, size_t bufferSize);
        static bool ColorEdit3(const char* label, float* color);
        static bool ColorEdit4(const char* label, float* color);

        /**
         * @brief Vector controls
         */
        static bool Vec3Control(const char* label, glm::vec3& value, float resetValue = 0.0f, float columnWidth = 100.0f);

        /**
         * @brief Draw texture
         */
        static void Image(uint32_t textureID, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1));

        /**
         * @brief Docking
         */
        static void BeginDockspace();
        static void EndDockspace();
    };

}

