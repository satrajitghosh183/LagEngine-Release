#pragma once
#include <cmath>
#include <array>
#include <vector>

// ═══════════════════════════════════════════════════════════════
//  2D Transform Math — Educational Linear Algebra
// ═══════════════════════════════════════════════════════════════

struct Vec2 {
    float x = 0, y = 0;
    Vec2() = default;
    Vec2(float x, float y) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(float s) const { return {x*s, y*s}; }
};

// 3x3 matrix for 2D homogeneous coordinates
struct Mat3 {
    // Row-major: m[row][col]
    float m[3][3] = {{1,0,0},{0,1,0},{0,0,1}};

    static Mat3 identity() { return {}; }

    static Mat3 rotation(float angleDeg) {
        float r = angleDeg * 3.14159265f / 180.0f;
        float c = std::cos(r), s = std::sin(r);
        Mat3 mat;
        mat.m[0][0] = c;  mat.m[0][1] = -s;
        mat.m[1][0] = s;  mat.m[1][1] = c;
        return mat;
    }

    static Mat3 scale(float sx, float sy) {
        Mat3 mat;
        mat.m[0][0] = sx;
        mat.m[1][1] = sy;
        return mat;
    }

    static Mat3 shear(float shx, float shy) {
        Mat3 mat;
        mat.m[0][1] = shx;
        mat.m[1][0] = shy;
        return mat;
    }

    static Mat3 translation(float tx, float ty) {
        Mat3 mat;
        mat.m[0][2] = tx;
        mat.m[1][2] = ty;
        return mat;
    }

    Mat3 operator*(const Mat3& b) const {
        Mat3 r;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                r.m[i][j] = 0;
                for (int k = 0; k < 3; ++k)
                    r.m[i][j] += m[i][k] * b.m[k][j];
            }
        return r;
    }

    Vec2 apply(const Vec2& v) const {
        float rx = m[0][0]*v.x + m[0][1]*v.y + m[0][2];
        float ry = m[1][0]*v.x + m[1][1]*v.y + m[1][2];
        return {rx, ry};
    }

    float determinant() const {
        return m[0][0] * (m[1][1]*m[2][2] - m[1][2]*m[2][1])
             - m[0][1] * (m[1][0]*m[2][2] - m[1][2]*m[2][0])
             + m[0][2] * (m[1][0]*m[2][1] - m[1][1]*m[2][0]);
    }
};

// Predefined shapes as vertex lists
struct Shape {
    std::vector<Vec2> vertices;
    std::vector<std::pair<int,int>> edges; // index pairs

    // Unit square
    static Shape unitSquare() {
        return {
            {{-1,-1},{1,-1},{1,1},{-1,1}},
            {{0,1},{1,2},{2,3},{3,0}}
        };
    }

    // Arrow pointing right
    static Shape arrow() {
        return {
            {{-1.0f, -0.3f}, {0.3f, -0.3f}, {0.3f, -0.7f}, {1.0f, 0.0f},
             {0.3f, 0.7f}, {0.3f, 0.3f}, {-1.0f, 0.3f}},
            {{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,0}}
        };
    }

    // House shape
    static Shape house() {
        return {
            {{-0.8f, -1.0f}, {0.8f, -1.0f}, {0.8f, 0.2f},
             {0.0f, 1.0f}, {-0.8f, 0.2f}},
            {{0,1},{1,2},{2,3},{3,4},{4,0}}
        };
    }

    // F shape (asymmetric — shows orientation clearly)
    static Shape letterF() {
        return {
            {{-0.5f, -1.0f}, {0.0f, -1.0f}, {0.0f, -0.1f}, {0.5f, -0.1f},
             {0.5f, 0.1f}, {0.0f, 0.1f}, {0.0f, 0.5f}, {0.7f, 0.5f},
             {0.7f, 0.7f}, {0.0f, 0.7f}, {0.0f, 1.0f}, {-0.5f, 1.0f}},
            {{0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,8},{8,9},{9,10},{10,11},{11,0}}
        };
    }
};
