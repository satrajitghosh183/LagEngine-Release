#include "gui.h"
#include <sstream>
#include <iomanip>
#include <cmath>

static const sf::Color kPanelBg(10, 13, 22, 245);
static const sf::Color kDivider(35, 80, 150, 50);
static const sf::Color kAccent(70, 155, 240);
static const sf::Color kAccentDim(45, 100, 180);
static const sf::Color kLabel(160, 175, 200);
static const sf::Color kValue(100, 190, 255);
static const sf::Color kMuted(100, 110, 130);
static const sf::Color kTrackBg(22, 30, 48);
static const sf::Color kTrackOutline(38, 55, 85);
static const sf::Color kTrackFill(35, 95, 175, 130);

GUI::GUI(float panelX, float panelWidth, const sf::Font& font)
    : m_font(font), m_panelX(panelX), m_panelW(panelWidth)
{
    m_margin  = 22.f;
    m_sliderW = m_panelW - m_margin * 2;
}

void GUI::addSlider(const std::string& label, float* value,
                    float minVal, float maxVal) {
    Slider s;
    s.label = label; s.minVal = minVal; s.maxVal = maxVal;
    s.value = value; s.x = m_panelX + m_margin; s.y = 0;
    s.width = m_sliderW;
    m_sliders.push_back(s);
}

void GUI::addPreset(const std::string& name, const SPHParams& params) {
    m_presets.push_back({name, params});
}

void GUI::setScenes(const std::vector<Scene>& scenes) {
    m_sceneNames.clear();
    for (const auto& s : scenes) m_sceneNames.push_back(s.name);
}

// ═══════════════════════════════════════════════════════════════
int GUI::handleEvent(const sf::Event& event, SPHSolver& solver) {
    int sceneClicked = -1;

    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        float mx = static_cast<float>(press->position.x);
        float my = static_cast<float>(press->position.y);

        // Slider knobs
        for (auto& s : m_sliders) {
            float ky = s.y + 14.f;
            float dx = mx - s.knobX(), dy = my - ky;
            if (dx * dx + dy * dy < 250.f) s.dragging = true;
        }

        // Scene buttons (F1–F4 also work, checked in main)
        if (!m_sceneNames.empty() && my >= m_sceneBtnY &&
            my <= m_sceneBtnY + m_sceneBtnH && mx >= m_panelX + m_margin) {
            float btnW = (m_sliderW - 3.f * (m_sceneNames.size() - 1))
                         / static_cast<float>(m_sceneNames.size());
            for (int i = 0; i < static_cast<int>(m_sceneNames.size()); ++i) {
                float bx = m_panelX + m_margin + i * (btnW + 3.f);
                if (mx >= bx && mx <= bx + btnW) sceneClicked = i;
            }
        }

        // Preset buttons (clickable)
        if (!m_presets.empty() && my >= m_presetBtnY &&
            my <= m_presetBtnY + m_presetBtnH && mx >= m_panelX + m_margin) {
            float btnW = (m_sliderW - 3.f * (m_presets.size() - 1))
                         / static_cast<float>(m_presets.size());
            for (int i = 0; i < static_cast<int>(m_presets.size()); ++i) {
                float bx = m_panelX + m_margin + i * (btnW + 3.f);
                if (mx >= bx && mx <= bx + btnW) {
                    solver.params() = m_presets[i].params;
                    solver.recalcKernels();
                    m_activePreset = i;
                }
            }
        }
    }
    if (event.is<sf::Event::MouseButtonReleased>())
        for (auto& s : m_sliders) s.dragging = false;

    if (const auto* move = event.getIf<sf::Event::MouseMoved>()) {
        float mx = static_cast<float>(move->position.x);
        for (auto& s : m_sliders)
            if (s.dragging) s.setFromNorm((mx - s.x) / s.width);
    }

    // Preset keys 1–4
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        int idx = -1;
        if (key->code == sf::Keyboard::Key::Num1) idx = 0;
        if (key->code == sf::Keyboard::Key::Num2) idx = 1;
        if (key->code == sf::Keyboard::Key::Num3) idx = 2;
        if (key->code == sf::Keyboard::Key::Num4) idx = 3;
        if (idx >= 0 && idx < static_cast<int>(m_presets.size())) {
            solver.params() = m_presets[idx].params;
            solver.recalcKernels();
            m_activePreset = idx;
        }
        // Scene keys F1–F4
        if (key->code == sf::Keyboard::Key::F1) sceneClicked = 0;
        if (key->code == sf::Keyboard::Key::F2) sceneClicked = 1;
        if (key->code == sf::Keyboard::Key::F3) sceneClicked = 2;
        if (key->code == sf::Keyboard::Key::F4) sceneClicked = 3;
    }
    return sceneClicked;
}

// ═══════════════════════════════════════════════════════════════
//  Drawing helpers
// ═══════════════════════════════════════════════════════════════

void GUI::drawSection(sf::RenderWindow& window, const std::string& title,
                      float& y) {
    y += 6.f;
    sf::Vertex div[] = {
        {{m_panelX + m_margin, y}, kDivider},
        {{m_panelX + m_panelW - m_margin, y}, kDivider}};
    window.draw(div, 2, sf::PrimitiveType::Lines);
    y += 10.f;
    sf::Text t(m_font, title, 11);
    t.setFillColor(kAccent);
    t.setStyle(sf::Text::Bold);
    t.setLetterSpacing(1.3f);
    t.setPosition({m_panelX + m_margin, y});
    window.draw(t);
    y += 20.f;
}

void GUI::drawStat(sf::RenderWindow& window, const std::string& label,
                   const std::string& value, float& y) {
    sf::Text lt(m_font, label, 10);
    lt.setFillColor(kMuted);
    lt.setPosition({m_panelX + m_margin, y});
    window.draw(lt);
    sf::Text vt(m_font, value, 10);
    vt.setFillColor(kLabel);
    auto vb = vt.getLocalBounds();
    vt.setPosition({m_panelX + m_panelW - m_margin - vb.size.x, y});
    window.draw(vt);
    y += 16.f;
}

// ═══════════════════════════════════════════════════════════════
//  Main draw
// ═══════════════════════════════════════════════════════════════

void GUI::draw(sf::RenderWindow& window, const SPHSolver& solver,
               float fps, bool paused, int activeScene) {
    float winH = static_cast<float>(window.getSize().y);

    // Panel bg
    sf::RectangleShape panel({m_panelW, winH});
    panel.setPosition({m_panelX, 0});
    panel.setFillColor(kPanelBg);
    window.draw(panel);

    sf::Vertex edge[] = {
        {{m_panelX, 0}, sf::Color(35, 80, 150, 80)},
        {{m_panelX, winH}, sf::Color(20, 50, 100, 30)}};
    window.draw(edge, 2, sf::PrimitiveType::Lines);

    float y = 14.f;

    // Title
    sf::Text title(m_font, "SPH Fluid Simulation", 14);
    title.setFillColor(sf::Color::White);
    title.setStyle(sf::Text::Bold);
    title.setPosition({m_panelX + m_margin, y});
    window.draw(title);
    y += 18.f;

    sf::Text sub(m_font, "Smoothed Particle Hydrodynamics", 9);
    sub.setFillColor(kMuted);
    sub.setPosition({m_panelX + m_margin, y});
    window.draw(sub);
    y += 12.f;

    // ── Scenes ──────────────────────────────────────────────────
    if (!m_sceneNames.empty()) {
        drawSection(window, "SCENES  (F1-F4)", y);
        m_sceneBtnY = y;

        float btnW = (m_sliderW - 3.f * (m_sceneNames.size() - 1))
                     / static_cast<float>(m_sceneNames.size());
        for (int i = 0; i < static_cast<int>(m_sceneNames.size()); ++i) {
            float bx = m_panelX + m_margin + i * (btnW + 3.f);
            bool active = (i == activeScene);
            sf::RectangleShape btn({btnW, m_sceneBtnH});
            btn.setPosition({bx, y});
            btn.setFillColor(active ? sf::Color(25, 75, 145, 200)
                                    : sf::Color(20, 28, 45, 180));
            btn.setOutlineColor(active ? kAccent : kTrackOutline);
            btn.setOutlineThickness(1.f);
            window.draw(btn);

            sf::Text bt(m_font, m_sceneNames[i], 9);
            bt.setFillColor(active ? sf::Color::White : kLabel);
            auto bb = bt.getLocalBounds();
            bt.setPosition({bx + (btnW - bb.size.x) / 2, y + 4.f});
            window.draw(bt);
        }
        y += m_sceneBtnH + 4.f;
    }

    // ── Presets ─────────────────────────────────────────────────
    drawSection(window, "FLUID TYPE  (1-4)", y);
    m_presetBtnY = y;
    {
        float btnW = (m_sliderW - 3.f * (m_presets.size() - 1))
                     / static_cast<float>(m_presets.size());
        for (int i = 0; i < static_cast<int>(m_presets.size()); ++i) {
            float bx = m_panelX + m_margin + i * (btnW + 3.f);
            bool active = (i == m_activePreset);
            sf::RectangleShape btn({btnW, 20.f});
            btn.setPosition({bx, y});
            btn.setFillColor(active ? sf::Color(35, 95, 175, 200)
                                    : sf::Color(25, 35, 55, 180));
            btn.setOutlineColor(active ? kAccent : kTrackOutline);
            btn.setOutlineThickness(1.f);
            window.draw(btn);
            sf::Text bt(m_font, m_presets[i].name, 9);
            bt.setFillColor(active ? sf::Color::White : kLabel);
            auto bb = bt.getLocalBounds();
            bt.setPosition({bx + (btnW - bb.size.x) / 2, y + 3.f});
            window.draw(bt);
        }
        y += 26.f;
    }

    // ── Sliders ─────────────────────────────────────────────────
    drawSection(window, "PROPERTIES", y);
    for (auto& s : m_sliders) {
        s.y = y;
        drawSlider(window, s);
        y += Slider::ROW_H;
    }

    // ── Stats ───────────────────────────────────────────────────
    drawSection(window, "STATISTICS", y);
    std::ostringstream oss;
    oss << solver.count();
    drawStat(window, "Particles", oss.str(), y);
    oss.str(""); oss << static_cast<int>(fps);
    drawStat(window, "FPS", oss.str(), y);
    oss.str(""); oss << solver.params().substeps;
    drawStat(window, "Substeps", oss.str(), y);
    oss.str(""); oss << solver.obstacles().size();
    drawStat(window, "Obstacles", oss.str(), y);

    if (paused) {
        y += 2.f;
        sf::Text pt(m_font, "PAUSED", 11);
        pt.setFillColor(sf::Color(255, 110, 90));
        pt.setStyle(sf::Text::Bold);
        pt.setPosition({m_panelX + m_margin, y});
        window.draw(pt);
        y += 16.f;
    }

    // ── Controls ────────────────────────────────────────────────
    drawSection(window, "CONTROLS", y);
    const char* keys[]    = {"Click", "D", "R", "Space", "Esc"};
    const char* actions[] = {"Add fluid", "Dam break", "Reset", "Pause", "Quit"};
    for (int i = 0; i < 5; ++i) {
        sf::RectangleShape badge({32.f, 14.f});
        badge.setPosition({m_panelX + m_margin, y + 1.f});
        badge.setFillColor(sf::Color(25, 35, 55, 180));
        badge.setOutlineColor(kTrackOutline);
        badge.setOutlineThickness(1.f);
        window.draw(badge);

        sf::Text kt(m_font, keys[i], 8);
        kt.setFillColor(kValue);
        auto kb = kt.getLocalBounds();
        kt.setPosition({m_panelX + m_margin + (32.f - kb.size.x) / 2, y + 1.f});
        window.draw(kt);

        sf::Text at(m_font, actions[i], 9);
        at.setFillColor(kLabel);
        at.setPosition({m_panelX + m_margin + 40.f, y + 1.f});
        window.draw(at);
        y += 17.f;
    }
}

// ═══════════════════════════════════════════════════════════════
//  Slider
// ═══════════════════════════════════════════════════════════════

void GUI::drawSlider(sf::RenderWindow& window, Slider& s) {
    float trackY = s.y + 14.f;

    sf::Text label(m_font, s.label, 9);
    label.setFillColor(kLabel);
    label.setPosition({s.x, s.y});
    window.draw(label);

    std::ostringstream oss;
    if (std::abs(*s.value) >= 100.f) oss << static_cast<int>(*s.value);
    else if (std::abs(*s.value) >= 1.f) oss << std::fixed << std::setprecision(1) << *s.value;
    else oss << std::fixed << std::setprecision(4) << *s.value;

    sf::Text vt(m_font, oss.str(), 9);
    vt.setFillColor(kValue);
    auto vb = vt.getLocalBounds();
    vt.setPosition({s.x + s.width - vb.size.x, s.y});
    window.draw(vt);

    sf::RectangleShape trackBg({s.width, Slider::HEIGHT});
    trackBg.setPosition({s.x, trackY - Slider::HEIGHT / 2});
    trackBg.setFillColor(kTrackBg);
    trackBg.setOutlineColor(kTrackOutline);
    trackBg.setOutlineThickness(0.5f);
    window.draw(trackBg);

    float fillW = s.normalized() * s.width;
    sf::RectangleShape fill({fillW, Slider::HEIGHT});
    fill.setPosition({s.x, trackY - Slider::HEIGHT / 2});
    fill.setFillColor(kTrackFill);
    window.draw(fill);

    float kx = s.knobX();
    sf::Color kc = s.dragging ? sf::Color(120, 200, 255) : kAccentDim;

    sf::CircleShape glow(Slider::KNOB_R + 4.f, 16);
    glow.setOrigin({Slider::KNOB_R + 4, Slider::KNOB_R + 4});
    glow.setPosition({kx, trackY});
    glow.setFillColor(sf::Color(kc.r, kc.g, kc.b, s.dragging ? (uint8_t)50 : (uint8_t)20));
    window.draw(glow);

    sf::CircleShape knob(Slider::KNOB_R, 16);
    knob.setOrigin({Slider::KNOB_R, Slider::KNOB_R});
    knob.setPosition({kx, trackY});
    knob.setFillColor(kc);
    knob.setOutlineColor(sf::Color(180, 210, 255, 80));
    knob.setOutlineThickness(1.f);
    window.draw(knob);

    float sr = Slider::KNOB_R * 0.28f;
    sf::CircleShape spec(sr, 8);
    spec.setOrigin({sr, sr});
    spec.setPosition({kx - 1.5f, trackY - 1.5f});
    spec.setFillColor(sf::Color(255, 255, 255, 55));
    window.draw(spec);
}
