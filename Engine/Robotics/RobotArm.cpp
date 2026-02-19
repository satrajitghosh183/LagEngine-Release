#include "RobotArm.hpp"
#include <glm/gtc/matrix_access.hpp>
#include <cmath>
#include <cassert>

namespace GameEngine {

    RobotArm::RobotArm(const std::vector<DHParameter>& dhParams)
        : m_DHParams(dhParams) {}

    void RobotArm::SetDHParameters(const std::vector<DHParameter>& dhParams) {
        m_DHParams = dhParams;
    }

    glm::mat4 RobotArm::GetEndEffectorTransform(const std::vector<float>& jointAngles) const {
        assert(jointAngles.size() >= m_DHParams.size());

        glm::mat4 T(1.0f);

        for (size_t i = 0; i < m_DHParams.size(); ++i) {
            DHParameter dh = m_DHParams[i];

            // Apply joint variable
            if (dh.Type == DHParameter::JointType::Revolute) {
                dh.theta = jointAngles[i];
            } else {
                dh.d = jointAngles[i];
            }

            T = T * dh.GetTransformMatrix();
        }

        return T;
    }

    glm::vec3 RobotArm::GetEndEffectorPosition(const std::vector<float>& jointAngles) const {
        glm::mat4 T = GetEndEffectorTransform(jointAngles);
        return glm::vec3(T[3]);
    }

    std::vector<std::array<float, 6>> RobotArm::GetJacobian(const std::vector<float>& jointAngles) const {
        const size_t n = m_DHParams.size();
        std::vector<std::array<float, 6>> jacobian(n);

        const float epsilon = 1e-4f;

        // Reference end-effector transform
        glm::mat4 T0 = GetEndEffectorTransform(jointAngles);
        glm::vec3 p0 = glm::vec3(T0[3]);

        // Extract reference rotation columns for angular velocity computation
        glm::vec3 rx0 = glm::vec3(T0[0]);
        glm::vec3 ry0 = glm::vec3(T0[1]);
        glm::vec3 rz0 = glm::vec3(T0[2]);

        for (size_t i = 0; i < n; ++i) {
            std::vector<float> perturbedAngles = jointAngles;
            perturbedAngles[i] += epsilon;

            glm::mat4 T1 = GetEndEffectorTransform(perturbedAngles);
            glm::vec3 p1 = glm::vec3(T1[3]);

            // Linear velocity component (dp/dq)
            glm::vec3 dp = (p1 - p0) / epsilon;
            jacobian[i][0] = dp.x;
            jacobian[i][1] = dp.y;
            jacobian[i][2] = dp.z;

            // Angular velocity component via finite difference of rotation
            // Use the skew-symmetric part of dR * R^T
            glm::vec3 rx1 = glm::vec3(T1[0]);
            glm::vec3 ry1 = glm::vec3(T1[1]);
            glm::vec3 rz1 = glm::vec3(T1[2]);

            // Approximate angular velocity:
            // omega = (1/eps) * 0.5 * ( cross(rx0, rx1) + cross(ry0, ry1) + cross(rz0, rz1) )
            glm::vec3 omega = (1.0f / epsilon) * 0.5f * (
                glm::cross(rx0, rx1) + glm::cross(ry0, ry1) + glm::cross(rz0, rz1)
            );

            jacobian[i][3] = omega.x;
            jacobian[i][4] = omega.y;
            jacobian[i][5] = omega.z;
        }

        return jacobian;
    }

}
