#pragma once

#include "Component.hpp"
#include "../../Physics/SoftBody/XPBDSolver.hpp"
#include "../../Graphics/Mesh3D.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    class SoftBodyComponent : public Component {
    public:
        SoftBodyComponent() = default;
        ~SoftBodyComponent() = default;

        COMPONENT_TYPE(SoftBodyComponent)

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;

        void OnCreate() override;
        void OnFixedUpdate(float fixedDeltaTime) override;
        void OnDestroy() override;

        void UpdateMesh();
        Physics::XPBDSolver* GetSolver() { return m_Solver.get(); }

        // Public properties
        float Mass = 1.0f;
        float Damping = 0.99f;
        int SubSteps = 4;
        bool EnableDistanceConstraints = true;
        bool EnableBendConstraints = true;
        bool EnableVolumeConstraints = false;
        bool EnableCollisionConstraints = true;
        float DistanceCompliance = 0.0f;
        float BendCompliance = 0.001f;
        int GridResX = 10;
        int GridResY = 10;
        float Width = 2.0f;
        float Height = 2.0f;
        bool IsSimulating = true;

    private:
        void InitializeGrid();

        Scope<Physics::XPBDSolver> m_Solver;
        Ref<Mesh3D> m_Mesh;
    };

}
