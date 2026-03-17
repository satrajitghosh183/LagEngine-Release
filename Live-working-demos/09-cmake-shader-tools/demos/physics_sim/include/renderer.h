#pragma once
#include "sph.h"
#include <SFML/Graphics.hpp>

class FluidRenderer {
public:
    FluidRenderer(unsigned int simW, unsigned int simH);

    void draw(sf::RenderWindow& window, const SPHSolver& solver);

private:
    void drawBackground(sf::RenderWindow& window);
    void drawContainer(sf::RenderWindow& window);
    void drawObstacles(sf::RenderWindow& window, const SPHSolver& solver);
    void drawWaterBody(sf::RenderWindow& window, const SPHSolver& solver);
    void drawSurfaceDetail(sf::RenderWindow& window, const SPHSolver& solver);
    void drawFoam(sf::RenderWindow& window, const SPHSolver& solver);

    void drawGlassLine(sf::RenderWindow& window, Vec2 a, Vec2 b,
                       float thickness, sf::Color core, sf::Color glow);

    unsigned int m_simW, m_simH;
    sf::RenderTexture m_bodyRT;
    sf::RenderTexture m_hlRT;
};
