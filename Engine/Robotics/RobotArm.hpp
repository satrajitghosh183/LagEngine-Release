#pragma once

#include "DHParameter.hpp"
#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Robot arm model using Denavit-Hartenberg parameters
     *
     * Provides forward kinematics and Jacobian computation
     * for an arbitrary serial kinematic chain.
     */
    class RobotArm {
    public:
        RobotArm() = default;
        explicit RobotArm(const std::vector<DHParameter>& dhParams);

        /**
         * @brief Set DH parameters for all joints
         */
        void SetDHParameters(const std::vector<DHParameter>& dhParams);

        /**
         * @brief Get current DH parameter list
         */
        const std::vector<DHParameter>& GetDHParameters() const { return m_DHParams; }

        /**
         * @brief Number of joints
         */
        size_t GetNumJoints() const { return m_DHParams.size(); }

        /**
         * @brief Forward kinematics: compute end-effector position
         * @param jointAngles  Joint variable values (theta for revolute, d for prismatic)
         */
        glm::vec3 GetEndEffectorPosition(const std::vector<float>& jointAngles) const;

        /**
         * @brief Forward kinematics: compute full end-effector transform
         */
        glm::mat4 GetEndEffectorTransform(const std::vector<float>& jointAngles) const;

        /**
         * @brief Compute the 6xN numerical Jacobian using finite differences
         *
         * Returns a vector of column vectors. Each column corresponds to one joint.
         * Top 3 rows = linear velocity, bottom 3 rows = angular velocity.
         * Stored as column-major: result[joint] = { dx, dy, dz, dwx, dwy, dwz }
         */
        std::vector<std::array<float, 6>> GetJacobian(const std::vector<float>& jointAngles) const;

    private:
        std::vector<DHParameter> m_DHParams;
    };

}
