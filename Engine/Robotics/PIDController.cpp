#include "PIDController.hpp"
#include <algorithm>

namespace GameEngine {

    PIDController::PIDController(float kp, float ki, float kd,
                                 float minOutput, float maxOutput)
        : Kp(kp), Ki(ki), Kd(kd),
          MinOutput(minOutput), MaxOutput(maxOutput) {}

    float PIDController::Update(float error, float dt) {
        if (dt <= 0.0f) return 0.0f;

        // Proportional term
        float pTerm = Kp * error;

        // Integral term with anti-windup clamping
        m_Integral += error * dt;
        float iTerm = Ki * m_Integral;

        // Clamp the integral contribution to prevent windup
        if (iTerm > MaxOutput) {
            iTerm = MaxOutput;
            m_Integral = MaxOutput / (Ki != 0.0f ? Ki : 1.0f);
        } else if (iTerm < MinOutput) {
            iTerm = MinOutput;
            m_Integral = MinOutput / (Ki != 0.0f ? Ki : 1.0f);
        }

        // Derivative term
        float dTerm = 0.0f;
        if (!m_FirstUpdate) {
            dTerm = Kd * (error - m_PreviousError) / dt;
        }
        m_FirstUpdate = false;
        m_PreviousError = error;

        // Compute and clamp output
        float output = pTerm + iTerm + dTerm;
        return std::clamp(output, MinOutput, MaxOutput);
    }

    void PIDController::Reset() {
        m_Integral = 0.0f;
        m_PreviousError = 0.0f;
        m_FirstUpdate = true;
    }

}
