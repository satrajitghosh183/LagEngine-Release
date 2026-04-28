// Cannon2D.cpp
#include "engine/objects/Cannon2D.hpp"
#include <cmath>
#include <iostream>

namespace engine::objects {

    constexpr float PI = 3.14159265358979323846f;

    Cannon2D::Cannon2D(const glm::vec2& pos)
        : position(pos) {
        active = true;
        visible = true;
    }

    void Cannon2D::rotate(float degrees) {
        angle += degrees;

        if (angle < -170.f) angle = -170.f;
        if (angle > -10.f) angle = -10.f;

        std::cout << "Cannon angle: " << angle << " degrees" << std::endl;
    }

    void Cannon2D::update(float) {
        // Cannon doesn't need physics updates
    }

    std::vector<scene::Object2D::Vertex2D> Cannon2D::getTriangleVertices() const {
        std::vector<Vertex2D> verts;

        float rad = angle * PI / 180.f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);

        // Helper to rotate a point around the cannon position
        auto rotatePoint = [&](float lx, float ly) -> glm::vec2 {
            return {position.x + lx * cosA - ly * sinA,
                    position.y + lx * sinA + ly * cosA};
        };

        // --- Base circle (dark gray) ---
        const int segments = 24;
        float baseRadius = 20.f;
        glm::vec3 baseColor = {0.275f, 0.275f, 0.294f};
        for (int i = 0; i < segments; ++i) {
            float a0 = (float)i / segments * 2.0f * PI;
            float a1 = (float)(i + 1) / segments * 2.0f * PI;
            verts.push_back({{position.x, position.y, 0.0f}, baseColor});
            verts.push_back({{position.x + baseRadius * std::cos(a0), position.y + baseRadius * std::sin(a0), 0.0f}, baseColor});
            verts.push_back({{position.x + baseRadius * std::cos(a1), position.y + baseRadius * std::sin(a1), 0.0f}, baseColor});
        }

        // --- Barrel (rectangle as two triangles) ---
        float barrelLength = 60.f;
        float barrelHalf = 6.f;
        glm::vec3 barrelColor = {0.157f, 0.157f, 0.176f};

        glm::vec2 bl = rotatePoint(0, -barrelHalf);
        glm::vec2 br = rotatePoint(barrelLength, -barrelHalf);
        glm::vec2 tr = rotatePoint(barrelLength, barrelHalf);
        glm::vec2 tl = rotatePoint(0, barrelHalf);

        verts.push_back({{bl.x, bl.y, 0.0f}, barrelColor});
        verts.push_back({{br.x, br.y, 0.0f}, barrelColor});
        verts.push_back({{tr.x, tr.y, 0.0f}, barrelColor});

        verts.push_back({{bl.x, bl.y, 0.0f}, barrelColor});
        verts.push_back({{tr.x, tr.y, 0.0f}, barrelColor});
        verts.push_back({{tl.x, tl.y, 0.0f}, barrelColor});

        // --- Muzzle (wider rectangle at barrel end) ---
        float muzzleWidth = 7.f;
        glm::vec3 muzzleColor = {0.392f, 0.392f, 0.412f};
        glm::vec2 ml = rotatePoint(barrelLength, -muzzleWidth);
        glm::vec2 mr = rotatePoint(barrelLength + 5.f, -muzzleWidth);
        glm::vec2 mtr = rotatePoint(barrelLength + 5.f, muzzleWidth);
        glm::vec2 mtl = rotatePoint(barrelLength, muzzleWidth);

        verts.push_back({{ml.x, ml.y, 0.0f}, muzzleColor});
        verts.push_back({{mr.x, mr.y, 0.0f}, muzzleColor});
        verts.push_back({{mtr.x, mtr.y, 0.0f}, muzzleColor});

        verts.push_back({{ml.x, ml.y, 0.0f}, muzzleColor});
        verts.push_back({{mtr.x, mtr.y, 0.0f}, muzzleColor});
        verts.push_back({{mtl.x, mtl.y, 0.0f}, muzzleColor});

        // --- Firing mechanism (small red circle behind barrel) ---
        float mechRadius = 5.f;
        glm::vec3 mechColor = {0.706f, 0.118f, 0.118f};
        glm::vec2 mechPos = {position.x - cosA * 10.f, position.y - sinA * 10.f};
        for (int i = 0; i < segments; ++i) {
            float a0 = (float)i / segments * 2.0f * PI;
            float a1 = (float)(i + 1) / segments * 2.0f * PI;
            verts.push_back({{mechPos.x, mechPos.y, 0.0f}, mechColor});
            verts.push_back({{mechPos.x + mechRadius * std::cos(a0), mechPos.y + mechRadius * std::sin(a0), 0.0f}, mechColor});
            verts.push_back({{mechPos.x + mechRadius * std::cos(a1), mechPos.y + mechRadius * std::sin(a1), 0.0f}, mechColor});
        }

        // --- Power bar background ---
        float barWidth = 50.f;
        float barHeight = 5.f;
        glm::vec3 barBg = {0.196f, 0.196f, 0.196f};
        float barX = position.x - 25.f;
        float barY = position.y + 30.f;

        verts.push_back({{barX,           barY,           0.0f}, barBg});
        verts.push_back({{barX + barWidth, barY,           0.0f}, barBg});
        verts.push_back({{barX + barWidth, barY + barHeight, 0.0f}, barBg});

        verts.push_back({{barX,           barY,           0.0f}, barBg});
        verts.push_back({{barX + barWidth, barY + barHeight, 0.0f}, barBg});
        verts.push_back({{barX,           barY + barHeight, 0.0f}, barBg});

        // --- Power bar fill ---
        float powerPercent = (power - 200.f) / 800.f;
        powerPercent = std::max(0.0f, std::min(1.0f, powerPercent));
        float fillWidth = barWidth * powerPercent;

        glm::vec3 powerColor;
        if (powerPercent < 0.3f)
            powerColor = {0.0f, 0.784f, 0.0f};
        else if (powerPercent < 0.7f)
            powerColor = {0.784f, 0.784f, 0.0f};
        else
            powerColor = {0.784f, 0.0f, 0.0f};

        verts.push_back({{barX,            barY,           0.0f}, powerColor});
        verts.push_back({{barX + fillWidth, barY,           0.0f}, powerColor});
        verts.push_back({{barX + fillWidth, barY + barHeight, 0.0f}, powerColor});

        verts.push_back({{barX,            barY,           0.0f}, powerColor});
        verts.push_back({{barX + fillWidth, barY + barHeight, 0.0f}, powerColor});
        verts.push_back({{barX,            barY + barHeight, 0.0f}, powerColor});

        return verts;
    }

    glm::vec2 Cannon2D::getFiringVelocity() const {
        float rad = angle * PI / 180.f;
        return {
            std::cos(rad) * power * 0.2f,
            std::sin(rad) * power * 0.2f
        };
    }

    glm::vec2 Cannon2D::getMuzzlePosition() const {
        float rad = angle * PI / 180.f;
        float barrelLength = 60.0f;
        return {
            position.x + std::cos(rad) * barrelLength,
            position.y + std::sin(rad) * barrelLength
        };
    }

    void Cannon2D::adjustPower(float amount) {
        power += amount;
        if (power < 200.0f) power = 200.0f;
        if (power > 1000.0f) power = 1000.0f;
        std::cout << "Cannon power adjusted to: " << power << std::endl;
    }

}
