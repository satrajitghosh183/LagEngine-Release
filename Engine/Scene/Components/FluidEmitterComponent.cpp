#include "FluidEmitterComponent.hpp"
#include "TransformComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"
#include <random>
#include <ctime>

namespace GameEngine {

    static std::mt19937 s_RandEng(static_cast<unsigned int>(std::time(nullptr)));

    // =========================================================================
    // Serialization
    // =========================================================================

    nlohmann::json FluidEmitterComponent::Serialize() const {
        nlohmann::json data;
        data["type"]               = GetTypeName();
        data["shape"]              = static_cast<int>(Shape);
        data["emissionRate"]       = EmissionRate;
        data["initialVelocity"]    = { InitialVelocity.x, InitialVelocity.y, InitialVelocity.z };
        data["velocityRandomness"] = VelocityRandomness;
        data["boxSize"]            = { BoxSize.x, BoxSize.y, BoxSize.z };
        data["sphereRadius"]       = SphereRadius;
        data["particleLifetime"]   = ParticleLifetime;
        data["boundsMin"]          = { BoundsMin.x, BoundsMin.y, BoundsMin.z };
        data["boundsMax"]          = { BoundsMax.x, BoundsMax.y, BoundsMax.z };
        data["smoothingLength"]    = SmoothingLength;
        data["restDensity"]        = RestDensity;
        data["pressureCoeff"]      = PressureCoefficient;
        data["viscosityCoeff"]     = ViscosityCoefficient;
        data["bounceCoeff"]        = BounceCoefficient;
        data["particleSize"]       = ParticleSize;
        data["emitting"]           = Emitting;
        data["simulationEnabled"]  = SimulationEnabled;
        data["maxParticles"]       = MaxParticles;
        data["initBlockNx"]        = InitBlockNx;
        data["initBlockNy"]        = InitBlockNy;
        data["initBlockNz"]        = InitBlockNz;
        data["initBlockSpacing"]   = InitBlockSpacing;
        return data;
    }

    void FluidEmitterComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("shape"))              Shape = static_cast<EmitterShape>(data["shape"].get<int>());
        if (data.contains("emissionRate"))       EmissionRate = data["emissionRate"];
        if (data.contains("initialVelocity")) {
            InitialVelocity = { data["initialVelocity"][0], data["initialVelocity"][1], data["initialVelocity"][2] };
        }
        if (data.contains("velocityRandomness")) VelocityRandomness = data["velocityRandomness"];
        if (data.contains("boxSize"))            BoxSize = { data["boxSize"][0], data["boxSize"][1], data["boxSize"][2] };
        if (data.contains("sphereRadius"))       SphereRadius = data["sphereRadius"];
        if (data.contains("particleLifetime"))   ParticleLifetime = data["particleLifetime"];
        if (data.contains("boundsMin"))          BoundsMin = { data["boundsMin"][0], data["boundsMin"][1], data["boundsMin"][2] };
        if (data.contains("boundsMax"))          BoundsMax = { data["boundsMax"][0], data["boundsMax"][1], data["boundsMax"][2] };
        if (data.contains("smoothingLength"))    SmoothingLength    = data["smoothingLength"];
        if (data.contains("restDensity"))        RestDensity        = data["restDensity"];
        if (data.contains("pressureCoeff"))      PressureCoefficient  = data["pressureCoeff"];
        if (data.contains("viscosityCoeff"))     ViscosityCoefficient = data["viscosityCoeff"];
        if (data.contains("bounceCoeff"))        BounceCoefficient  = data["bounceCoeff"];
        if (data.contains("particleSize"))       ParticleSize       = data["particleSize"];
        if (data.contains("emitting"))           Emitting           = data["emitting"];
        if (data.contains("simulationEnabled"))  SimulationEnabled  = data["simulationEnabled"];
        if (data.contains("maxParticles"))       MaxParticles       = data["maxParticles"];
        if (data.contains("initBlockNx"))        InitBlockNx        = data["initBlockNx"];
        if (data.contains("initBlockNy"))        InitBlockNy        = data["initBlockNy"];
        if (data.contains("initBlockNz"))        InitBlockNz        = data["initBlockNz"];
        if (data.contains("initBlockSpacing"))   InitBlockSpacing   = data["initBlockSpacing"];
    }

    // =========================================================================
    // ECS lifecycle
    // =========================================================================

    void FluidEmitterComponent::OnCreate() {
        if (!m_Solver) {
            m_Solver = CreateRef<Physics::SPHFluidSolver>();
        }
        // Push tuning params into the solver (matches old_code/src/sph_water.cpp defaults)
        m_Solver->SmoothingLength       = SmoothingLength;
        m_Solver->RestDensity           = RestDensity;
        m_Solver->PressureCoefficient   = PressureCoefficient;
        m_Solver->ViscosityCoefficient  = ViscosityCoefficient;
        m_Solver->BounceCoefficient     = BounceCoefficient;
        m_Solver->DomainMin             = BoundsMin;
        m_Solver->DomainMax             = BoundsMax;

        // Auto-spawn block when not using continuous emission
        if (!Emitting && InitBlockNx > 0 && InitBlockNy > 0 && InitBlockNz > 0) {
            InitWaterBlock(InitBlockNx, InitBlockNy, InitBlockNz, InitBlockSpacing);
        }

        GE_CORE_DEBUG("FluidEmitterComponent::OnCreate (SPHFluidSolver, {} particles)", GetParticleCount());
    }

    void FluidEmitterComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!m_Solver || !SimulationEnabled) return;

        // Continuous particle emission
        if (Emitting && GetParticleCount() < MaxParticles) {
            m_EmissionAccumulator += EmissionRate * fixedDeltaTime;
            int toEmit = static_cast<int>(m_EmissionAccumulator);
            m_EmissionAccumulator -= toEmit;
            for (int i = 0; i < toEmit && GetParticleCount() < MaxParticles; ++i) {
                m_Solver->AddParticle(GetEmitPosition(), GetEmitVelocity());
            }
        }

        // SPH step — use small substep like old code (dt=0.004, 2 substeps/frame)
        const float sphDt    = 0.004f;
        float       accum    = fixedDeltaTime;
        while (accum >= sphDt) {
            m_Solver->Update(sphDt);
            accum -= sphDt;
        }
        if (accum > 0.0f)
            m_Solver->Update(accum);
    }

    void FluidEmitterComponent::OnDestroy() {
        m_Solver.reset();
        GE_CORE_DEBUG("FluidEmitterComponent::OnDestroy");
    }

    // =========================================================================
    // Block water init (port of old_code/src/sph_water.cpp SPH::init())
    // =========================================================================

    void FluidEmitterComponent::InitWaterBlock(int nx, int ny, int nz, float spacing) {
        if (!m_Solver) {
            m_Solver = CreateRef<Physics::SPHFluidSolver>();
            m_Solver->SmoothingLength      = SmoothingLength;
            m_Solver->RestDensity          = RestDensity;
            m_Solver->PressureCoefficient  = PressureCoefficient;
            m_Solver->ViscosityCoefficient = ViscosityCoefficient;
            m_Solver->BounceCoefficient    = BounceCoefficient;
            m_Solver->DomainMin            = BoundsMin;
            m_Solver->DomainMax            = BoundsMax;
        }

        // Clamp block inside domain with a small margin (like old code)
        glm::vec3 margin(0.08f, 0.05f, 0.08f);
        glm::vec3 origin = BoundsMin + margin;
        m_Solver->InitBlock(nx, ny, nz, spacing, origin);

        GE_CORE_INFO("FluidEmitterComponent: InitWaterBlock {}x{}x{} = {} particles",
                     nx, ny, nz, nx * ny * nz);
    }

    // =========================================================================
    // Queries
    // =========================================================================

    void FluidEmitterComponent::EmitBurst(int count) {
        if (!m_Solver) return;
        for (int i = 0; i < count && GetParticleCount() < MaxParticles; ++i)
            m_Solver->AddParticle(GetEmitPosition(), GetEmitVelocity());
    }

    int FluidEmitterComponent::GetParticleCount() const {
        return m_Solver ? m_Solver->GetParticleCount() : 0;
    }

    const std::vector<Physics::FluidParticle>& FluidEmitterComponent::GetParticles() const {
        static const std::vector<Physics::FluidParticle> empty;
        return m_Solver ? m_Solver->GetParticles() : empty;
    }

    std::vector<glm::vec3> FluidEmitterComponent::GetParticlePositions() const {
        if (!m_Solver) return {};
        return m_Solver->GetPositions();  // efficient copy of positions only
    }

    void FluidEmitterComponent::Clear() {
        if (m_Solver) m_Solver->Clear();
    }

    // =========================================================================
    // Helpers
    // =========================================================================

    glm::vec3 FluidEmitterComponent::GetEmitPosition() const {
        glm::vec3 base(0.0f);
        if (GetOwnerEntity() && GetOwnerEntity().HasComponent<TransformComponent>())
            base = GetOwnerEntity().GetComponent<TransformComponent>().Position;

        glm::vec3 offset(0.0f);
        switch (Shape) {
            case EmitterShape::Box:
                offset = { RandomFloat(-BoxSize.x*0.5f, BoxSize.x*0.5f),
                           RandomFloat(-BoxSize.y*0.5f, BoxSize.y*0.5f),
                           RandomFloat(-BoxSize.z*0.5f, BoxSize.z*0.5f) };
                break;
            case EmitterShape::Sphere:
                offset = RandomInUnitSphere() * SphereRadius;
                break;
            default: break;
        }
        return base + offset;
    }

    glm::vec3 FluidEmitterComponent::GetEmitVelocity() const {
        glm::vec3 v = InitialVelocity;
        if (VelocityRandomness > 0.0f)
            v += RandomInUnitSphere() * VelocityRandomness * glm::length(InitialVelocity);
        return v;
    }

    float FluidEmitterComponent::RandomFloat(float lo, float hi) const {
        std::uniform_real_distribution<float> dist(lo, hi);
        return dist(s_RandEng);
    }

    glm::vec3 FluidEmitterComponent::RandomInUnitSphere() const {
        glm::vec3 r;
        do {
            r = { RandomFloat(-1,1), RandomFloat(-1,1), RandomFloat(-1,1) };
        } while (glm::dot(r,r) > 1.0f);
        return r;
    }

} // namespace GameEngine
