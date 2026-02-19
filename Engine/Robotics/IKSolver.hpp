#pragma once

#include "RobotArm.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Result of an IK solve attempt
     */
    struct IKResult {
        std::vector<float> jointAngles;
        bool converged = false;
        int iterations = 0;
        float error = 0.0f;
    };

    /**
     * @brief Inverse Kinematics solver using Jacobian transpose method
     *
     * The Jacobian transpose method is simple and robust:
     *   dq = alpha * J^T * e
     * where e is the position error and alpha is a step size.
     */
    class IKSolver {
    public:
        IKSolver() = default;

        /**
         * @brief Solve IK for a target position
         * @param arm            The robot arm model
         * @param currentAngles  Starting joint angles
         * @param targetPosition Desired end-effector position in world space
         * @return IKResult with converged angles or best attempt
         */
        IKResult Solve(const RobotArm& arm,
                       const std::vector<float>& currentAngles,
                       const glm::vec3& targetPosition) const;

    public:
        int   MaxIterations = 100;
        float Tolerance     = 0.001f;
        float StepSize      = 0.1f;
    };

}
