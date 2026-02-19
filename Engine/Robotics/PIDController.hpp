#pragma once

namespace GameEngine {

    /**
     * @brief Simple PID controller with anti-windup clamping
     */
    class PIDController {
    public:
        PIDController() = default;
        PIDController(float kp, float ki, float kd,
                      float minOutput = -1e6f, float maxOutput = 1e6f);

        /**
         * @brief Compute control output given current error and timestep
         * @param error  The current error (setpoint - measured)
         * @param dt     Time step in seconds
         * @return Clamped control output
         */
        float Update(float error, float dt);

        /**
         * @brief Reset integral and previous-error state
         */
        void Reset();

    public:
        float Kp = 1.0f;
        float Ki = 0.0f;
        float Kd = 0.0f;
        float MinOutput = -1e6f;
        float MaxOutput =  1e6f;

    private:
        float m_Integral = 0.0f;
        float m_PreviousError = 0.0f;
        bool  m_FirstUpdate = true;
    };

}
