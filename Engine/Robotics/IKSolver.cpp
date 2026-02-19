#include "IKSolver.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {

    IKResult IKSolver::Solve(const RobotArm& arm,
                             const std::vector<float>& currentAngles,
                             const glm::vec3& targetPosition) const {
        IKResult result;
        result.jointAngles = currentAngles;
        result.converged = false;
        result.iterations = 0;
        result.error = 0.0f;

        const auto& dhParams = arm.GetDHParameters();
        const size_t n = dhParams.size();

        if (n == 0 || currentAngles.size() < n) {
            return result;
        }

        std::vector<float> angles = currentAngles;

        for (int iter = 0; iter < MaxIterations; ++iter) {
            // Current end-effector position
            glm::vec3 currentPos = arm.GetEndEffectorPosition(angles);

            // Position error
            glm::vec3 error = targetPosition - currentPos;
            float errorMagnitude = glm::length(error);

            result.iterations = iter + 1;
            result.error = errorMagnitude;

            // Check convergence
            if (errorMagnitude < Tolerance) {
                result.converged = true;
                break;
            }

            // Compute Jacobian (only use linear part, top 3 rows)
            auto jacobian = arm.GetJacobian(angles);

            // Jacobian transpose method: dq = alpha * J^T * e
            for (size_t i = 0; i < n; ++i) {
                // J^T * e for joint i (dot product of Jacobian column's linear part with error)
                float jte = jacobian[i][0] * error.x
                          + jacobian[i][1] * error.y
                          + jacobian[i][2] * error.z;

                angles[i] += StepSize * jte;

                // Clamp to joint limits
                angles[i] = std::clamp(angles[i], dhParams[i].MinLimit, dhParams[i].MaxLimit);
            }
        }

        result.jointAngles = angles;
        return result;
    }

}
