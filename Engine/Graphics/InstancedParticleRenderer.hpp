#pragma once

#include "../Core/Base.hpp"
#include "Shader.hpp"
#include "Texture2D.hpp"
#include "Camera3D.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Instance data for a single particle
     */
    struct ParticleInstance {
        glm::vec3 Position;
        float Size;
        glm::vec4 Color;
        float Rotation;      // Z-axis rotation for billboards
        float LifeRemaining; // 0-1 for shader effects
    };

    /**
     * @brief GPU-instanced particle renderer
     * 
     * Efficiently renders thousands of particles using hardware instancing.
     * Particles are always billboarded toward the camera.
     * 
     * Features:
     * - Hardware instancing for maximum performance
     * - Automatic billboarding
     * - Texture atlas support
     * - Soft particles (depth fade)
     * - Color and size variation
     */
    class InstancedParticleRenderer {
    public:
        /**
         * @brief Initialize renderer resources
         */
        static void Init();
        
        /**
         * @brief Shutdown and clean up
         */
        static void Shutdown();
        
        /**
         * @brief Begin particle batch
         */
        static void Begin(const Camera3D& camera);
        
        /**
         * @brief End batch and submit for rendering
         */
        static void End();
        
        /**
         * @brief Submit a batch of particles for rendering
         * @param particles Vector of particle instances
         * @param texture Optional particle texture (nullptr = white quad)
         */
        static void DrawParticles(
            const std::vector<ParticleInstance>& particles,
            const Ref<Texture2D>& texture = nullptr
        );
        
        /**
         * @brief Set blend mode
         */
        enum class BlendMode {
            Additive,       // Fire, glow effects
            Alpha,          // Standard transparency
            Multiply        // Shadows, smoke
        };
        static void SetBlendMode(BlendMode mode);
        
        /**
         * @brief Enable/disable soft particles (depth fade)
         */
        static void SetSoftParticles(bool enabled, float fadeDistance = 0.5f);
        
        /**
         * @brief Set texture atlas grid size
         */
        static void SetAtlasSize(int columns, int rows);
        
        /**
         * @brief Get statistics
         */
        struct Statistics {
            uint32_t DrawCalls = 0;
            uint32_t ParticlesRendered = 0;
            uint32_t BatchCount = 0;
        };
        static const Statistics& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats = Statistics(); }
        
    private:
        static void CreateResources();
        static void UpdateInstanceBuffer(const std::vector<ParticleInstance>& particles);
        
        static Ref<Shader> s_ParticleShader;
        static uint32_t s_QuadVAO;
        static uint32_t s_QuadVBO;
        static uint32_t s_InstanceVBO;
        static uint32_t s_MaxParticles;
        static Ref<Texture2D> s_DefaultTexture;
        
        static glm::mat4 s_ViewProjection;
        static glm::vec3 s_CameraRight;
        static glm::vec3 s_CameraUp;
        
        static BlendMode s_BlendMode;
        static bool s_SoftParticles;
        static float s_SoftParticleFadeDistance;
        static int s_AtlasColumns;
        static int s_AtlasRows;
        
        static Statistics s_Stats;
    };

}
