#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {

    /**
     * @brief Denavit-Hartenberg parameter for a single joint
     *
     * Uses standard DH convention:
     * T = Rz(theta) * Tz(d) * Tx(a) * Rx(alpha)
     */
    struct DHParameter {
        float a = 0.0f;        // Link length (distance along x)
        float alpha = 0.0f;    // Link twist (rotation about x, radians)
        float d = 0.0f;        // Joint offset (distance along z)
        float theta = 0.0f;    // Joint angle (rotation about z, radians)

        enum class JointType { Revolute, Prismatic };
        JointType Type = JointType::Revolute;

        float MinLimit = -3.14159f;
        float MaxLimit =  3.14159f;

        /**
         * @brief Compute the 4x4 homogeneous transformation matrix
         *        using the standard DH convention
         */
        glm::mat4 GetTransformMatrix() const;
    };

}
