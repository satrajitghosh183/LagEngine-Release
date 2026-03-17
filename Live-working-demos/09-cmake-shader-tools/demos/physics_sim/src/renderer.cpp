#include "renderer.h"
#include <algorithm>
#include <cmath>

// ═══════════════════════════════════════════════════════════════
//  Per-fluid color palette
// ═══════════════════════════════════════════════════════════════

struct FluidPalette {
    sf::Color body;
    sf::Color highlight;
    sf::Color rim;
    sf::Color foam;
    float     alphaScale;
};

static FluidPalette getPalette(int scheme) {
    switch (scheme) {
    case 1:  return {{110, 70, 12}, {160, 110, 30}, {210, 165, 55}, {255, 230, 160}, 1.3f};
    case 2:  return {{45, 75, 105}, {80, 140, 190}, {130, 210, 245}, {225, 248, 255}, 0.55f};
    case 3:  return {{140, 28, 5}, {230, 95, 15}, {255, 150, 45}, {255, 215, 130}, 1.5f};
    default: return {{12, 50, 115}, {25, 90, 180}, {70, 165, 235}, {200, 230, 255}, 1.0f};
    }
}

// ═══════════════════════════════════════════════════════════════

FluidRenderer::FluidRenderer(unsigned int simW, unsigned int simH)
    : m_simW(simW), m_simH(simH),
      m_bodyRT(sf::Vector2u{simW, simH}),
      m_hlRT(sf::Vector2u{simW, simH})
{
}

// ═══════════════════════════════════════════════════════════════
//  Background
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::drawBackground(sf::RenderWindow& window) {
    sf::VertexArray bg(sf::PrimitiveType::Triangles, 6);
    sf::Color top(5, 8, 18), bot(12, 18, 35);
    float w = static_cast<float>(m_simW), h = static_cast<float>(m_simH);
    bg[0] = {{0,0}, top};  bg[1] = {{w,0}, top};  bg[2] = {{w,h}, bot};
    bg[3] = {{0,0}, top};  bg[4] = {{w,h}, bot};  bg[5] = {{0,h}, bot};
    window.draw(bg);
}

void FluidRenderer::drawContainer(sf::RenderWindow& window) {
    float w = static_cast<float>(m_simW), h = static_cast<float>(m_simH);

    sf::RectangleShape outerGlow({w - 2, h - 2});
    outerGlow.setPosition({1, 1});
    outerGlow.setFillColor(sf::Color::Transparent);
    outerGlow.setOutlineColor(sf::Color(30, 80, 160, 20));
    outerGlow.setOutlineThickness(4.f);
    window.draw(outerGlow);

    sf::RectangleShape border({w - 12, h - 12});
    border.setPosition({6, 6});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(50, 110, 190, 70));
    border.setOutlineThickness(1.5f);
    window.draw(border);

    sf::Vertex topE[] = {
        {{6, 6}, sf::Color(80, 160, 230, 40)},
        {{w - 6, 6}, sf::Color(40, 90, 160, 10)}};
    sf::Vertex leftE[] = {
        {{6, 6}, sf::Color(80, 160, 230, 40)},
        {{6, h - 6}, sf::Color(20, 60, 120, 5)}};
    window.draw(topE, 2, sf::PrimitiveType::Lines);
    window.draw(leftE, 2, sf::PrimitiveType::Lines);

    sf::Color tickCol(50, 110, 180, 30);
    for (float y = 50.f; y < h - 6; y += 50.f) {
        sf::Vertex tick[] = {{{6, y}, tickCol}, {{14.f, y}, tickCol}};
        window.draw(tick, 2, sf::PrimitiveType::Lines);
    }
}

// ═══════════════════════════════════════════════════════════════
//  Obstacles
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::drawGlassLine(sf::RenderWindow& window, Vec2 a, Vec2 b,
                                   float thickness, sf::Color core,
                                   sf::Color glow) {
    sf::Vector2f sa{a.x, a.y}, sb{b.x, b.y};
    sf::Vector2f d = sb - sa;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.1f) return;
    sf::Vector2f n{-d.y / len, d.x / len};

    float gw = thickness * 2.5f;
    sf::Vector2f gn = n * (gw / 2.f);
    sf::VertexArray glowQuad(sf::PrimitiveType::Triangles, 6);
    glowQuad[0] = {sa + gn, glow}; glowQuad[1] = {sa - gn, glow};
    glowQuad[2] = {sb + gn, glow}; glowQuad[3] = {sa - gn, glow};
    glowQuad[4] = {sb - gn, glow}; glowQuad[5] = {sb + gn, glow};
    window.draw(glowQuad);

    sf::Vector2f cn = n * (thickness / 2.f);
    sf::VertexArray coreQuad(sf::PrimitiveType::Triangles, 6);
    coreQuad[0] = {sa + cn, core}; coreQuad[1] = {sa - cn, core};
    coreQuad[2] = {sb + cn, core}; coreQuad[3] = {sa - cn, core};
    coreQuad[4] = {sb - cn, core}; coreQuad[5] = {sb + cn, core};
    window.draw(coreQuad);

    sf::Vector2f hn = n * (thickness * 0.15f);
    sf::Vector2f offset = n * (thickness * 0.25f);
    sf::Color spec(200, 230, 255, 50);
    sf::VertexArray specLine(sf::PrimitiveType::Triangles, 6);
    specLine[0] = {sa + offset + hn, spec};
    specLine[1] = {sa + offset - hn, spec};
    specLine[2] = {sb + offset + hn, spec};
    specLine[3] = {sa + offset - hn, spec};
    specLine[4] = {sb + offset - hn, spec};
    specLine[5] = {sb + offset + hn, spec};
    window.draw(specLine);

    for (auto& pt : {sa, sb}) {
        sf::CircleShape cap(thickness / 2.f, 10);
        cap.setOrigin({thickness / 2.f, thickness / 2.f});
        cap.setPosition(pt);
        cap.setFillColor(core);
        window.draw(cap);
    }
}

void FluidRenderer::drawObstacles(sf::RenderWindow& window,
                                   const SPHSolver& solver) {
    sf::Color core(80, 150, 210, 140);
    sf::Color glow(40, 100, 180, 25);
    for (const auto& seg : solver.obstacles())
        drawGlassLine(window, seg.a, seg.b, 5.f, core, glow);
}

// ═══════════════════════════════════════════════════════════════
//  Fluid body — two-pass CircleShape blob rendering
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::drawWaterBody(sf::RenderWindow& window,
                                   const SPHSolver& solver) {
    const auto& particles = solver.particles();
    if (particles.empty()) return;

    float h    = solver.params().smoothingRadius;
    float rho0 = std::max(solver.params().restDensity, 1.f);
    FluidPalette pal = getPalette(solver.params().colorScheme);

    m_bodyRT.clear(sf::Color::Transparent);

    // Outer pass: large, low-alpha circles — accumulates to smooth blob
    float outerR = h * 2.5f;
    for (const auto& p : particles) {
        float dr = std::clamp(p.density / rho0, 0.f, 1.5f);
        uint8_t a = static_cast<uint8_t>(std::clamp(
            (14.f + dr * 10.f) * pal.alphaScale, 1.f, 60.f));
        sf::CircleShape c(outerR, 20);
        c.setOrigin({outerR, outerR});
        c.setPosition({p.pos.x, p.pos.y});
        c.setFillColor(sf::Color(pal.body.r, pal.body.g, pal.body.b, a));
        m_bodyRT.draw(c);
    }

    // Inner pass: smaller, higher alpha — solid opaque core
    float innerR = h * 0.9f;
    for (const auto& p : particles) {
        float dr = std::clamp(p.density / rho0, 0.f, 1.5f);
        uint8_t a = static_cast<uint8_t>(std::clamp(
            (35.f + dr * 55.f) * pal.alphaScale, 1.f, 200.f));
        sf::CircleShape c(innerR, 14);
        c.setOrigin({innerR, innerR});
        c.setPosition({p.pos.x, p.pos.y});
        c.setFillColor(sf::Color(pal.body.r, pal.body.g, pal.body.b, a));
        m_bodyRT.draw(c);
    }

    m_bodyRT.display();
    sf::Sprite bodySprite(m_bodyRT.getTexture());
    window.draw(bodySprite);

    // Highlight / caustics — additive
    m_hlRT.clear(sf::Color::Transparent);
    float hlR = h * 0.7f;
    for (const auto& p : particles) {
        float dr = std::clamp(p.density / rho0, 0.f, 1.5f);
        float brightness = 1.0f - std::abs(dr - 0.7f) * 1.2f;
        brightness = std::clamp(brightness, 0.1f, 1.0f);
        uint8_t a = static_cast<uint8_t>(22 * brightness * pal.alphaScale);
        sf::CircleShape c(hlR, 10);
        c.setOrigin({hlR, hlR});
        c.setPosition({p.pos.x, p.pos.y});
        c.setFillColor(sf::Color(pal.highlight.r, pal.highlight.g,
                                  pal.highlight.b, a));
        m_hlRT.draw(c, sf::BlendAdd);
    }
    m_hlRT.display();
    sf::Sprite hlSprite(m_hlRT.getTexture());
    window.draw(hlSprite, sf::BlendAdd);
}

// ═══════════════════════════════════════════════════════════════
//  Surface detail — bright rims on edge particles
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::drawSurfaceDetail(sf::RenderWindow& window,
                                       const SPHSolver& solver) {
    const auto& particles = solver.particles();
    float h = solver.params().smoothingRadius;
    float rho0 = std::max(solver.params().restDensity, 1.f);
    FluidPalette pal = getPalette(solver.params().colorScheme);
    float surfR = h * 0.35f;

    for (const auto& p : particles) {
        float dr = p.density / rho0;
        if (dr > 0.65f) continue;

        float surfAlpha = std::clamp(1.0f - dr / 0.65f, 0.f, 1.f);
        uint8_t a = static_cast<uint8_t>(70 * surfAlpha);

        sf::CircleShape rim(surfR, 10);
        rim.setOrigin({surfR, surfR});
        rim.setPosition({p.pos.x, p.pos.y});
        rim.setFillColor(sf::Color::Transparent);
        rim.setOutlineColor(sf::Color(pal.rim.r, pal.rim.g, pal.rim.b, a));
        rim.setOutlineThickness(1.5f);
        window.draw(rim);

        float specR = surfR * 0.25f;
        sf::CircleShape spec(specR, 8);
        spec.setOrigin({specR, specR});
        spec.setPosition({p.pos.x + surfR * 0.3f, p.pos.y - surfR * 0.4f});
        spec.setFillColor(sf::Color(pal.foam.r, pal.foam.g, pal.foam.b,
                          static_cast<uint8_t>(55 * surfAlpha)));
        window.draw(spec);
    }
}

// ═══════════════════════════════════════════════════════════════
//  Foam — splashes on fast surface particles
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::drawFoam(sf::RenderWindow& window,
                              const SPHSolver& solver) {
    const auto& particles = solver.particles();
    float h = solver.params().smoothingRadius;
    float rho0 = std::max(solver.params().restDensity, 1.f);
    float maxSpeed = std::max(solver.maxSpeed(), 1.f);
    FluidPalette pal = getPalette(solver.params().colorScheme);

    for (const auto& p : particles) {
        float sr = p.vel.length() / maxSpeed;
        float dr = p.density / rho0;
        if (sr < 0.5f || dr > 0.8f) continue;

        float fa = std::clamp((sr - 0.5f) * 2.f * (1.f - dr), 0.f, 1.f);
        uint8_t a = static_cast<uint8_t>(80 * fa);
        float r = h * (0.25f + 0.15f * fa);

        sf::CircleShape foam(r, 8);
        foam.setOrigin({r, r});
        foam.setPosition({p.pos.x, p.pos.y});
        foam.setFillColor(sf::Color(pal.foam.r, pal.foam.g, pal.foam.b, a));
        window.draw(foam);
    }
}

// ═══════════════════════════════════════════════════════════════
//  Composite
// ═══════════════════════════════════════════════════════════════

void FluidRenderer::draw(sf::RenderWindow& window, const SPHSolver& solver) {
    drawBackground(window);
    drawContainer(window);
    drawObstacles(window, solver);

    if (solver.particles().empty()) return;

    drawWaterBody(window, solver);
    drawSurfaceDetail(window, solver);
    drawFoam(window, solver);

    drawObstacles(window, solver);
}
