#include "raytracer.h"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <chrono>
#include <cmath>

static constexpr unsigned int WIDTH  = 800;
static constexpr unsigned int HEIGHT = 600;
static constexpr int SCANLINES_PER_FRAME = 6;

int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Mini Raytracer");
    window.setFramerateLimit(60);

    sf::Font font;
    bool hasFont = font.openFromFile("/System/Library/Fonts/SFNSMono.ttf")
               || font.openFromFile("/System/Library/Fonts/Menlo.ttc")
               || font.openFromFile("/System/Library/Fonts/Monaco.ttf")
               || font.openFromFile("/System/Library/Fonts/Helvetica.ttc");

    Scene scene = buildDemoScene();

    // Camera setup
    Vec3 camPos  = {0.0f, 1.0f, 2.5f};
    float fov    = 60.0f;
    float aspect = static_cast<float>(WIDTH) / HEIGHT;
    float scale  = std::tan(fov * 0.5f * 3.14159f / 180.0f);

    // Render state
    sf::Image image(sf::Vector2u{WIDTH, HEIGHT}, sf::Color::Black);
    sf::Texture texture;
    (void)texture.loadFromImage(image);
    int currentScanline = 0;
    bool renderDone = false;

    auto startTime = std::chrono::steady_clock::now();
    float renderTime = 0.0f;

    while (window.isOpen()) {
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                window.close();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                // Re-render
                if (key->code == sf::Keyboard::Key::R) {
                    currentScanline = 0;
                    renderDone = false;
                    image = sf::Image(sf::Vector2u{WIDTH, HEIGHT}, sf::Color::Black);
                    startTime = std::chrono::steady_clock::now();
                }
            }
        }

        // Progressive rendering: a few scanlines per frame
        if (!renderDone) {
            int endLine = std::min(currentScanline + SCANLINES_PER_FRAME,
                                   static_cast<int>(HEIGHT));
            for (int y = currentScanline; y < endLine; ++y) {
                for (unsigned int x = 0; x < WIDTH; ++x) {
                    // Normalized device coords (-1..1)
                    float px = (2.0f * (x + 0.5f) / WIDTH - 1.0f) * aspect * scale;
                    float py = (1.0f - 2.0f * (y + 0.5f) / HEIGHT) * scale;

                    Ray ray{camPos, Vec3{px, py, -1.0f}.normalized()};
                    Vec3 col = scene.trace(ray);

                    // Gamma correction
                    col = {std::sqrt(col.x), std::sqrt(col.y), std::sqrt(col.z)};

                    auto r = static_cast<uint8_t>(std::clamp(col.x, 0.f, 1.f) * 255);
                    auto g = static_cast<uint8_t>(std::clamp(col.y, 0.f, 1.f) * 255);
                    auto b = static_cast<uint8_t>(std::clamp(col.z, 0.f, 1.f) * 255);
                    image.setPixel(sf::Vector2u{x, static_cast<unsigned>(y)},
                                   sf::Color(r, g, b));
                }
            }
            currentScanline = endLine;
            if (currentScanline >= static_cast<int>(HEIGHT)) {
                renderDone = true;
                auto endTime = std::chrono::steady_clock::now();
                renderTime = std::chrono::duration<float>(endTime - startTime).count();
            }
            texture.update(image);
        }

        // Draw
        window.clear(sf::Color::Black);
        sf::Sprite sprite(texture);
        window.draw(sprite);

        // Overlay
        if (hasFont) {
            // Scanline progress indicator
            if (!renderDone) {
                float progress = static_cast<float>(currentScanline) / HEIGHT;
                // Progress bar background
                sf::RectangleShape barBg(sf::Vector2f{200.f, 8.f});
                barBg.setPosition({WIDTH - 220.f, 14.f});
                barBg.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(barBg);
                // Progress bar fill
                sf::RectangleShape barFill(sf::Vector2f{200.f * progress, 8.f});
                barFill.setPosition({WIDTH - 220.f, 14.f});
                barFill.setFillColor(sf::Color(80, 180, 255));
                window.draw(barFill);

                std::ostringstream oss;
                oss << "Rendering... " << static_cast<int>(progress * 100) << "%";
                sf::Text txt(font, oss.str(), 13);
                txt.setFillColor(sf::Color(200, 220, 255, 220));
                txt.setPosition({WIDTH - 220.f, 24.f});
                window.draw(txt);

                // Scanline cursor
                sf::Vertex line[] = {
                    {{0.f, static_cast<float>(currentScanline)}, sf::Color(80, 180, 255, 120)},
                    {{static_cast<float>(WIDTH), static_cast<float>(currentScanline)}, sf::Color(80, 180, 255, 120)}
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);
            } else {
                std::ostringstream oss;
                oss << "Render complete  " << std::fixed;
                oss.precision(2);
                oss << renderTime << "s  |  " << scene.spheres.size() << " spheres  "
                    << scene.maxBounces << " bounces  |  [R] Re-render  [Esc] Quit";
                sf::Text txt(font, oss.str(), 13);
                txt.setFillColor(sf::Color(180, 220, 180, 200));
                txt.setPosition({10.f, static_cast<float>(HEIGHT) - 28.f});
                window.draw(txt);
            }

            // Title
            sf::Text title(font, "Mini Raytracer", 15);
            title.setFillColor(sf::Color(255, 255, 255, 180));
            title.setStyle(sf::Text::Bold);
            title.setPosition({10.f, 10.f});
            window.draw(title);
        }

        window.display();
    }
    return 0;
}
