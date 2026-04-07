#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>

namespace GameEngine {
namespace Math {

    /**
     * @brief Mathematical constants
     */
    constexpr float PI = glm::pi<float>();
    constexpr float TWO_PI = glm::two_pi<float>();
    constexpr float HALF_PI = glm::half_pi<float>();
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;
    constexpr float EPSILON = 0.0001f;

    /**
     * @brief Convert degrees to radians
     */
    inline float ToRadians(float degrees) {
        return degrees * DEG_TO_RAD;
    }

    /**
     * @brief Convert radians to degrees
     */
    inline float ToDegrees(float radians) {
        return radians * RAD_TO_DEG;
    }

    /**
     * @brief Linear interpolation
     */
    template<typename T>
    inline T Lerp(const T& a, const T& b, float t) {
        return a + (b - a) * t;
    }

    /**
     * @brief Inverse lerp (find t given a, b, and value)
     */
    inline float InverseLerp(float a, float b, float value) {
        if (abs(b - a) < EPSILON) return 0.0f;
        return (value - a) / (b - a);
    }

    /**
     * @brief Smooth step interpolation (cubic)
     */
    inline float SmoothStep(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    /**
     * @brief Smoother step interpolation (quintic)
     */
    inline float SmootherStep(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    /**
     * @brief Spherical linear interpolation (for rotations)
     */
    inline glm::quat Slerp(const glm::quat& a, const glm::quat& b, float t) {
        return glm::slerp(a, b, t);
    }

    /**
     * @brief Map value from one range to another
     */
    inline float Remap(float value, float inMin, float inMax, float outMin, float outMax) {
        float t = InverseLerp(inMin, inMax, value);
        return Lerp(outMin, outMax, t);
    }

    /**
     * @brief Clamp value between min and max
     */
    template<typename T>
    inline T Clamp(const T& value, const T& min, const T& max) {
        return glm::clamp(value, min, max);
    }

    /**
     * @brief Clamp value between 0 and 1
     */
    template<typename T>
    inline T Clamp01(const T& value) {
        return glm::clamp(value, T(0), T(1));
    }

    /**
     * @brief Wrap value to range [min, max)
     */
    inline float Wrap(float value, float min, float max) {
        float range = max - min;
        return value - range * floor((value - min) / range);
    }

    /**
     * @brief Wrap angle to [-PI, PI]
     */
    inline float WrapAngle(float angle) {
        return Wrap(angle, -PI, PI);
    }

    /**
     * @brief Sign function (-1, 0, or 1)
     */
    template<typename T>
    inline T Sign(const T& value) {
        return (value > T(0)) ? T(1) : ((value < T(0)) ? T(-1) : T(0));
    }

    /**
     * @brief Approximately equal (with epsilon)
     */
    inline bool Approximately(float a, float b, float epsilon = EPSILON) {
        return abs(a - b) < epsilon;
    }

    /**
     * @brief Move towards target with max delta
     */
    template<typename T>
    inline T MoveTowards(const T& current, const T& target, float maxDelta) {
        T delta = target - current;
        float distance = glm::length(delta);
        
        if (distance <= maxDelta || distance < EPSILON) {
            return target;
        }
        
        return current + delta / distance * maxDelta;
    }

    /**
     * @brief Smooth damp (spring-like motion)
     */
    inline float SmoothDamp(float current, float target, float& velocity, 
                           float smoothTime, float maxSpeed, float deltaTime) {
        smoothTime = glm::max(0.0001f, smoothTime);
        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
        
        float change = current - target;
        float originalTo = target;
        
        float maxChange = maxSpeed * smoothTime;
        change = glm::clamp(change, -maxChange, maxChange);
        target = current - change;
        
        float temp = (velocity + omega * change) * deltaTime;
        velocity = (velocity - omega * temp) * exp;
        
        float output = target + (change + temp) * exp;
        
        if ((originalTo - current > 0.0f) == (output > originalTo)) {
            output = originalTo;
            velocity = (output - originalTo) / deltaTime;
        }
        
        return output;
    }

    /**
     * @brief Bounce interpolation
     */
    inline float Bounce(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }

    /**
     * @brief Exponential decay
     */
    inline float ExponentialDecay(float a, float decay, float deltaTime) {
        return a * exp(-decay * deltaTime);
    }

    /**
     * @brief Calculate barycentric coordinates
     */
    inline glm::vec3 Barycentric(const glm::vec2& p, const glm::vec2& a, 
                                 const glm::vec2& b, const glm::vec2& c) {
        glm::vec2 v0 = b - a;
        glm::vec2 v1 = c - a;
        glm::vec2 v2 = p - a;
        
        float d00 = glm::dot(v0, v0);
        float d01 = glm::dot(v0, v1);
        float d11 = glm::dot(v1, v1);
        float d20 = glm::dot(v2, v0);
        float d21 = glm::dot(v2, v1);
        
        float denom = d00 * d11 - d01 * d01;
        if (abs(denom) < EPSILON) return glm::vec3(0);
        
        float v = (d11 * d20 - d01 * d21) / denom;
        float w = (d00 * d21 - d01 * d20) / denom;
        float u = 1.0f - v - w;
        
        return glm::vec3(u, v, w);
    }

}}