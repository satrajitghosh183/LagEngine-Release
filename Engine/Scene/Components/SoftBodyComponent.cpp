#include "SoftBodyComponent.hpp"
#include "../../Graphics/Mesh3D.hpp"
#include <glm/glm.hpp>
#include <algorithm>

namespace GameEngine {

    void SoftBodyComponent::OnCreate() {
        m_Solver = CreateScope<Physics::XPBDSolver>();
        m_Solver->SetDamping(Damping);
        InitializeGrid();
    }

    void SoftBodyComponent::InitializeGrid() {
        if (!m_Solver) return;
        m_Solver->Clear();

        float spacingX = Width / static_cast<float>(GridResX - 1);
        float spacingY = Height / static_cast<float>(GridResY - 1);

        // Create particles
        for (int y = 0; y < GridResY; y++) {
            for (int x = 0; x < GridResX; x++) {
                glm::vec3 pos(x * spacingX - Width * 0.5f, Height - y * spacingY, 0.0f);
                m_Solver->AddParticle(pos, Mass);
            }
        }

        // Pin top row
        for (int x = 0; x < GridResX; x++) {
            m_Solver->PinParticle(x);
        }

        auto idx = [this](int x, int y) { return y * GridResX + x; };

        // Distance constraints
        if (EnableDistanceConstraints) {
            for (int y = 0; y < GridResY; y++) {
                for (int x = 0; x < GridResX; x++) {
                    if (x < GridResX - 1) {
                        auto c = std::make_unique<Physics::DistanceConstraint>(idx(x, y), idx(x + 1, y), spacingX);
                        c->Compliance = DistanceCompliance;
                        m_Solver->AddConstraint(std::move(c));
                    }
                    if (y < GridResY - 1) {
                        auto c = std::make_unique<Physics::DistanceConstraint>(idx(x, y), idx(x, y + 1), spacingY);
                        c->Compliance = DistanceCompliance;
                        m_Solver->AddConstraint(std::move(c));
                    }
                }
            }
        }

        // Bend constraints (using diagonal)
        if (EnableBendConstraints) {
            for (int y = 0; y < GridResY - 1; y++) {
                for (int x = 0; x < GridResX - 1; x++) {
                    float diagLen = std::sqrt(spacingX * spacingX + spacingY * spacingY);
                    auto c = std::make_unique<Physics::DistanceConstraint>(idx(x, y), idx(x + 1, y + 1), diagLen);
                    c->Compliance = BendCompliance;
                    m_Solver->AddConstraint(std::move(c));
                }
            }
        }

        // Floor collision
        if (EnableCollisionConstraints) {
            for (int i = 0; i < GridResX * GridResY; i++) {
                m_Solver->AddConstraint(std::make_unique<Physics::CollisionConstraint>(
                    i, glm::vec3(0, 1, 0), -0.5f));
            }
        }
    }

    void SoftBodyComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!m_Solver || !IsSimulating) return;
        m_Solver->SetDamping(Damping);
        m_Solver->Solve(fixedDeltaTime, SubSteps);
        UpdateMesh();
    }

    void SoftBodyComponent::OnDestroy() {
        m_Solver.reset();
    }

    void SoftBodyComponent::UpdateMesh() {
        if (!m_Solver || !m_Mesh) return;

        const auto& particles = m_Solver->GetParticles();
        if (particles.empty()) return;

        // Copy existing vertices so indices/UVs are preserved
        std::vector<Vertex3D> verts(m_Mesh->GetVertices());
        size_t count = std::min(verts.size(), particles.size());

        for (size_t i = 0; i < count; i++) {
            verts[i].Position = particles[i].position;
        }

        // Recalculate per-vertex normals from neighbours
        for (int y = 1; y < GridResY - 1; y++) {
            for (int x = 1; x < GridResX - 1; x++) {
                int idx  = y * GridResX + x;
                if ((size_t)idx >= count) continue;
                glm::vec3 dx = verts[idx + 1].Position - verts[idx - 1].Position;
                glm::vec3 dy = verts[idx + GridResX].Position - verts[idx - GridResX].Position;
                verts[idx].Normal = glm::normalize(glm::cross(dx, dy));
            }
        }

        m_Mesh->UpdateVertices(verts);
    }

    nlohmann::json SoftBodyComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = "SoftBodyComponent";
        j["mass"] = Mass;
        j["damping"] = Damping;
        j["subSteps"] = SubSteps;
        j["distanceConstraints"] = EnableDistanceConstraints;
        j["bendConstraints"] = EnableBendConstraints;
        j["volumeConstraints"] = EnableVolumeConstraints;
        j["collisionConstraints"] = EnableCollisionConstraints;
        j["distanceCompliance"] = DistanceCompliance;
        j["bendCompliance"] = BendCompliance;
        j["gridResX"] = GridResX;
        j["gridResY"] = GridResY;
        j["width"] = Width;
        j["height"] = Height;
        j["simulating"] = IsSimulating;
        return j;
    }

    void SoftBodyComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("mass")) Mass = data["mass"];
        if (data.contains("damping")) Damping = data["damping"];
        if (data.contains("subSteps")) SubSteps = data["subSteps"];
        if (data.contains("distanceConstraints")) EnableDistanceConstraints = data["distanceConstraints"];
        if (data.contains("bendConstraints")) EnableBendConstraints = data["bendConstraints"];
        if (data.contains("volumeConstraints")) EnableVolumeConstraints = data["volumeConstraints"];
        if (data.contains("collisionConstraints")) EnableCollisionConstraints = data["collisionConstraints"];
        if (data.contains("distanceCompliance")) DistanceCompliance = data["distanceCompliance"];
        if (data.contains("bendCompliance")) BendCompliance = data["bendCompliance"];
        if (data.contains("gridResX")) GridResX = data["gridResX"];
        if (data.contains("gridResY")) GridResY = data["gridResY"];
        if (data.contains("width")) Width = data["width"];
        if (data.contains("height")) Height = data["height"];
        if (data.contains("simulating")) IsSimulating = data["simulating"];
    }

}
