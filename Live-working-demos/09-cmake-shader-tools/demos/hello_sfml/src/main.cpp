#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>

struct Ball {
    float x, y, vx, vy, radius;
    sf::Color color;
};

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Hello Graphics");
    window.setFramerateLimit(60);

    // Colorful bouncing balls
    std::vector<Ball> balls;
    std::srand(42);
    sf::Color colors[] = {
        {255, 80, 80}, {80, 255, 120}, {80, 120, 255},
        {255, 200, 50}, {255, 80, 200}, {80, 230, 230},
    };
    for (int i = 0; i < 14; ++i) {
        Ball b;
        b.x = 100.f + std::rand() % 600;
        b.y = 100.f + std::rand() % 400;
        b.vx = (std::rand() % 200 - 100) / 30.f;
        b.vy = (std::rand() % 200 - 100) / 30.f;
        b.radius = 14.f + std::rand() % 22;
        b.color = colors[i % 6];
        balls.push_back(b);
    }

    // Trail render texture
    sf::RenderTexture trailRT(sf::Vector2u{800, 600});
    trailRT.clear(sf::Color::Transparent);

    sf::Font font;
    bool hasFont = font.openFromFile("/System/Library/Fonts/SFNSMono.ttf")
               || font.openFromFile("/System/Library/Fonts/Menlo.ttc")
               || font.openFromFile("/System/Library/Fonts/Monaco.ttf")
               || font.openFromFile("/System/Library/Fonts/Helvetica.ttc");

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>())
                if (key->code == sf::Keyboard::Key::Escape) window.close();
        }

        for (auto& b : balls) {
            b.x += b.vx;
            b.y += b.vy;
            if (b.x - b.radius < 0 || b.x + b.radius > 800) b.vx = -b.vx;
            if (b.y - b.radius < 0 || b.y + b.radius > 600) b.vy = -b.vy;
            b.x = std::clamp(b.x, b.radius, 800.f - b.radius);
            b.y = std::clamp(b.y, b.radius, 600.f - b.radius);
        }

        // Fade trails
        sf::RectangleShape fade({800, 600});
        fade.setFillColor(sf::Color(0, 0, 0, 12));
        trailRT.draw(fade);
        for (const auto& b : balls) {
            sf::CircleShape c(b.radius * 0.5f, 16);
            c.setOrigin({b.radius * 0.5f, b.radius * 0.5f});
            c.setPosition({b.x, b.y});
            c.setFillColor(sf::Color(b.color.r, b.color.g, b.color.b, 50));
            trailRT.draw(c);
        }
        trailRT.display();

        // Draw
        window.clear(sf::Color(8, 10, 22));

        sf::Sprite trailSprite(trailRT.getTexture());
        window.draw(trailSprite);

        for (const auto& b : balls) {
            sf::CircleShape glow(b.radius + 10, 24);
            glow.setOrigin({b.radius + 10, b.radius + 10});
            glow.setPosition({b.x, b.y});
            glow.setFillColor(sf::Color(b.color.r, b.color.g, b.color.b, 25));
            window.draw(glow);

            sf::CircleShape c(b.radius, 24);
            c.setOrigin({b.radius, b.radius});
            c.setPosition({b.x, b.y});
            c.setFillColor(b.color);
            window.draw(c);

            sf::CircleShape spec(b.radius * 0.3f, 12);
            spec.setOrigin({b.radius * 0.3f, b.radius * 0.3f});
            spec.setPosition({b.x - b.radius * 0.25f, b.y - b.radius * 0.3f});
            spec.setFillColor(sf::Color(255, 255, 255, 55));
            window.draw(spec);
        }

        if (hasFont) {
            sf::Text title(font, "Hello Graphics!", 30);
            title.setFillColor(sf::Color::White);
            title.setStyle(sf::Text::Bold);
            auto tb = title.getLocalBounds();
            title.setPosition({(800 - tb.size.x) / 2, 18});
            window.draw(title);

            sf::Text sub(font, "CMake Generator - Error Recovery Demo", 13);
            sub.setFillColor(sf::Color(100, 130, 180));
            auto sb = sub.getLocalBounds();
            sub.setPosition({(800 - sb.size.x) / 2, 55});
            window.draw(sub);

            sf::Text esc(font, "Press Esc to return", 11);
            esc.setFillColor(sf::Color(70, 80, 100));
            auto eb = esc.getLocalBounds();
            esc.setPosition({(800 - eb.size.x) / 2, 577});
            window.draw(esc);
        }

        window.display();
    }
    return 0;
}
