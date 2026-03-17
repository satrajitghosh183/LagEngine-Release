#pragma once
#include "sph.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct Slider {
    std::string label;
    float minVal, maxVal;
    float* value;
    float x, y, width;
    bool dragging = false;

    static constexpr float HEIGHT = 4.0f;
    static constexpr float KNOB_R = 7.0f;
    static constexpr float ROW_H  = 38.0f;

    float normalized() const { return (*value - minVal) / (maxVal - minVal); }
    void  setFromNorm(float t) { *value = minVal + std::clamp(t, 0.f, 1.f) * (maxVal - minVal); }
    float knobX() const { return x + normalized() * width; }
};

struct Preset  { std::string name; SPHParams params; };

class GUI {
public:
    GUI(float panelX, float panelWidth, const sf::Font& font);

    void addSlider(const std::string& label, float* value,
                   float minVal, float maxVal);
    void addPreset(const std::string& name, const SPHParams& params);
    void setScenes(const std::vector<Scene>& scenes);

    // Returns index of scene that was clicked (or -1 for none)
    int  handleEvent(const sf::Event& event, SPHSolver& solver);
    void draw(sf::RenderWindow& window, const SPHSolver& solver,
              float fps, bool paused, int activeScene);

    bool isOverPanel(float mx) const { return mx >= m_panelX; }

private:
    void drawSlider(sf::RenderWindow& window, Slider& s);
    void drawSection(sf::RenderWindow& window, const std::string& title,
                     float& y);
    void drawStat(sf::RenderWindow& window, const std::string& label,
                  const std::string& value, float& y);

    std::vector<Slider>       m_sliders;
    std::vector<Preset>       m_presets;
    std::vector<std::string>  m_sceneNames;
    int m_activePreset = 0;

    const sf::Font& m_font;
    float m_panelX, m_panelW, m_margin, m_sliderW;

    // Button Y positions (set during draw, used in event)
    float m_sceneBtnY  = 0;
    float m_sceneBtnH  = 22.f;
    float m_presetBtnY = 0;
    float m_presetBtnH = 20.f;
};
