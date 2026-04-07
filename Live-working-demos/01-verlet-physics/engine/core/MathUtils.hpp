// engine/core/MathUtils.hpp
#pragma once
#include <SFML/System/Vector2.hpp>
#include <cmath>
#include <algorithm>

// Namespace for math utilities inside the engine core.
namespace engine::core::math {

    // Constant for PI (used for angle conversion)
    constexpr float PI = 3.14159265358979323846f;

    // Converts degrees to radians.
    inline float toRadians(float degrees) {
        return degrees * (PI / 180.0f);
    }

    // Converts radians to degrees.
    inline float toDegrees(float radians) {
        return radians * (180.0f / PI);
    }

    // 2D Dot product
    inline float dot(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        return v1.x * v2.x + v1.y * v2.y;
    }

    // 2D Cross product (returns scalar value)
    inline float cross(const sf::Vector2f& v1, const sf::Vector2f& v2) {
        return v1.x * v2.y - v1.y * v2.x;
    }

    // Magnitude of a 2D vector
    inline float length(const sf::Vector2f& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    // Squared magnitude (no sqrt, for performance)
    inline float lengthSquared(const sf::Vector2f& v) {
        return v.x * v.x + v.y * v.y;
    }

    // Normalize a 2D vector
    inline sf::Vector2f normalize(const sf::Vector2f& v) {
        float len = length(v);
        if (len == 0.0f) return {0.0f, 0.0f};
        return {v.x / len, v.y / len};
    }

    // Linear interpolation between two scalars
    inline float lerp(float a, float b, float t) {
        return a + t * (b - a);
    }

    // Linear interpolation between two 2D vectors
    inline sf::Vector2f lerp(const sf::Vector2f& a, const sf::Vector2f& b, float t) {
        return { lerp(a.x, b.x, t), lerp(a.y, b.y, t) };
    }

    // Clamp scalar value between min and max
    inline float clamp(float value, float minVal, float maxVal) {
        return std::max(minVal, std::min(maxVal, value));
    }

    // Angle between two vectors (in radians)
    inline float angleBetween(const sf::Vector2f& a, const sf::Vector2f& b) {
        sf::Vector2f na = normalize(a);
        sf::Vector2f nb = normalize(b);
        float dp = dot(na, nb);
        dp = clamp(dp, -1.0f, 1.0f); // Avoid NaN from acos()
        return std::acos(dp);
    }

    // Reflect vector across a normal
    inline sf::Vector2f reflect(const sf::Vector2f& v, const sf::Vector2f& normal) {
        float dp = dot(v, normal);
        return { v.x - 2 * dp * normal.x, v.y - 2 * dp * normal.y };
    }

    // Project vector a onto vector b
    inline sf::Vector2f project(const sf::Vector2f& a, const sf::Vector2f& b) {
        sf::Vector2f nb = normalize(b);
        float projScale = dot(a, nb);
        return { nb.x * projScale, nb.y * projScale };
    }

    // Distance between two points
    inline float distance(const sf::Vector2f& a, const sf::Vector2f& b) {
        return length({b.x - a.x, b.y - a.y});
    }

    // Squared distance (faster, no sqrt)
    inline float distanceSquared(const sf::Vector2f& a, const sf::Vector2f& b) {
        sf::Vector2f diff = {b.x - a.x, b.y - a.y};
        return lengthSquared(diff);
    }

    // Returns angle (in degrees) relative to x-axis
    inline float angle(const sf::Vector2f& v) {
        return toDegrees(std::atan2(v.y, v.x));
    }

} // namespace engine::core::math
