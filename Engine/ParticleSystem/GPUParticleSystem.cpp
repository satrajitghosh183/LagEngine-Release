#include "GPUParticleSystem.hpp"
#include "../Core/Logger.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <cstdlib>

namespace GameEngine {

    // -----------------------------------------------------------------------
    // Embedded shader sources
    // -----------------------------------------------------------------------

    static const char* s_ComputeShaderSource = R"(
#version 430 core

layout(local_size_x = 256) in;

struct Particle {
    vec4  position;
    vec4  velocity;
    vec4  color;
    float life;
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform float u_DeltaTime;
uniform vec3  u_Gravity;
uniform float u_Restitution;
uniform float u_Damping;
uniform float u_FloorY;
uniform float u_EmitHeight;
uniform float u_MaxLife;
uniform int   u_EmitCount;     // how many particles to (re-)emit this frame
uniform uint  u_MaxParticles;
uniform uint  u_FrameSeed;     // simple seed for pseudo-random emission

// Simple hash for pseudo-random numbers
uint hash(uint x) {
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    x *= 0x45d9f3bu;
    x ^= x >> 16u;
    return x;
}

float randFloat(uint seed) {
    return float(hash(seed)) / float(0xFFFFFFFFu);
}

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= u_MaxParticles) return;

    Particle p = particles[idx];

    if (p.life > 0.0) {
        // --- Integrate ---
        p.velocity.xyz += u_Gravity * u_DeltaTime;
        p.position.xyz += p.velocity.xyz * u_DeltaTime;

        // --- Floor collision ---
        if (p.position.y < u_FloorY) {
            p.position.y = u_FloorY;
            p.velocity.y = -p.velocity.y * u_Restitution;
            p.velocity.xyz *= u_Damping;
        }

        // --- Age ---
        p.life -= u_DeltaTime;

        // Fade alpha as life decreases
        float t = clamp(p.life / u_MaxLife, 0.0, 1.0);
        p.color.a = t;
    } else {
        // --- Re-emit dead particles ---
        // We use a simple counter approach: the first u_EmitCount dead particles
        // encountered (globally) get re-emitted.  Because of the parallel nature
        // this is approximate, which is fine for visual effects.
        if (u_EmitCount > 0) {
            // Deterministic-ish per-frame seed
            uint seed = u_FrameSeed * 1973u + idx * 9277u + 6271u;
            float r1 = randFloat(seed);
            float r2 = randFloat(seed + 1u);
            float r3 = randFloat(seed + 2u);
            float r4 = randFloat(seed + 3u);

            // Only revive if this index falls within the emit window
            // Spread emission across the buffer to avoid clumping
            uint emitSlot = idx % uint(u_EmitCount + 1);
            if (emitSlot < uint(u_EmitCount)) {
                p.position = vec4((r1 - 0.5) * 2.0, u_EmitHeight, (r2 - 0.5) * 2.0, 1.0);
                p.velocity = vec4((r3 - 0.5) * 2.0, -r4 * 2.0, (randFloat(seed + 4u) - 0.5) * 2.0, 0.0);
                p.color    = vec4(r1, r3, r4, 1.0);
                p.life     = u_MaxLife * (0.5 + 0.5 * randFloat(seed + 5u));
            }
        }
    }

    particles[idx] = p;
}
)";

    static const char* s_VertexShaderSource = R"(
#version 430 core

struct Particle {
    vec4  position;
    vec4  velocity;
    vec4  color;
    float life;
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform mat4 u_ViewProjection;
uniform float u_PointSize;

out vec4 v_Color;

void main() {
    Particle p = particles[gl_InstanceID];
    v_Color = p.color;

    // Cull dead particles by placing them off-screen
    if (p.life <= 0.0) {
        gl_Position = vec4(-9999.0, -9999.0, -9999.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    gl_Position  = u_ViewProjection * vec4(p.position.xyz, 1.0);
    gl_PointSize = u_PointSize * 1000.0 / max(gl_Position.w, 0.001);
}
)";

    static const char* s_FragmentShaderSource = R"(
#version 430 core

in vec4 v_Color;
out vec4 FragColor;

void main() {
    // Circular point sprite
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float dist = dot(coord, coord);
    if (dist > 1.0)
        discard;

    float alpha = v_Color.a * (1.0 - dist);
    FragColor = vec4(v_Color.rgb, alpha);
}
)";

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    static uint32_t CompileShader(GLenum type, const char* source) {
        uint32_t shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        int success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
            GE_CORE_ERROR("GPUParticleSystem shader compile error: {}", infoLog);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    static uint32_t LinkProgram(uint32_t* shaders, int count) {
        uint32_t program = glCreateProgram();
        for (int i = 0; i < count; ++i)
            glAttachShader(program, shaders[i]);
        glLinkProgram(program);

        int success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[1024];
            glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
            GE_CORE_ERROR("GPUParticleSystem program link error: {}", infoLog);
            glDeleteProgram(program);
            return 0;
        }

        for (int i = 0; i < count; ++i)
            glDeleteShader(shaders[i]);

        return program;
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    GPUParticleSystem::~GPUParticleSystem() {
        if (m_Initialized)
            Shutdown();
    }

    void GPUParticleSystem::Init(uint32_t maxParticles) {
        if (m_Initialized) {
            GE_CORE_WARN("GPUParticleSystem::Init called on already-initialised system");
            return;
        }

        m_MaxParticles = maxParticles;

        // Create SSBO with zeroed-out particles (life <= 0 means dead)
        std::vector<Particle> initial(m_MaxParticles);
        for (auto& p : initial) {
            p.Position = glm::vec4(0.0f);
            p.Velocity = glm::vec4(0.0f);
            p.Color    = glm::vec4(1.0f);
            p.Life     = 0.0f;
        }

        glGenBuffers(1, &m_ParticleSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ParticleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     static_cast<GLsizeiptr>(m_MaxParticles * sizeof(Particle)),
                     initial.data(),
                     GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // Compile shaders
        CompileComputeShader();
        CompileRenderShaders();

        // Create a dummy VAO for the instanced draw (no vertex attributes needed
        // since all data is read from the SSBO via gl_InstanceID).
        glGenVertexArrays(1, &m_VAO);

        m_Initialized = true;
        GE_CORE_INFO("GPUParticleSystem initialised with {} particles", m_MaxParticles);
    }

    void GPUParticleSystem::Update(float dt) {
        if (!m_Initialized || m_ComputeProgram == 0) return;

        // Calculate how many particles to emit this step
        m_EmitAccumulator += static_cast<float>(EmitRate) * dt;
        int emitCount = static_cast<int>(m_EmitAccumulator);
        m_EmitAccumulator -= static_cast<float>(emitCount);

        // Bind SSBO
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO);

        glUseProgram(m_ComputeProgram);

        // Set uniforms
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_DeltaTime"),    dt);
        glUniform3fv(glGetUniformLocation(m_ComputeProgram, "u_Gravity"),     1, glm::value_ptr(Gravity));
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_Restitution"),  Restitution);
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_Damping"),      Damping);
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_FloorY"),       FloorY);
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_EmitHeight"),   EmitHeight);
        glUniform1f(glGetUniformLocation(m_ComputeProgram, "u_MaxLife"),      MaxLife);
        glUniform1i(glGetUniformLocation(m_ComputeProgram, "u_EmitCount"),    emitCount);
        glUniform1ui(glGetUniformLocation(m_ComputeProgram, "u_MaxParticles"), m_MaxParticles);

        // Frame-varying seed for pseudo-random emission
        static uint32_t s_FrameCounter = 0;
        glUniform1ui(glGetUniformLocation(m_ComputeProgram, "u_FrameSeed"), s_FrameCounter++);

        // Dispatch compute
        uint32_t numGroups = (m_MaxParticles + 255) / 256;
        glDispatchCompute(numGroups, 1, 1);

        // Memory barrier so subsequent draw sees updated SSBO data
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        glUseProgram(0);
    }

    void GPUParticleSystem::Render(const glm::mat4& viewProj) {
        if (!m_Initialized || m_RenderProgram == 0) return;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO);

        glUseProgram(m_RenderProgram);
        glUniformMatrix4fv(
            glGetUniformLocation(m_RenderProgram, "u_ViewProjection"),
            1, GL_FALSE, glm::value_ptr(viewProj));
        glUniform1f(
            glGetUniformLocation(m_RenderProgram, "u_PointSize"), ParticleSize);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glBindVertexArray(m_VAO);
        glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(m_MaxParticles));
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_PROGRAM_POINT_SIZE);

        glUseProgram(0);
    }

    void GPUParticleSystem::Shutdown() {
        if (!m_Initialized) return;

        if (m_ParticleSSBO)   { glDeleteBuffers(1, &m_ParticleSSBO);   m_ParticleSSBO   = 0; }
        if (m_ComputeProgram) { glDeleteProgram(m_ComputeProgram);     m_ComputeProgram = 0; }
        if (m_RenderProgram)  { glDeleteProgram(m_RenderProgram);      m_RenderProgram  = 0; }
        if (m_VAO)            { glDeleteVertexArrays(1, &m_VAO);       m_VAO            = 0; }

        m_Initialized = false;
        GE_CORE_INFO("GPUParticleSystem shut down");
    }

    // -----------------------------------------------------------------------
    // Shader compilation helpers
    // -----------------------------------------------------------------------

    void GPUParticleSystem::CompileComputeShader() {
        uint32_t cs = CompileShader(GL_COMPUTE_SHADER, s_ComputeShaderSource);
        if (cs == 0) return;
        m_ComputeProgram = LinkProgram(&cs, 1);
    }

    void GPUParticleSystem::CompileRenderShaders() {
        uint32_t shaders[2];
        shaders[0] = CompileShader(GL_VERTEX_SHADER, s_VertexShaderSource);
        shaders[1] = CompileShader(GL_FRAGMENT_SHADER, s_FragmentShaderSource);
        if (shaders[0] == 0 || shaders[1] == 0) {
            if (shaders[0]) glDeleteShader(shaders[0]);
            if (shaders[1]) glDeleteShader(shaders[1]);
            return;
        }
        m_RenderProgram = LinkProgram(shaders, 2);
    }

} // namespace GameEngine
