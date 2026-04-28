// engine/core/MathUtils.hpp
#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

namespace engine::core::math {

    constexpr float PI = 3.14159265358979323846f;

    inline float toRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    inline float toDegrees(float radians) {
        return radians * (180.0f / PI);
    }

    // 2D Dot product
    inline float dot(const glm::vec2& v1, const glm::vec2& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    // 2D Cross product (returns scalar value)
    inline float cross(const glm::vec2& v1, const glm::vec2& v2) {
        return v1.x * v2.y - v1.y * v2.x;
    }

    // Magnitude of a 2D vector
    inline float length(const glm::vec2& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    // Squared magnitude (no sqrt, for performance)
    inline float lengthSquared(const glm::vec2& v) {
        return v.x * v.x + v.y * v.y;
    }

    // Normalize a 2D vector
    inline glm::vec2 normalize(const glm::vec2& v) {
        float len = length(v);
        if (len == 0.0f) return {0.0f, 0.0f};
        return {v.x / len, v.y / len};
    }

    // Linear interpolation between two scalars
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Linear interpolation between two 2D vectors
    inline glm::vec2 lerp(const glm::vec2& a, const glm::vec2& b, float t) {
        return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
    }

    // Clamp scalar value between min and max
    inline float clamp(float value, float minVal, float maxVal) {
        return std::max(minVal, std::min(maxVal, value));
    }

    // Angle between two vectors (in radians)
    inline float angleBetween(const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 na = normalize(a);
        glm::vec2 nb = normalize(b);
        float dp = dot(na, nb);
        dp = clamp(dp, -1.0f, 1.0f);
        return std::acos(dp);
    }

    // Reflect vector across a normal
    inline glm::vec2 reflect(const glm::vec2& v, const glm::vec2& normal) {
        float dp = dot(v, normal);
        return { v.x - 2 * dp * normal.x, v.y - 2 * dp * normal.y };
    }

    // Project vector a onto vector b
    inline glm::vec2 project(const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 nb = normalize(b);
        float projScale = dot(a, nb);
        return { nb.x * projScale, nb.y * projScale };
    }

    // Distance between two points
    inline float distance(const glm::vec2& a, const glm::vec2& b) {
        return length({b.x - a.x, b.y - a.y});
    }

    // Squared distance (faster, no sqrt)
    inline float distanceSquared(const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 diff = {b.x - a.x, b.y - a.y};
        return lengthSquared(diff);
    }

    // Returns angle (in degrees) relative to x-axis
    inline float angle(const glm::vec2& v) {
        return toDegrees(std::atan2(v.y, v.x));
    }

} // namespace engine::core::math
