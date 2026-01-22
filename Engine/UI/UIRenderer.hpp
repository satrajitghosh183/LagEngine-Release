#pragma once

#include "../Core/Base.hpp"

// Forward declare GLFWwindow
struct GLFWwindow;

namespace GameEngine {

    /**
     * @brief Simple UI Renderer using ImGui
     */
    class UIRenderer {
    public:
        /**
         * @brief Initialize UI rendering
         */
        static void Init();

        /**
         * @brief Initialize with specific GLFW window
         */
        static void Init(GLFWwindow* window);

        /**
         * @brief Shutdown UI rendering
         */
        static void Shutdown();

        /**
         * @brief Begin a new UI frame
         */
        static void BeginFrame();

        /**
         * @brief End the UI frame and render
         */
        static void EndFrame();

        /**
         * @brief Check if UI is initialized
         */
        static bool IsInitialized() { return s_Initialized; }

    private:
        static bool s_Initialized;
    };

}
