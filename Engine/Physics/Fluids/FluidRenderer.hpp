#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace GameEngine::Physics {

    /**
     * @brief Billboard sphere-impostor renderer for SPH fluid particles.
     *
     * Ported directly from old_code/src/sph_water.cpp.
     *
     * Each particle is drawn as a GL_POINT whose fragment shader reconstructs
     * a sphere normal from the point-coord, then applies Fresnel + specular
     * to give a convincing water appearance.
     *
     * Usage:
     *   static FluidRenderer s_FR;
     *   s_FR.Init();       // call once after OpenGL context is ready
     *   // per frame:
     *   s_FR.Render(positions, viewMat, projMat, radius);
     */
    class FluidRenderer {
    public:
        FluidRenderer()  = default;
        ~FluidRenderer() { Shutdown(); }

        /** Compile shaders and allocate GPU buffers.  Call after GL context is ready. */
        void Init();

        /**
         * @brief Draw particles as sphere impostors.
         * @param positions  World-space particle positions
         * @param view       Camera view matrix
         * @param proj       Camera projection matrix
         * @param radius     World-space sphere radius (controls point size)
         */
        void Render(const std::vector<glm::vec3>& positions,
                    const glm::mat4& view,
                    const glm::mat4& proj,
                    float radius = 0.014f);

        void Shutdown();

        bool IsReady() const { return m_Ready; }

    private:
        void CompileShaders();

        uint32_t m_Program  = 0;
        uint32_t m_VAO      = 0;
        uint32_t m_VBO      = 0;
        bool     m_Ready    = false;

        // Uniform locations
        int m_LocView   = -1;
        int m_LocProj   = -1;
        int m_LocPoint  = -1;
        int m_LocLight  = -1;
        int m_LocEnvTop = -1;
        int m_LocEnvBot = -1;
        int m_LocRad    = -1;
    };

} // namespace GameEngine::Physics
