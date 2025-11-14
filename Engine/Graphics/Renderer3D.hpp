#pragma once

#include "../Core/Base.hpp"
#include "Mesh3D.hpp"
#include "Material.hpp"
#include "Camera3D.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief 3D Renderer
     * 
     * High-level rendering API for 3D graphics
     * 
     * Features:
     * - Submit meshes with materials
     * - Automatic batching (future)
     * - Lighting support
     * - Statistics tracking
     * 
     * Usage:
     *   Renderer3D::BeginScene(camera);
     *   Renderer3D::Submit(mesh, material, transform);
     *   Renderer3D::EndScene();
     */
    class Renderer3D {
    public:
        /**
         * @brief Initialize renderer
         */
        static void Init();
        
        /**
         * @brief Shutdown renderer
         */
        static void Shutdown();
        
        /**
         * @brief Begin scene rendering
         */
        static void BeginScene(const Camera3D& camera);
        
        /**
         * @brief End scene rendering
         */
        static void EndScene();
        
        /**
         * @brief Submit mesh for rendering
         */
        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Material>& material, const glm::mat4& transform = glm::mat4(1.0f));
        
        /**
         * @brief Submit mesh with shader (no material)
         */
        static void Submit(const Ref<Mesh3D>& mesh, const Ref<Shader>& shader, const glm::mat4& transform = glm::mat4(1.0f));
        
        /**
         * @brief Rendering statistics
         */
        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t VertexCount = 0;
            uint32_t IndexCount = 0;
            
            void Reset() {
                DrawCalls = 0;
                VertexCount = 0;
                IndexCount = 0;
            }
        };
        
        static const Statistics& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats.Reset(); }
        
    private:
        struct SceneData {
            glm::mat4 ViewProjectionMatrix;
            glm::vec3 CameraPosition;
        };
        
        static SceneData s_SceneData;
        static Statistics s_Stats;
    };
}