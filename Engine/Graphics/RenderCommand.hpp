#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace GameEngine {

    /**
     * @brief Low-level rendering commands
     * 
     * Abstraction over OpenGL for basic rendering operations
     * All functions are static for easy access
     */
    class RenderCommand {
    public:
        /**
         * @brief Initialize renderer
         */
        static void Init();
        
        /**
         * @brief Set viewport dimensions
         */
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        
        /**
         * @brief Set clear color
         */
        static void SetClearColor(const glm::vec4& color);
        
        /**
         * @brief Clear screen
         */
        static void Clear();
        
        /**
         * @brief Enable/disable depth testing
         */
        static void SetDepthTest(bool enabled);
        
        /**
         * @brief Enable/disable blending
         */
        static void SetBlending(bool enabled);
        
        /**
         * @brief Enable/disable face culling
         */
        static void SetCulling(bool enabled);
        
        /**
         * @brief Set polygon mode (fill, line, point)
         */
        enum class PolygonMode {
            Fill,
            Line,
            Point
        };
        static void SetPolygonMode(PolygonMode mode);
        
        /**
         * @brief Draw indexed geometry
         */
        static void DrawIndexed(uint32_t count, bool wireframe = false);
        
        /**
         * @brief Draw non-indexed geometry
         */
        static void DrawArrays(uint32_t count);
    };
}