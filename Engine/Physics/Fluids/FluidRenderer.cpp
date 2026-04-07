/**
 * FluidRenderer.cpp
 *
 * Billboard sphere-impostor renderer for SPH particles.
 * Ported from old_code/src/sph_water.cpp (the VS/FS shader pair).
 *
 * Visual features (unchanged from old code):
 *   - Per-particle point size computed from world-space radius / eye-distance
 *   - Sphere normal reconstructed in the fragment shader from gl_PointCoord
 *   - Fresnel blend: deep-blue core → sky environment at grazing angles
 *   - Blinn specular highlight
 *   - Alpha transparency for subsurface look
 */

#include "FluidRenderer.hpp"
#include "../../Core/Logger.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cstring>

namespace GameEngine::Physics {

    // =========================================================================
    // Shaders — ported verbatim from old_code/src/sph_water.cpp
    // =========================================================================

    static const char* s_VS = R"GLSL(
#version 130

uniform mat4 uView;
uniform mat4 uProj;
uniform float uPointScale;   // world_radius * scale_factor

in vec3 aPos;
out vec3 vEyePos;

void main() {
    vec4 eye    = uView * vec4(aPos, 1.0);
    vEyePos     = eye.xyz;
    gl_Position = uProj * eye;
    float dist  = -eye.z;
    gl_PointSize = max(1.0, uPointScale / max(0.001, dist));
}
)GLSL";

    static const char* s_FS = R"GLSL(
#version 130

in  vec3 vEyePos;
out vec4 FragColor;

uniform vec3  uLightDir;   // normalised, eye-space
uniform vec3  uEnvTop;     // sky colour
uniform vec3  uEnvBot;     // ground colour
uniform float uRadius;     // world radius (unused here but kept for parity)

void main() {
    // Map point-coord to [-1,1]^2
    vec2  uv = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(uv, uv);
    if (r2 > 1.0) discard;           // clip to circle

    // Reconstruct sphere normal in billboard space
    float z  = sqrt(max(0.0, 1.0 - r2));
    vec3  n  = normalize(vec3(uv.x, uv.y, z));

    vec3  V      = normalize(-vEyePos);
    float NdotL  = max(0.0, dot(n, uLightDir));
    float spec   = pow(max(0.0, dot(reflect(-uLightDir, n), V)), 64.0);

    // Fresnel + environment blend
    float fres   = pow(1.0 - max(0.0, dot(n, V)), 3.0);
    vec3  env    = mix(uEnvBot, uEnvTop, clamp(n.y * 0.5 + 0.5, 0.0, 1.0));
    vec3  base   = mix(vec3(0.02, 0.10, 0.9), env, fres);
    vec3  col    = base * (0.2 + 0.8 * NdotL) + spec * vec3(1.0);

    float alpha  = 0.35 + 0.25 * fres;
    FragColor    = vec4(col, alpha);
}
)GLSL";

    // =========================================================================
    // Helpers
    // =========================================================================

    static GLuint CompileShader(GLenum type, const char* src) {
        GLuint sh = glCreateShader(type);
        glShaderSource(sh, 1, &src, nullptr);
        glCompileShader(sh);
        GLint ok;
        glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetShaderInfoLog(sh, sizeof(log), nullptr, log);
            GE_CORE_ERROR("FluidRenderer shader compile error:\n{}", log);
            glDeleteShader(sh);
            return 0;
        }
        return sh;
    }

    // =========================================================================
    // Public API
    // =========================================================================

    void FluidRenderer::Init() {
        if (m_Ready) return;
        CompileShaders();

        glGenVertexArrays(1, &m_VAO);
        glBindVertexArray(m_VAO);

        glGenBuffers(1, &m_VBO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        // Allocate space for up to 100k particles (will grow via glBufferData if needed)
        glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);

        glBindVertexArray(0);
        m_Ready = (m_Program != 0);

        if (m_Ready)
            GE_CORE_INFO("FluidRenderer initialised (sphere-impostor, Fresnel water)");
    }

    void FluidRenderer::Render(const std::vector<glm::vec3>& positions,
                               const glm::mat4& view,
                               const glm::mat4& proj,
                               float radius) {
        if (!m_Ready || positions.empty()) return;

        // Upload positions
        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(positions.size() * sizeof(glm::vec3)),
                     positions.data(),
                     GL_DYNAMIC_DRAW);

        // Render state
        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // Write depth so spheres occlude each other, but don't disturb existing geometry
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        glUseProgram(m_Program);
        glUniformMatrix4fv(m_LocView,  1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(m_LocProj,  1, GL_FALSE, glm::value_ptr(proj));

        // point-scale: radius * constant so particle fills ~right amount of pixels
        glUniform1f(m_LocPoint,  radius * 1400.0f);
        glUniform1f(m_LocRad,    radius);

        // Fixed light (eye-space direction — slightly above and to the right)
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.6f, 0.8f, 0.3f));
        glUniform3f(m_LocLight,  lightDir.x, lightDir.y, lightDir.z);

        // Environment colours (sky/ground for Fresnel blend)
        glUniform3f(m_LocEnvTop, 0.70f, 0.85f, 1.00f);  // pale sky
        glUniform3f(m_LocEnvBot, 0.90f, 0.95f, 1.00f);  // pale ground

        glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(positions.size()));

        glBindVertexArray(0);
        glUseProgram(0);

        // Restore defaults
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);
    }

    void FluidRenderer::Shutdown() {
        if (m_VBO)     { glDeleteBuffers(1, &m_VBO);       m_VBO     = 0; }
        if (m_VAO)     { glDeleteVertexArrays(1, &m_VAO);  m_VAO     = 0; }
        if (m_Program) { glDeleteProgram(m_Program);        m_Program = 0; }
        m_Ready = false;
    }

    void FluidRenderer::CompileShaders() {
        GLuint vs = CompileShader(GL_VERTEX_SHADER,   s_VS);
        GLuint fs = CompileShader(GL_FRAGMENT_SHADER, s_FS);
        if (!vs || !fs) { glDeleteShader(vs); glDeleteShader(fs); return; }

        m_Program = glCreateProgram();
        glAttachShader(m_Program, vs);
        glAttachShader(m_Program, fs);
        glBindAttribLocation(m_Program, 0, "aPos");
        glLinkProgram(m_Program);

        GLint ok;
        glGetProgramiv(m_Program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[2048];
            glGetProgramInfoLog(m_Program, sizeof(log), nullptr, log);
            GE_CORE_ERROR("FluidRenderer program link error:\n{}", log);
            glDeleteProgram(m_Program);
            m_Program = 0;
        }
        glDeleteShader(vs);
        glDeleteShader(fs);

        if (m_Program) {
            m_LocView   = glGetUniformLocation(m_Program, "uView");
            m_LocProj   = glGetUniformLocation(m_Program, "uProj");
            m_LocPoint  = glGetUniformLocation(m_Program, "uPointScale");
            m_LocLight  = glGetUniformLocation(m_Program, "uLightDir");
            m_LocEnvTop = glGetUniformLocation(m_Program, "uEnvTop");
            m_LocEnvBot = glGetUniformLocation(m_Program, "uEnvBot");
            m_LocRad    = glGetUniformLocation(m_Program, "uRadius");
        }
    }

} // namespace GameEngine::Physics
