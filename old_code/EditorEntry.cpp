#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <iostream>

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Game Engine Editor");
    window.setFramerateLimit(60);
    ImGui::SFML::Init(window);

    sf::Clock deltaClock;
    bool showDemo = true;
    float value = 0.0f;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);
            if (event.type == sf::Event::Closed)
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Scene Hierarchy");
        ImGui::Text("Engine Running...");
        ImGui::Checkbox("Show ImGui Demo", &showDemo);
        ImGui::SliderFloat("Value", &value, 0.0f, 100.0f);
        ImGui::End();

        if (showDemo) ImGui::ShowDemoWindow();

        window.clear(sf::Color(20, 20, 25));
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
