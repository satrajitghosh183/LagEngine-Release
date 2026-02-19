#include "FluidEmitterComponent.hpp"
#include "TransformComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"
#include <random>
#include <ctime>

namespace GameEngine {

    static std::mt19937 s_RandomEngine(static_cast<unsigned int>(std::time(nullptr)));

    nlohmann::json FluidEmitterComponent::Serialize() const {
        nlohmann::json data;
        data["type"] = GetTypeName();
        data["shape"] = static_cast<int>(Shape);
        data["emissionRate"] = EmissionRate;
        data["initialVelocity"] = { InitialVelocity.x, InitialVelocity.y, InitialVelocity.z };
        data["velocityRandomness"] = VelocityRandomness;
        data["boxSize"] = { BoxSize.x, BoxSize.y, BoxSize.z };
        data["sphereRadius"] = SphereRadius;
        data["particleLifetime"] = ParticleLifetime;
        data["boundsMin"] = { BoundsMin.x, BoundsMin.y, BoundsMin.z };
        data["boundsMax"] = { BoundsMax.x, BoundsMax.y, BoundsMax.z };
        data["particleMass"] = ParticleMass;
        data["restDensity"] = RestDensity;
        data["viscosity"] = Viscosity;
        data["surfaceTension"] = SurfaceTension;
        data["emitting"] = Emitting;
        data["simulationEnabled"] = SimulationEnabled;
        data["maxParticles"] = MaxParticles;
        return data;
    }
    
    void FluidEmitterComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("shape")) Shape = static_cast<EmitterShape>(data["shape"].get<int>());
        if (data.contains("emissionRate")) EmissionRate = data["emissionRate"];
        if (data.contains("initialVelocity")) {
            InitialVelocity.x = data["initialVelocity"][0];
            InitialVelocity.y = data["initialVelocity"][1];
            InitialVelocity.z = data["initialVelocity"][2];
        }
        if (data.contains("velocityRandomness")) VelocityRandomness = data["velocityRandomness"];
        if (data.contains("boxSize")) {
            BoxSize.x = data["boxSize"][0];
            BoxSize.y = data["boxSize"][1];
            BoxSize.z = data["boxSize"][2];
        }
        if (data.contains("sphereRadius")) SphereRadius = data["sphereRadius"];
        if (data.contains("particleLifetime")) ParticleLifetime = data["particleLifetime"];
        if (data.contains("boundsMin")) {
            BoundsMin.x = data["boundsMin"][0];
            BoundsMin.y = data["boundsMin"][1];
            BoundsMin.z = data["boundsMin"][2];
        }
        if (data.contains("boundsMax")) {
            BoundsMax.x = data["boundsMax"][0];
            BoundsMax.y = data["boundsMax"][1];
            BoundsMax.z = data["boundsMax"][2];
        }
        if (data.contains("particleMass")) ParticleMass = data["particleMass"];
        if (data.contains("restDensity")) RestDensity = data["restDensity"];
        if (data.contains("viscosity")) Viscosity = data["viscosity"];
        if (data.contains("surfaceTension")) SurfaceTension = data["surfaceTension"];
        if (data.contains("emitting")) Emitting = data["emitting"];
        if (data.contains("simulationEnabled")) SimulationEnabled = data["simulationEnabled"];
        if (data.contains("maxParticles")) MaxParticles = data["maxParticles"];
    }

    void FluidEmitterComponent::OnCreate() {
        // Create solver if not set externally
        if (!m_Solver) {
            m_Solver = CreateRef<Physics::SPHSolver>(MaxParticles);
            
            // Configure solver
            m_Solver->ParticleMass = ParticleMass;
            m_Solver->RestDensity = RestDensity;
            m_Solver->Viscosity = Viscosity;
            m_Solver->SurfaceTension = SurfaceTension;
            m_Solver->BoundsMin = BoundsMin;
            m_Solver->BoundsMax = BoundsMax;
        }
        
        GE_CORE_DEBUG("FluidEmitterComponent::OnCreate - MaxParticles: {}", MaxParticles);
    }

    void FluidEmitterComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!m_Solver) return;
        
        // Emit particles
        if (Emitting && GetParticleCount() < MaxParticles) {
            m_EmissionAccumulator += EmissionRate * fixedDeltaTime;
            
            int particlesToEmit = static_cast<int>(m_EmissionAccumulator);
            m_EmissionAccumulator -= particlesToEmit;
            
            for (int i = 0; i < particlesToEmit && GetParticleCount() < MaxParticles; i++) {
                glm::vec3 position = GetEmitPosition();
                glm::vec3 velocity = GetEmitVelocity();
                m_Solver->AddParticle(position, velocity);
            }
        }
        
        // Update simulation
        if (SimulationEnabled) {
            m_Solver->Update(fixedDeltaTime);
        }
    }

    void FluidEmitterComponent::OnDestroy() {
        m_Solver.reset();
        GE_CORE_DEBUG("FluidEmitterComponent::OnDestroy");
    }

    void FluidEmitterComponent::EmitBurst(int count) {
        if (!m_Solver) return;
        
        for (int i = 0; i < count && GetParticleCount() < MaxParticles; i++) {
            glm::vec3 position = GetEmitPosition();
            glm::vec3 velocity = GetEmitVelocity();
            m_Solver->AddParticle(position, velocity);
        }
    }

    int FluidEmitterComponent::GetParticleCount() const {
        return m_Solver ? static_cast<int>(m_Solver->GetParticles().size()) : 0;
    }

    const std::vector<Physics::FluidParticle>& FluidEmitterComponent::GetParticles() const {
        static std::vector<Physics::FluidParticle> empty;
        return m_Solver ? m_Solver->GetParticles() : empty;
    }

    void FluidEmitterComponent::Clear() {
        if (m_Solver) {
            m_Solver->Clear();
        }
    }

    glm::vec3 FluidEmitterComponent::GetEmitPosition() const {
        // Get entity transform via Owner pointer
        glm::vec3 basePosition(0.0f);
        
        if (Owner && Owner->HasComponent<TransformComponent>()) {
            basePosition = Owner->GetComponent<TransformComponent>().Position;
        }
        
        glm::vec3 offset(0.0f);
        
        switch (Shape) {
            case EmitterShape::Point:
                // No offset
                break;
                
            case EmitterShape::Box:
                offset.x = RandomFloat(-BoxSize.x * 0.5f, BoxSize.x * 0.5f);
                offset.y = RandomFloat(-BoxSize.y * 0.5f, BoxSize.y * 0.5f);
                offset.z = RandomFloat(-BoxSize.z * 0.5f, BoxSize.z * 0.5f);
                break;
                
            case EmitterShape::Sphere:
                offset = RandomInUnitSphere() * SphereRadius;
                break;
        }
        
        return basePosition + offset;
    }

    glm::vec3 FluidEmitterComponent::GetEmitVelocity() const {
        glm::vec3 velocity = InitialVelocity;
        
        // Add randomness
        if (VelocityRandomness > 0.0f) {
            glm::vec3 random = RandomInUnitSphere() * VelocityRandomness;
            velocity += random * glm::length(InitialVelocity);
        }
        
        return velocity;
    }

    float FluidEmitterComponent::RandomFloat(float min, float max) const {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_RandomEngine);
    }

    glm::vec3 FluidEmitterComponent::RandomInUnitSphere() const {
        glm::vec3 result;
        do {
            result.x = RandomFloat(-1.0f, 1.0f);
            result.y = RandomFloat(-1.0f, 1.0f);
            result.z = RandomFloat(-1.0f, 1.0f);
        } while (glm::dot(result, result) > 1.0f);
        
        return result;
    }

}
