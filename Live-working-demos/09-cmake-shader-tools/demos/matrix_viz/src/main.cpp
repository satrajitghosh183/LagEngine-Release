#include "transforms.h"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>

static constexpr unsigned int SIM_W   = 720;
static constexpr unsigned int PANEL_W = 280;
static constexpr unsigned int WIN_W   = SIM_W + PANEL_W;
static constexpr unsigned int WIN_H   = 720;

// ─── Slider (matching SPH GUI style) ──────────────────────────
struct Slider {
    std::string label;
    float minVal, maxVal, *value;
    float x, y, width;
    bool dragging = false;
    static constexpr float H = 4.f, KNOB = 7.f, ROW = 38.f;
    float norm() const { return (*value - minVal) / (maxVal - minVal); }
    void setNorm(float t) { *value = minVal + std::clamp(t, 0.f, 1.f) * (maxVal - minVal); }
    float knobX() const { return x + norm() * width; }
};

// ─── Colors ───────────────────────────────────────────────────
static const sf::Color kPanelBg(10, 13, 22, 245);
static const sf::Color kAccent(70, 155, 240);
static const sf::Color kAccentDim(45, 100, 180);
static const sf::Color kLabel(160, 175, 200);
static const sf::Color kValue(100, 190, 255);
static const sf::Color kMuted(100, 110, 130);
static const sf::Color kTrackBg(22, 30, 48);
static const sf::Color kTrackOutline(38, 55, 85);
static const sf::Color kDivider(35, 80, 150, 50);

// ─── World-to-screen ─────────────────────────────────────────
static sf::Vector2f toScreen(Vec2 v, float cx, float cy, float zoom) {
    return {cx + v.x * zoom, cy - v.y * zoom};
}

// ─── Draw a thick line as a quad ──────────────────────────────
static void drawThickLine(sf::RenderWindow& win, sf::Vector2f a, sf::Vector2f b,
                           float thickness, sf::Color col) {
    sf::Vector2f d = b - a;
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 0.1f) return;
    sf::Vector2f n{-d.y / len * thickness / 2, d.x / len * thickness / 2};
    sf::VertexArray q(sf::PrimitiveType::Triangles, 6);
    q[0] = {a + n, col}; q[1] = {a - n, col}; q[2] = {b + n, col};
    q[3] = {a - n, col}; q[4] = {b - n, col}; q[5] = {b + n, col};
    win.draw(q);
}

// ─── Draw a shape ─────────────────────────────────────────────
static void drawShape(sf::RenderWindow& win, const Shape& shape, const Mat3& mat,
                      float cx, float cy, float zoom, sf::Color lineCol,
                      sf::Color vertCol, float thickness = 2.f) {
    for (const auto& [a, b] : shape.edges) {
        Vec2 pa = mat.apply(shape.vertices[a]);
        Vec2 pb = mat.apply(shape.vertices[b]);
        drawThickLine(win, toScreen(pa, cx, cy, zoom),
                      toScreen(pb, cx, cy, zoom), thickness, lineCol);
    }
    for (const auto& v : shape.vertices) {
        Vec2 pv = mat.apply(v);
        sf::Vector2f sv = toScreen(pv, cx, cy, zoom);
        sf::CircleShape dot(3.f, 12);
        dot.setOrigin({3.f, 3.f});
        dot.setPosition(sv);
        dot.setFillColor(vertCol);
        win.draw(dot);
    }
}

// ─── Draw basis vectors with arrowheads ───────────────────────
static void drawBasis(sf::RenderWindow& win, const Mat3& mat,
                      float cx, float cy, float zoom) {
    Vec2 origin = mat.apply({0, 0});
    Vec2 ex = mat.apply({1, 0});
    Vec2 ey = mat.apply({0, 1});

    auto drawArrow = [&](Vec2 from, Vec2 to, sf::Color col) {
        sf::Vector2f sf_ = toScreen(from, cx, cy, zoom);
        sf::Vector2f st = toScreen(to, cx, cy, zoom);
        drawThickLine(win, sf_, st, 3.f, col);
        // Arrowhead
        sf::CircleShape head(6.f, 3);
        head.setOrigin({6.f, 6.f});
        head.setPosition(st);
        head.setFillColor(col);
        float angle = std::atan2(st.y - sf_.y, st.x - sf_.x) * 180.f / 3.14159f;
        head.setRotation(sf::degrees(angle + 90.f));
        win.draw(head);
    };

    drawArrow(origin, ex, sf::Color(255, 85, 85, 220));  // x: red
    drawArrow(origin, ey, sf::Color(85, 255, 85, 220));  // y: green
}

// ─── Draw transformed grid (3Blue1Brown style) ────────────────
static void drawTransformedGrid(sf::RenderWindow& win, const Mat3& mat,
                                 float cx, float cy, float zoom) {
    float range = 6.f;

    // x = const lines (vertical in original) -> reddish
    for (float gx = -range; gx <= range; gx += 1.f) {
        Vec2 top = mat.apply({gx, -range});
        Vec2 bot = mat.apply({gx, range});
        sf::Vector2f st = toScreen(top, cx, cy, zoom);
        sf::Vector2f sb = toScreen(bot, cx, cy, zoom);
        sf::Color col = (gx == 0.f)
            ? sf::Color(255, 70, 70, 80)
            : sf::Color(255, 100, 100, 25);
        float thick = (gx == 0.f) ? 2.f : 1.f;
        drawThickLine(win, st, sb, thick, col);
    }

    // y = const lines (horizontal in original) -> greenish
    for (float gy = -range; gy <= range; gy += 1.f) {
        Vec2 left = mat.apply({-range, gy});
        Vec2 right = mat.apply({range, gy});
        sf::Vector2f sl = toScreen(left, cx, cy, zoom);
        sf::Vector2f sr = toScreen(right, cx, cy, zoom);
        sf::Color col = (gy == 0.f)
            ? sf::Color(70, 255, 70, 80)
            : sf::Color(100, 255, 100, 25);
        float thick = (gy == 0.f) ? 2.f : 1.f;
        drawThickLine(win, sl, sr, thick, col);
    }
}

// ─── Draw filled transformed unit parallelogram ───────────────
static void drawParallelogram(sf::RenderWindow& win, const Mat3& mat,
                               float cx, float cy, float zoom, float det) {
    Vec2 corners[] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    sf::Color fillCol = (det >= 0)
        ? sf::Color(40, 110, 220, 45)
        : sf::Color(220, 50, 50, 45);

    sf::Vector2f sc[4];
    for (int i = 0; i < 4; ++i)
        sc[i] = toScreen(mat.apply(corners[i]), cx, cy, zoom);

    sf::VertexArray fill(sf::PrimitiveType::Triangles, 6);
    fill[0] = {sc[0], fillCol}; fill[1] = {sc[1], fillCol}; fill[2] = {sc[2], fillCol};
    fill[3] = {sc[0], fillCol}; fill[4] = {sc[2], fillCol}; fill[5] = {sc[3], fillCol};
    win.draw(fill);

    // Bright outline of the parallelogram
    sf::Color outCol = (det >= 0)
        ? sf::Color(80, 160, 255, 120)
        : sf::Color(255, 100, 80, 120);
    for (int i = 0; i < 4; ++i)
        drawThickLine(win, sc[i], sc[(i + 1) % 4], 1.5f, outCol);
}

// ─── Draw panel section divider ───────────────────────────────
static void drawSection(sf::RenderWindow& win, const sf::Font& font,
                         const std::string& title, float panelX, float margin,
                         float panelW, float& y) {
    y += 6.f;
    sf::Vertex div[] = {
        {{panelX + margin, y}, kDivider},
        {{panelX + panelW - margin, y}, kDivider}};
    win.draw(div, 2, sf::PrimitiveType::Lines);
    y += 10.f;
    sf::Text t(font, title, 11);
    t.setFillColor(kAccent);
    t.setStyle(sf::Text::Bold);
    t.setLetterSpacing(1.3f);
    t.setPosition({panelX + margin, y});
    win.draw(t);
    y += 20.f;
}

// ─── Draw slider (matching SPH GUI) ──────────────────────────
static void drawSlider(sf::RenderWindow& win, const sf::Font& font, Slider& s) {
    float trackY = s.y + 14.f;

    sf::Text label(font, s.label, 9);
    label.setFillColor(kLabel);
    label.setPosition({s.x, s.y});
    win.draw(label);

    std::ostringstream oss;
    if (std::abs(*s.value) >= 100.f) oss << static_cast<int>(*s.value);
    else if (std::abs(*s.value) >= 1.f) oss << std::fixed << std::setprecision(1) << *s.value;
    else oss << std::fixed << std::setprecision(2) << *s.value;

    sf::Text vt(font, oss.str(), 9);
    vt.setFillColor(kValue);
    auto vb = vt.getLocalBounds();
    vt.setPosition({s.x + s.width - vb.size.x, s.y});
    win.draw(vt);

    sf::RectangleShape tbg({s.width, Slider::H});
    tbg.setPosition({s.x, trackY - Slider::H / 2});
    tbg.setFillColor(kTrackBg);
    tbg.setOutlineColor(kTrackOutline);
    tbg.setOutlineThickness(0.5f);
    win.draw(tbg);

    sf::RectangleShape tf({s.norm() * s.width, Slider::H});
    tf.setPosition({s.x, trackY - Slider::H / 2});
    tf.setFillColor(sf::Color(35, 95, 175, 130));
    win.draw(tf);

    sf::Color kc = s.dragging ? sf::Color(120, 200, 255) : kAccentDim;
    sf::CircleShape glow(Slider::KNOB + 4.f, 16);
    glow.setOrigin({Slider::KNOB + 4, Slider::KNOB + 4});
    glow.setPosition({s.knobX(), trackY});
    glow.setFillColor(sf::Color(kc.r, kc.g, kc.b, s.dragging ? (uint8_t)50 : (uint8_t)20));
    win.draw(glow);

    sf::CircleShape knob(Slider::KNOB, 16);
    knob.setOrigin({Slider::KNOB, Slider::KNOB});
    knob.setPosition({s.knobX(), trackY});
    knob.setFillColor(kc);
    knob.setOutlineColor(sf::Color(180, 210, 255, 80));
    knob.setOutlineThickness(1.f);
    win.draw(knob);
}

// ═══════════════════════════════════════════════════════════════

int main() {
    sf::RenderWindow window(sf::VideoMode({WIN_W, WIN_H}),
                            "Matrix Transform Visualizer");
    window.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = font.openFromFile("/System/Library/Fonts/SFNSMono.ttf")
               || font.openFromFile("/System/Library/Fonts/Menlo.ttc")
               || font.openFromFile("/System/Library/Fonts/Monaco.ttf")
               || font.openFromFile("/System/Library/Fonts/Helvetica.ttc");
    if (!hasFont) return 1;

    // Transform parameters
    float rotation = 0, scaleX = 1, scaleY = 1, shearX = 0, shearY = 0;
    float animate = 0;

    float panelX = static_cast<float>(SIM_W);
    float margin = 22.f;
    float sliderW = PANEL_W - margin * 2;

    std::vector<Slider> sliders = {
        {"Rotation",  -180, 180, &rotation, panelX + margin, 0, sliderW},
        {"Scale X",   0.1f, 3.0f, &scaleX, panelX + margin, 0, sliderW},
        {"Scale Y",   0.1f, 3.0f, &scaleY, panelX + margin, 0, sliderW},
        {"Shear X",   -2.0f, 2.0f, &shearX, panelX + margin, 0, sliderW},
        {"Shear Y",   -2.0f, 2.0f, &shearY, panelX + margin, 0, sliderW},
    };

    Shape shape = Shape::letterF();
    int shapeIdx = 3;
    const char* shapeNames[] = {"Square", "Arrow", "House", "Letter F"};
    float zoom = 55.f;
    float cx = SIM_W / 2.f, cy = WIN_H / 2.f;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) window.close();
                if (key->code == sf::Keyboard::Key::A)
                    animate = (animate > 0) ? 0 : 45.f;
                if (key->code == sf::Keyboard::Key::R)
                    { rotation=0; scaleX=1; scaleY=1; shearX=0; shearY=0; }
                if (key->code == sf::Keyboard::Key::Tab) {
                    shapeIdx = (shapeIdx + 1) % 4;
                    Shape shapes[] = {Shape::unitSquare(), Shape::arrow(),
                                      Shape::house(), Shape::letterF()};
                    shape = shapes[shapeIdx];
                }
            }
            if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                float mx = static_cast<float>(press->position.x);
                float my = static_cast<float>(press->position.y);
                for (auto& s : sliders) {
                    float ky = s.y + 14.f;
                    float dx = mx - s.knobX(), dy = my - ky;
                    if (dx*dx + dy*dy < 250.f) s.dragging = true;
                }
            }
            if (event->is<sf::Event::MouseButtonReleased>())
                for (auto& s : sliders) s.dragging = false;
            if (const auto* move = event->getIf<sf::Event::MouseMoved>()) {
                float mx = static_cast<float>(move->position.x);
                for (auto& s : sliders)
                    if (s.dragging) s.setNorm((mx - s.x) / s.width);
            }
        }

        if (animate > 0) {
            rotation += animate / 60.f;
            if (rotation > 180) rotation -= 360;
        }

        Mat3 mat = Mat3::scale(scaleX, scaleY)
                 * Mat3::shear(shearX, shearY)
                 * Mat3::rotation(rotation);
        float det = mat.determinant();

        // ─── Render sim area ────────────────────────────────────
        window.clear(sf::Color(8, 10, 18));

        // Background gradient
        {
            sf::VertexArray bg(sf::PrimitiveType::Triangles, 6);
            sf::Color top(5, 8, 18), bot(12, 18, 35);
            float w = static_cast<float>(SIM_W), h = static_cast<float>(WIN_H);
            bg[0]={{0,0},top}; bg[1]={{w,0},top}; bg[2]={{w,h},bot};
            bg[3]={{0,0},top}; bg[4]={{w,h},bot}; bg[5]={{0,h},bot};
            window.draw(bg);
        }

        // Original grid (very faint)
        sf::Color origGrid(255, 255, 255, 10);
        for (float gx = -12; gx <= 12; gx += 1.f) {
            sf::Vector2f t = toScreen({gx, -12}, cx, cy, zoom);
            sf::Vector2f b = toScreen({gx, 12}, cx, cy, zoom);
            sf::Vertex l[] = {{t, origGrid}, {b, origGrid}};
            window.draw(l, 2, sf::PrimitiveType::Lines);
        }
        for (float gy = -12; gy <= 12; gy += 1.f) {
            sf::Vector2f l = toScreen({-12, gy}, cx, cy, zoom);
            sf::Vector2f r = toScreen({12, gy}, cx, cy, zoom);
            sf::Vertex line[] = {{l, origGrid}, {r, origGrid}};
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        // Original axes (slightly brighter)
        sf::Color axCol(60, 60, 80);
        sf::Vertex xAx[] = {{toScreen({-12,0},cx,cy,zoom),axCol},{toScreen({12,0},cx,cy,zoom),axCol}};
        sf::Vertex yAx[] = {{toScreen({0,-12},cx,cy,zoom),axCol},{toScreen({0,12},cx,cy,zoom),axCol}};
        window.draw(xAx, 2, sf::PrimitiveType::Lines);
        window.draw(yAx, 2, sf::PrimitiveType::Lines);

        // Transformed grid (the star of the show)
        drawTransformedGrid(window, mat, cx, cy, zoom);

        // Filled parallelogram showing area
        drawParallelogram(window, mat, cx, cy, zoom, det);

        // Original shape (dim ghost)
        drawShape(window, shape, Mat3::identity(), cx, cy, zoom,
                  sf::Color(80, 80, 100, 60), sf::Color(100, 100, 120, 40), 1.5f);

        // Basis vectors
        drawBasis(window, mat, cx, cy, zoom);

        // Transformed shape (bright)
        drawShape(window, shape, mat, cx, cy, zoom,
                  sf::Color(80, 200, 255, 240), sf::Color(140, 230, 255), 2.5f);

        // Sim border
        sf::RectangleShape border({static_cast<float>(SIM_W) - 2, static_cast<float>(WIN_H) - 2});
        border.setPosition({1.f, 1.f});
        border.setFillColor(sf::Color::Transparent);
        border.setOutlineColor(sf::Color(40, 80, 140, 40));
        border.setOutlineThickness(1.f);
        window.draw(border);

        // ─── Panel ──────────────────────────────────────────────
        sf::RectangleShape panel({static_cast<float>(PANEL_W), static_cast<float>(WIN_H)});
        panel.setPosition({panelX, 0.f});
        panel.setFillColor(kPanelBg);
        window.draw(panel);

        sf::Vertex edge[] = {
            {{panelX, 0}, sf::Color(35, 80, 150, 80)},
            {{panelX, static_cast<float>(WIN_H)}, sf::Color(20, 50, 100, 30)}};
        window.draw(edge, 2, sf::PrimitiveType::Lines);

        float y = 14.f;

        // Title
        sf::Text title(font, "Matrix Transform Visualizer", 14);
        title.setFillColor(sf::Color::White);
        title.setStyle(sf::Text::Bold);
        title.setPosition({panelX + margin, y});
        window.draw(title);
        y += 18.f;

        sf::Text sub(font, "Interactive Linear Algebra", 9);
        sub.setFillColor(kMuted);
        sub.setPosition({panelX + margin, y});
        window.draw(sub);
        y += 12.f;

        // ── Shape selector ────────────────────────────────────
        drawSection(window, font, "SHAPE  (Tab)", panelX, margin, PANEL_W, y);
        {
            float btnW = (sliderW - 3.f * 3) / 4.f;
            for (int i = 0; i < 4; ++i) {
                float bx = panelX + margin + i * (btnW + 3.f);
                bool active = (i == shapeIdx);
                sf::RectangleShape btn({btnW, 20.f});
                btn.setPosition({bx, y});
                btn.setFillColor(active ? sf::Color(25, 75, 145, 200)
                                        : sf::Color(20, 28, 45, 180));
                btn.setOutlineColor(active ? kAccent : kTrackOutline);
                btn.setOutlineThickness(1.f);
                window.draw(btn);
                sf::Text bt(font, shapeNames[i], 8);
                bt.setFillColor(active ? sf::Color::White : kLabel);
                auto bb = bt.getLocalBounds();
                bt.setPosition({bx + (btnW - bb.size.x) / 2, y + 4.f});
                window.draw(bt);
            }
            y += 26.f;
        }

        // ── Sliders ───────────────────────────────────────────
        drawSection(window, font, "TRANSFORM", panelX, margin, PANEL_W, y);
        for (auto& s : sliders) {
            s.y = y;
            drawSlider(window, font, s);
            y += Slider::ROW;
        }

        // ── Matrix display ────────────────────────────────────
        drawSection(window, font, "MATRIX", panelX, margin, PANEL_W, y);

        // Brackets
        float matX = panelX + margin + 8.f;
        float matW = sliderW - 16.f;
        sf::Color brkCol(100, 180, 255, 120);

        sf::RectangleShape bL({2.f, 68.f});
        bL.setPosition({matX, y - 2.f}); bL.setFillColor(brkCol);
        window.draw(bL);
        sf::RectangleShape bLt({6.f, 2.f});
        bLt.setPosition({matX, y - 2.f}); bLt.setFillColor(brkCol);
        window.draw(bLt);
        sf::RectangleShape bLb({6.f, 2.f});
        bLb.setPosition({matX, y + 64.f}); bLb.setFillColor(brkCol);
        window.draw(bLb);

        float rBrX = matX + matW;
        sf::RectangleShape bR({2.f, 68.f});
        bR.setPosition({rBrX, y - 2.f}); bR.setFillColor(brkCol);
        window.draw(bR);
        sf::RectangleShape bRt({6.f, 2.f});
        bRt.setPosition({rBrX - 4.f, y - 2.f}); bRt.setFillColor(brkCol);
        window.draw(bRt);
        sf::RectangleShape bRb({6.f, 2.f});
        bRb.setPosition({rBrX - 4.f, y + 64.f}); bRb.setFillColor(brkCol);
        window.draw(bRb);

        for (int r = 0; r < 3; ++r) {
            for (int c = 0; c < 3; ++c) {
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << mat.m[r][c];
                sf::Color numCol = (r < 2 && c < 2)
                    ? sf::Color(200, 220, 255)
                    : sf::Color(100, 110, 130);
                sf::Text num(font, oss.str(), 12);
                num.setFillColor(numCol);
                num.setPosition({matX + 14.f + c * ((matW - 28.f) / 3.f), y + r * 22.f});
                window.draw(num);
            }
        }
        y += 80.f;

        // Determinant
        {
            std::ostringstream doss;
            doss << "det = " << std::fixed << std::setprecision(3) << det;
            sf::Text detText(font, doss.str(), 12);
            sf::Color detCol = (det > 0.99f && det < 1.01f) ? sf::Color(80, 220, 120)
                             : (det > 0) ? sf::Color(80, 220, 120)
                             : (det < 0) ? sf::Color(255, 100, 100)
                                         : sf::Color(255, 200, 80);
            detText.setFillColor(detCol);
            detText.setPosition({panelX + margin, y});
            window.draw(detText);

            std::string meaning = (det > 0.99f && det < 1.01f) ? "area preserved"
                                : (det > 0) ? "area scaled"
                                : (det < 0) ? "orientation flipped"
                                            : "collapsed!";
            sf::Text mt(font, meaning, 10);
            mt.setFillColor(kMuted);
            auto mb = mt.getLocalBounds();
            mt.setPosition({panelX + PANEL_W - margin - mb.size.x, y + 1.f});
            window.draw(mt);
            y += 22.f;

            // Area value
            std::ostringstream aoss;
            aoss << "area = " << std::fixed << std::setprecision(3) << std::abs(det) << "x";
            sf::Text areaText(font, aoss.str(), 10);
            areaText.setFillColor(kMuted);
            areaText.setPosition({panelX + margin, y});
            window.draw(areaText);
            y += 14.f;
        }

        // ── Controls ──────────────────────────────────────────
        drawSection(window, font, "CONTROLS", panelX, margin, PANEL_W, y);

        const char* keys[]    = {"Tab", "A", "R", "Esc"};
        const char* actions[] = {"Cycle shapes", "Auto-rotate", "Reset all", "Quit"};
        for (int i = 0; i < 4; ++i) {
            sf::RectangleShape badge({32.f, 14.f});
            badge.setPosition({panelX + margin, y + 1.f});
            badge.setFillColor(sf::Color(25, 35, 55, 180));
            badge.setOutlineColor(kTrackOutline);
            badge.setOutlineThickness(1.f);
            window.draw(badge);

            sf::Text kt(font, keys[i], 8);
            kt.setFillColor(kValue);
            auto kb = kt.getLocalBounds();
            kt.setPosition({panelX + margin + (32.f - kb.size.x) / 2, y + 1.f});
            window.draw(kt);

            sf::Text at(font, actions[i], 9);
            at.setFillColor(kLabel);
            at.setPosition({panelX + margin + 40.f, y + 1.f});
            window.draw(at);
            y += 17.f;
        }

        // ── Legend ────────────────────────────────────────────
        drawSection(window, font, "LEGEND", panelX, margin, PANEL_W, y);

        auto legendItem = [&](const char* label, sf::Color col) {
            sf::RectangleShape swatch({10.f, 10.f});
            swatch.setPosition({panelX + margin, y + 2.f});
            swatch.setFillColor(col);
            window.draw(swatch);
            sf::Text lt(font, label, 9);
            lt.setFillColor(kLabel);
            lt.setPosition({panelX + margin + 16.f, y});
            window.draw(lt);
            y += 15.f;
        };

        legendItem("Original shape",    sf::Color(80, 80, 100, 120));
        legendItem("Transformed shape", sf::Color(80, 200, 255));
        legendItem("x-grid lines",      sf::Color(255, 100, 100, 120));
        legendItem("y-grid lines",      sf::Color(100, 255, 100, 120));
        legendItem("Unit area",         sf::Color(40, 110, 220, 90));

        window.display();
    }
    return 0;
}
