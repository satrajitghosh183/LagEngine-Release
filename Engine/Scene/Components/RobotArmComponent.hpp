#pragma once

#include "Component.hpp"
#include "../../Robotics/DHParameter.hpp"
#include "../../Robotics/RobotArm.hpp"
#include "../../Robotics/IKSolver.hpp"
#include "../../Robotics/PIDController.hpp"
#include "../../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Component that attaches a robot arm (FK/IK) to an entity
     *
     * Manages DH parameters, joint state, IK solving, and PID-based
     * joint control within the ECS update loop.
     */
    class RobotArmComponent : public Component {
    public:
        RobotArmComponent();

        COMPONENT_TYPE(RobotArmComponent)

        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;

        void OnCreate() override;
        void OnFixedUpdate(float fixedDeltaTime) override;

        /**
         * @brief Get current end-effector world position via FK
         */
        glm::vec3 GetEndEffectorPosition() const;

        /**
         * @brief Get current end-effector transform via FK
         */
        glm::mat4 GetEndEffectorTransform() const;

        /**
         * @brief Get the underlying RobotArm model
         */
        const RobotArm& GetRobotArm() const { return m_Arm; }

    public:
        // DH parameter chain (editable, re-built on OnCreate)
        std::vector<DHParameter> DHParams;

        // Current joint angles
        std::vector<float> JointAngles;

        // IK target
        glm::vec3 TargetPosition = glm::vec3(1.0f, 1.0f, 0.0f);
        bool IKEnabled = false;

        // PID gains
        float Kp = 2.0f;
        float Ki = 0.0f;
        float Kd = 0.5f;

        // IK solver settings
        float IKTolerance = 0.001f;
        int   IKMaxIterations = 100;

    private:
        RobotArm m_Arm;
        IKSolver m_Solver;
        std::vector<float> m_TargetAngles;
        std::vector<PIDController> m_PIDControllers;
    };

}
