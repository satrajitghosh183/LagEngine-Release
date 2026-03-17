#include "GPUParticleSystem.hpp"
#include "../Core/Logger.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <cstdlib>

namespace GameEngine {

    // -----------------------------------------------------------------------
    // Helper to check compute shader support
    // -----------------------------------------------------------------------
    static bool s_ComputeShadersSupported = false;
    static bool s_ComputeSupportChecked = false;
    
    static bool CheckComputeShaderSupport() {
        if (s_ComputeSupportChecked) return s_ComputeShadersSupported;
        s_ComputeSupportChecked = true;
        
        // Check OpenGL version (need 4.3+ for compute shaders)
        int major = 0, minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        
        // Compute shaders require OpenGL 4.3+
        s_ComputeShadersSupported = (major > 4) || (major == 4 && minor >= 3);
        
        if (!s_ComputeShadersSupported) {
            GE_CORE_WARN("GPUParticleSystem: Compute shaders not supported (OpenGL {}.{}, need 4.3+). Using CPU fallback.", major, minor);
        }
        
        return s_ComputeShadersSupported;
    }

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
    float colorState;  // 0=cyan(unbounced), 1=yellow(bounced), 2=magenta(dying)
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
uniform int   u_EmitCount;
uniform uint  u_MaxParticles;
uniform uint  u_FrameSeed;

// Colors from old_code/particle-sim: cyan, yellow, magenta
const vec3 COLOR_CYAN    = vec3(0.0, 0.635, 0.827);   // (0, 162, 211) unbounced
const vec3 COLOR_YELLOW  = vec3(0.98, 0.878, 0.078);  // (250, 224, 20) bounced
const vec3 COLOR_MAGENTA = vec3(0.878, 0.031, 0.521); // (224, 8, 133) dying

uint hash(uint x) {
    x ^= x >> 16u; x *= 0x45d9f3bu;
    x ^= x >> 16u; x *= 0x45d9f3bu;
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
            
            // Change color state on bounce (cyan -> yellow)
            if (p.colorState < 0.5) {
                p.colorState = 1.0;
            }
            
            // Check if particle is nearly stationary (dying)
            if (length(p.velocity.xyz) < 0.5) {
                p.colorState = 2.0;
            }
        }

        // --- Age ---
        p.life -= u_DeltaTime;
        float t = clamp(p.life / u_MaxLife, 0.0, 1.0);

        // Set color based on state
        vec3 baseColor;
        if (p.colorState < 0.5) {
            baseColor = COLOR_CYAN;
        } else if (p.colorState < 1.5) {
            baseColor = COLOR_YELLOW;
        } else {
            baseColor = COLOR_MAGENTA;
        }
        p.color = vec4(baseColor, t);
        
    } else {
        // --- Re-emit dead particles ---
        if (u_EmitCount > 0) {
            uint seed = u_FrameSeed * 1973u + idx * 9277u + 6271u;
            float r1 = randFloat(seed);
            float r2 = randFloat(seed + 1u);
            float r3 = randFloat(seed + 2u);
            float r4 = randFloat(seed + 3u);

            uint emitSlot = idx % uint(u_EmitCount + 1);
            if (emitSlot < uint(u_EmitCount)) {
                // Spread randomness like old_code (spreadRandomness = 0.2)
                float spread = 0.2;
                p.position = vec4((r1 - 0.5) * spread * 10.0, u_EmitHeight, (r2 - 0.5) * spread * 10.0, 1.0);
                // Initial velocity downward with slight randomness
                p.velocity = vec4((r3 - 0.5) * spread, -0.01 - r4 * 0.3, (randFloat(seed + 4u) - 0.5) * spread, 0.0);
                p.color    = vec4(COLOR_CYAN, 1.0);  // Start as cyan (unbounced)
                p.colorState = 0.0;  // Reset color state
                p.life     = u_MaxLife * (0.5 + 0.5 * randFloat(seed + 5u));
            }
        }
    }

    particles[idx] = p;
}
)";

    // GLSL 4.30 version for systems with compute shader support
    static const char* s_VertexShaderSource_430 = R"(
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

    // GLSL 3.30 compatible version for fallback (VBO-based)
    static const char* s_VertexShaderSource_330 = R"(
#version 330 core

layout(location = 0) in vec4 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in float a_Life;

uniform mat4 u_ViewProjection;
uniform float u_PointSize;

out vec4 v_Color;

void main() {
    v_Color = a_Color;

    // Cull dead particles by placing them off-screen
    if (a_Life <= 0.0) {
        gl_Position = vec4(-9999.0, -9999.0, -9999.0, 1.0);
        gl_PointSize = 0.0;
        return;
    }

    gl_Position  = u_ViewProjection * a_Position;
    gl_PointSize = u_PointSize * 1000.0 / max(gl_Position.w, 0.001);
}
)";

    // Fragment shader works with both versions
    static const char* s_FragmentShaderSource = R"(
#version 330 core

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
        m_UseComputeShaders = CheckComputeShaderSupport();

        // Create particle data
        std::vector<Particle> initial(m_MaxParticles);
        for (auto& p : initial) {
            p.Position = glm::vec4(0.0f);
            p.Velocity = glm::vec4(0.0f);
            p.Color    = glm::vec4(1.0f);
            p.Life     = 0.0f;
        }

        if (m_UseComputeShaders) {
            // Create SSBO for GPU compute path
            glGenBuffers(1, &m_ParticleSSBO);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ParticleSSBO);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         static_cast<GLsizeiptr>(m_MaxParticles * sizeof(Particle)),
                         initial.data(),
                         GL_DYNAMIC_COPY);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            // Compile compute shader
            CompileComputeShader();
        } else {
            // CPU fallback - store particles in CPU memory and use VBO for rendering
            m_ParticlesCPU = std::move(initial);
            
            glGenBuffers(1, &m_ParticleSSBO);  // Used as VBO in this mode
            glBindBuffer(GL_ARRAY_BUFFER, m_ParticleSSBO);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(m_MaxParticles * sizeof(Particle)),
                         m_ParticlesCPU.data(),
                         GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // Compile render shaders
        CompileRenderShaders();

        // Create VAO
        glGenVertexArrays(1, &m_VAO);
        
        if (!m_UseComputeShaders) {
            // Set up VAO for VBO-based rendering
            glBindVertexArray(m_VAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_ParticleSSBO);
            
            // Position (vec4)
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, Position));
            
            // Color (vec4)  
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, Color));
            
            // Life (float)
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (void*)offsetof(Particle, Life));
            
            glBindVertexArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        m_Initialized = true;
        GE_CORE_INFO("GPUParticleSystem initialised with {} particles ({})", 
                     m_MaxParticles, m_UseComputeShaders ? "GPU compute" : "CPU fallback");
    }

    void GPUParticleSystem::Update(float dt) {
        if (!m_Initialized) return;

        // Calculate how many particles to emit this step
        m_EmitAccumulator += static_cast<float>(EmitRate) * dt;
        int emitCount = static_cast<int>(m_EmitAccumulator);
        m_EmitAccumulator -= static_cast<float>(emitCount);

        if (m_UseComputeShaders && m_ComputeProgram != 0) {
            // GPU compute path
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
        } else {
            // CPU fallback path
            UpdateCPU(dt);
        }
    }
    
    // Colors from old_code/particle-sim: cyan, yellow, magenta
    static const glm::vec3 COLOR_CYAN    = glm::vec3(0.0f, 0.635f, 0.827f);   // (0, 162, 211) unbounced
    static const glm::vec3 COLOR_YELLOW  = glm::vec3(0.98f, 0.878f, 0.078f);  // (250, 224, 20) bounced
    static const glm::vec3 COLOR_MAGENTA = glm::vec3(0.878f, 0.031f, 0.521f); // (224, 8, 133) dying
    
    void GPUParticleSystem::UpdateCPU(float dt) {
        static uint32_t s_FrameCounter = 0;
        s_FrameCounter++;
        
        int emitCount = static_cast<int>(m_EmitAccumulator);
        int emitted = 0;
        
        auto randFloat = [](uint32_t& s) -> float {
            s ^= s >> 16u; s *= 0x45d9f3bu;
            s ^= s >> 16u; s *= 0x45d9f3bu;
            s ^= s >> 16u;
            return float(s) / float(0xFFFFFFFFu);
        };
        
        for (uint32_t i = 0; i < m_MaxParticles && emitted < emitCount; ++i) {
            Particle& p = m_ParticlesCPU[i];
            
            if (p.Life > 0.0f) {
                // Integrate
                p.Velocity += glm::vec4(Gravity * dt, 0.0f);
                p.Position += p.Velocity * dt;
                
                // Floor collision
                if (p.Position.y < FloorY) {
                    p.Position.y = FloorY;
                    p.Velocity.y = -p.Velocity.y * Restitution;
                    p.Velocity *= Damping;
                    
                    // Change color state on bounce (cyan -> yellow)
                    if (p._pad[0] < 0.5f) {
                        p._pad[0] = 1.0f;
                    }
                    
                    // Check if nearly stationary (dying -> magenta)
                    if (glm::length(glm::vec3(p.Velocity)) < 0.5f) {
                        p._pad[0] = 2.0f;
                    }
                }
                
                // Age
                p.Life -= dt;
                float alpha = glm::clamp(p.Life / MaxLife, 0.0f, 1.0f);
                
                // Set color based on state
                glm::vec3 baseColor;
                if (p._pad[0] < 0.5f) {
                    baseColor = COLOR_CYAN;
                } else if (p._pad[0] < 1.5f) {
                    baseColor = COLOR_YELLOW;
                } else {
                    baseColor = COLOR_MAGENTA;
                }
                p.Color = glm::vec4(baseColor, alpha);
            } else {
                // Re-emit dead particle
                uint32_t seed = s_FrameCounter * 1973u + i * 9277u + 6271u;
                
                float r1 = randFloat(seed);
                float r2 = randFloat(seed);
                float r3 = randFloat(seed);
                float r4 = randFloat(seed);
                
                // Spread randomness like old_code (spreadRandomness = 0.2)
                float spread = 0.2f;
                p.Position = glm::vec4((r1 - 0.5f) * spread * 10.0f, EmitHeight, (r2 - 0.5f) * spread * 10.0f, 1.0f);
                p.Velocity = glm::vec4((r3 - 0.5f) * spread, -0.01f - r4 * 0.3f, (randFloat(seed) - 0.5f) * spread, 0.0f);
                p.Color = glm::vec4(COLOR_CYAN, 1.0f);  // Start as cyan (unbounced)
                p._pad[0] = 0.0f;  // Reset color state
                p.Life = MaxLife * (0.5f + 0.5f * randFloat(seed));
                emitted++;
            }
        }
        
        // Update remaining live particles (those not re-emitted above)
        for (uint32_t i = 0; i < m_MaxParticles; ++i) {
            Particle& p = m_ParticlesCPU[i];
            if (p.Life > 0.0f && p._pad[0] >= 0.0f) {  // Already processed in emit loop? Skip via flag
                p.Velocity += glm::vec4(Gravity * dt, 0.0f);
                p.Position += p.Velocity * dt;
                
                if (p.Position.y < FloorY) {
                    p.Position.y = FloorY;
                    p.Velocity.y = -p.Velocity.y * Restitution;
                    p.Velocity *= Damping;
                    
                    // Change color state on bounce
                    if (p._pad[0] < 0.5f) p._pad[0] = 1.0f;
                    if (glm::length(glm::vec3(p.Velocity)) < 0.5f) p._pad[0] = 2.0f;
                }
                
                p.Life -= dt;
                float alpha = glm::clamp(p.Life / MaxLife, 0.0f, 1.0f);
                
                // Set color based on state
                glm::vec3 baseColor;
                if (p._pad[0] < 0.5f) {
                    baseColor = COLOR_CYAN;
                } else if (p._pad[0] < 1.5f) {
                    baseColor = COLOR_YELLOW;
                } else {
                    baseColor = COLOR_MAGENTA;
                }
                p.Color = glm::vec4(baseColor, alpha);
            }
        }
        
        // Upload to VBO
        glBindBuffer(GL_ARRAY_BUFFER, m_ParticleSSBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                        static_cast<GLsizeiptr>(m_MaxParticles * sizeof(Particle)),
                        m_ParticlesCPU.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void GPUParticleSystem::Render(const glm::mat4& viewProj) {
        if (!m_Initialized || m_RenderProgram == 0) return;

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
        
        if (m_UseComputeShaders) {
            // GPU path - read from SSBO via gl_InstanceID
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ParticleSSBO);
            glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(m_MaxParticles));
        } else {
            // CPU path - render from VBO vertex attributes
            glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_MaxParticles));
        }
        
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
        
        m_ParticlesCPU.clear();
        m_ParticlesCPU.shrink_to_fit();

        m_Initialized = false;
        m_UseComputeShaders = false;
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
        // Use GLSL 4.30 shaders for GPU path, GLSL 3.30 for CPU fallback
        const char* vertSource = m_UseComputeShaders ? s_VertexShaderSource_430 : s_VertexShaderSource_330;
        shaders[0] = CompileShader(GL_VERTEX_SHADER, vertSource);
        shaders[1] = CompileShader(GL_FRAGMENT_SHADER, s_FragmentShaderSource);
        if (shaders[0] == 0 || shaders[1] == 0) {
            if (shaders[0]) glDeleteShader(shaders[0]);
            if (shaders[1]) glDeleteShader(shaders[1]);
            return;
        }
        m_RenderProgram = LinkProgram(shaders, 2);
    }

} // namespace GameEngine
