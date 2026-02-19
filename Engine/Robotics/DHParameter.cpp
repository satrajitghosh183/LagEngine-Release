#include "DHParameter.hpp"
#include <cmath>

namespace GameEngine {

    glm::mat4 DHParameter::GetTransformMatrix() const {
        // Standard DH convention: T = Rz(theta) * Tz(d) * Tx(a) * Rx(alpha)
        float ct = std::cos(theta);
        float st = std::sin(theta);
        float ca = std::cos(alpha);
        float sa = std::sin(alpha);

        // Construct the combined matrix directly for efficiency
        // Column-major layout (GLM convention)
        glm::mat4 T(1.0f);

        T[0][0] = ct;
        T[0][1] = st;
        T[0][2] = 0.0f;
        T[0][3] = 0.0f;

        T[1][0] = -st * ca;
        T[1][1] =  ct * ca;
        T[1][2] =  sa;
        T[1][3] =  0.0f;

        T[2][0] =  st * sa;
        T[2][1] = -ct * sa;
        T[2][2] =  ca;
        T[2][3] =  0.0f;

        T[3][0] =  a * ct;
        T[3][1] =  a * st;
        T[3][2] =  d;
        T[3][3] =  1.0f;

        return T;
    }

}
