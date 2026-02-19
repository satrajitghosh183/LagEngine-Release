#include "RobotArmComponent.hpp"
#include "../../Core/Logger.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {

    RobotArmComponent::RobotArmComponent() {
        // Default 3-joint planar arm
        DHParameter joint1;
        joint1.a = 1.0f;
        joint1.alpha = 0.0f;
        joint1.d = 0.0f;
        joint1.theta = 0.0f;
        joint1.Type = DHParameter::JointType::Revolute;
        joint1.MinLimit = -3.14159f;
        joint1.MaxLimit =  3.14159f;

        DHParameter joint2;
        joint2.a = 0.8f;
        joint2.alpha = 0.0f;
        joint2.d = 0.0f;
        joint2.theta = 0.0f;
        joint2.Type = DHParameter::JointType::Revolute;
        joint2.MinLimit = -3.14159f;
        joint2.MaxLimit =  3.14159f;

        DHParameter joint3;
        joint3.a = 0.5f;
        joint3.alpha = 0.0f;
        joint3.d = 0.0f;
        joint3.theta = 0.0f;
        joint3.Type = DHParameter::JointType::Revolute;
        joint3.MinLimit = -3.14159f;
        joint3.MaxLimit =  3.14159f;

        DHParams = { joint1, joint2, joint3 };
        JointAngles = { 0.0f, 0.0f, 0.0f };
    }

    void RobotArmComponent::OnCreate() {
        // Build the robot arm model from DH parameters
        m_Arm.SetDHParameters(DHParams);

        // Ensure joint angles vector is correct size
        if (JointAngles.size() != DHParams.size()) {
            JointAngles.resize(DHParams.size(), 0.0f);
        }

        // Initialize target angles to current angles
        m_TargetAngles = JointAngles;

        // Create a PID controller per joint
        m_PIDControllers.clear();
        m_PIDControllers.reserve(DHParams.size());
        for (size_t i = 0; i < DHParams.size(); ++i) {
            m_PIDControllers.emplace_back(Kp, Ki, Kd, -10.0f, 10.0f);
        }

        // Configure IK solver
        m_Solver.MaxIterations = IKMaxIterations;
        m_Solver.Tolerance = IKTolerance;
        m_Solver.StepSize = 0.1f;

        GE_CORE_DEBUG("RobotArmComponent: Initialized with {} joints", DHParams.size());
    }

    void RobotArmComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!Enabled) return;

        // Run IK if enabled to compute target joint angles
        if (IKEnabled) {
            m_Solver.MaxIterations = IKMaxIterations;
            m_Solver.Tolerance = IKTolerance;

            IKResult result = m_Solver.Solve(m_Arm, JointAngles, TargetPosition);
            m_TargetAngles = result.jointAngles;
        }

        // Apply PID control to move each joint toward its target angle
        for (size_t i = 0; i < DHParams.size(); ++i) {
            if (i >= m_PIDControllers.size()) break;

            // Update PID gains in case they changed at runtime
            m_PIDControllers[i].Kp = Kp;
            m_PIDControllers[i].Ki = Ki;
            m_PIDControllers[i].Kd = Kd;

            float error = m_TargetAngles[i] - JointAngles[i];
            float correction = m_PIDControllers[i].Update(error, fixedDeltaTime);

            JointAngles[i] += correction * fixedDeltaTime;

            // Clamp to joint limits
            JointAngles[i] = std::clamp(JointAngles[i],
                                        DHParams[i].MinLimit,
                                        DHParams[i].MaxLimit);
        }
    }

    glm::vec3 RobotArmComponent::GetEndEffectorPosition() const {
        return m_Arm.GetEndEffectorPosition(JointAngles);
    }

    glm::mat4 RobotArmComponent::GetEndEffectorTransform() const {
        return m_Arm.GetEndEffectorTransform(JointAngles);
    }

    nlohmann::json RobotArmComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;

        // Serialize DH parameters
        nlohmann::json dhArray = nlohmann::json::array();
        for (const auto& dh : DHParams) {
            nlohmann::json dhObj;
            dhObj["a"] = dh.a;
            dhObj["alpha"] = dh.alpha;
            dhObj["d"] = dh.d;
            dhObj["theta"] = dh.theta;
            dhObj["jointType"] = static_cast<int>(dh.Type);
            dhObj["minLimit"] = dh.MinLimit;
            dhObj["maxLimit"] = dh.MaxLimit;
            dhArray.push_back(dhObj);
        }
        j["dhParams"] = dhArray;

        // Serialize joint angles
        j["jointAngles"] = JointAngles;

        // Serialize target and IK settings
        j["targetPosition"] = { TargetPosition.x, TargetPosition.y, TargetPosition.z };
        j["ikEnabled"] = IKEnabled;
        j["ikTolerance"] = IKTolerance;
        j["ikMaxIterations"] = IKMaxIterations;

        // Serialize PID gains
        j["kp"] = Kp;
        j["ki"] = Ki;
        j["kd"] = Kd;

        return j;
    }

    void RobotArmComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];

        if (data.contains("dhParams")) {
            DHParams.clear();
            for (const auto& dhObj : data["dhParams"]) {
                DHParameter dh;
                if (dhObj.contains("a"))         dh.a = dhObj["a"];
                if (dhObj.contains("alpha"))     dh.alpha = dhObj["alpha"];
                if (dhObj.contains("d"))         dh.d = dhObj["d"];
                if (dhObj.contains("theta"))     dh.theta = dhObj["theta"];
                if (dhObj.contains("jointType")) dh.Type = static_cast<DHParameter::JointType>(dhObj["jointType"].get<int>());
                if (dhObj.contains("minLimit"))  dh.MinLimit = dhObj["minLimit"];
                if (dhObj.contains("maxLimit"))  dh.MaxLimit = dhObj["maxLimit"];
                DHParams.push_back(dh);
            }
        }

        if (data.contains("jointAngles")) {
            JointAngles.clear();
            for (const auto& val : data["jointAngles"]) {
                JointAngles.push_back(val.get<float>());
            }
        }

        if (data.contains("targetPosition")) {
            auto tp = data["targetPosition"];
            TargetPosition = glm::vec3(tp[0], tp[1], tp[2]);
        }

        if (data.contains("ikEnabled"))
            IKEnabled = data["ikEnabled"];

        if (data.contains("ikTolerance"))
            IKTolerance = data["ikTolerance"];

        if (data.contains("ikMaxIterations"))
            IKMaxIterations = data["ikMaxIterations"];

        if (data.contains("kp")) Kp = data["kp"];
        if (data.contains("ki")) Ki = data["ki"];
        if (data.contains("kd")) Kd = data["kd"];
    }

}
