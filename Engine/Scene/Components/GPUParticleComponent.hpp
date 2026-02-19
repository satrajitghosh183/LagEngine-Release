#pragma once

#include "Component.hpp"
#include "../../ParticleSystem/GPUParticleSystem.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief ECS component that wraps a GPUParticleSystem.
     *
     * Attach this to an Entity to spawn a compute-shader-driven particle
     * effect.  All tuneable parameters are exposed as public members so
     * the editor / serialisation layer can modify them freely.
     */
    class GPUParticleComponent : public Component {
    public:
        COMPONENT_TYPE(GPUParticleComponent)

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;

        void OnCreate() override;
        void OnFixedUpdate(float fixedDeltaTime) override;
        void OnDestroy() override;

        /**
         * @brief Issue the GPU draw call for this particle system.
         *
         * Typically called by the renderer rather than the fixed-update loop.
         */
        void RenderParticles(const glm::mat4& viewProj);

        /** Get the underlying GPU particle system (may be null before OnCreate). */
        Ref<GPUParticleSystem> GetParticleSystem() const { return m_ParticleSystem; }

        // ---------------------------------------------------------------
        // Public properties  (serialised / editable)
        // ---------------------------------------------------------------
        int       MaxParticles = 100000;
        glm::vec3 Gravity      = glm::vec3(0.0f, -9.81f, 0.0f);
        float     Restitution  = 0.5f;
        float     Damping      = 0.99f;
        float     EmitHeight   = 3.0f;
        float     FloorY       = 0.0f;
        float     ParticleSize = 0.02f;
        int       EmitRate     = 1000;

    private:
        Ref<GPUParticleSystem> m_ParticleSystem;
    };

} // namespace GameEngine
