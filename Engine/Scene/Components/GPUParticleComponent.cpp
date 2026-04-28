#include "GPUParticleComponent.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {

    // -------------------------------------------------------------------
    // Serialization
    // -------------------------------------------------------------------

    nlohmann::json GPUParticleComponent::Serialize() const {
        nlohmann::json j;
        j["type"]         = GetTypeName();
        j["enabled"]      = Enabled;
        j["maxParticles"] = MaxParticles;
        j["gravity"]      = { Gravity.x, Gravity.y, Gravity.z };
        j["restitution"]  = Restitution;
        j["damping"]      = Damping;
        j["emitHeight"]   = EmitHeight;
        j["floorY"]       = FloorY;
        j["particleSize"] = ParticleSize;
        j["emitRate"]     = EmitRate;
        return j;
    }

    void GPUParticleComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];

        if (data.contains("maxParticles"))
            MaxParticles = data["maxParticles"];

        if (data.contains("gravity")) {
            auto g = data["gravity"];
            Gravity = glm::vec3(g[0], g[1], g[2]);
        }

        if (data.contains("restitution"))
            Restitution = data["restitution"];

        if (data.contains("damping"))
            Damping = data["damping"];

        if (data.contains("emitHeight"))
            EmitHeight = data["emitHeight"];

        if (data.contains("floorY"))
            FloorY = data["floorY"];

        if (data.contains("particleSize"))
            ParticleSize = data["particleSize"];

        if (data.contains("emitRate"))
            EmitRate = data["emitRate"];
    }

    // -------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------

    void GPUParticleComponent::OnCreate() {
        m_ParticleSystem = CreateRef<GPUParticleSystem>();

        // Push current property values into the system before init
        m_ParticleSystem->Gravity      = Gravity;
        m_ParticleSystem->Restitution  = Restitution;
        m_ParticleSystem->Damping      = Damping;
        m_ParticleSystem->EmitHeight   = EmitHeight;
        m_ParticleSystem->FloorY       = FloorY;
        m_ParticleSystem->ParticleSize = ParticleSize;
        m_ParticleSystem->EmitRate     = EmitRate;

        // Init requires a Vulkan render pass to fully wire up GPU compute paths.
        // The renderer hooks this in via the editor/scene render path; here we
        // pre-allocate the CPU-side capacity so the component is usable in tests.
        m_ParticleSystem->Init(static_cast<uint32_t>(MaxParticles), VK_NULL_HANDLE);

        GE_CORE_INFO("GPUParticleComponent created with {} max particles", MaxParticles);
    }

    void GPUParticleComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!Enabled || !m_ParticleSystem) return;

        // Sync any property changes made via the editor at runtime
        m_ParticleSystem->Gravity      = Gravity;
        m_ParticleSystem->Restitution  = Restitution;
        m_ParticleSystem->Damping      = Damping;
        m_ParticleSystem->EmitHeight   = EmitHeight;
        m_ParticleSystem->FloorY       = FloorY;
        m_ParticleSystem->ParticleSize = ParticleSize;
        m_ParticleSystem->EmitRate     = EmitRate;

        // GPU compute dispatch needs an active Vulkan command buffer which is
        // only available in the render thread. The render system updates the
        // particle simulation each frame; the component only owns config.
        (void)fixedDeltaTime;
    }

    void GPUParticleComponent::OnDestroy() {
        if (m_ParticleSystem) {
            m_ParticleSystem->Shutdown();
            m_ParticleSystem.reset();
            GE_CORE_INFO("GPUParticleComponent destroyed");
        }
    }

    // -------------------------------------------------------------------
    // Rendering
    // -------------------------------------------------------------------

    void GPUParticleComponent::RenderParticles(const glm::mat4& viewProj) {
        if (!Enabled || !m_ParticleSystem) return;
        // Rendering must be issued by the engine's render thread with a
        // valid command buffer — the component just stores the matrix.
        (void)viewProj;
    }

} // namespace GameEngine
