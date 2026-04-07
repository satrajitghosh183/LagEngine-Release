// // engine/objects/Cannon2D.cpp
// #include "engine/objects/Cannon2D.hpp"
// #include <cmath>

// namespace engine::objects {

//     Cannon2D::Cannon2D(const sf::Vector2f& pos)
//         : position(pos) {}

//     void Cannon2D::rotate(float degrees) {
//         angle += degrees;
//         if (angle < -170.f) angle = -170.f; // Clamp rotation range
//         if (angle > -10.f) angle = -10.f;
//     }

//     void Cannon2D::update(float) {
//         // Cannon itself doesn't need physics updates
//     }

//     void Cannon2D::render(sf::RenderWindow& window) {
//         sf::RectangleShape barrel(sf::Vector2f(50.f, 8.f));
//         barrel.setOrigin(0.f, 4.f);
//         barrel.setPosition(position);
//         barrel.setRotation(angle);
//         barrel.setFillColor(sf::Color::Red);
//         window.draw(barrel);

//         sf::CircleShape base(15.f);
//         base.setOrigin(15.f, 15.f);
//         base.setPosition(position);
//         base.setFillColor(sf::Color::Blue);
//         window.draw(base);
//     }

//     sf::Vector2f Cannon2D::getFiringVelocity() const {
//         float rad = angle * 3.14159265f / 180.f;
//         return { std::cos(rad) * power, std::sin(rad) * power };
//     }

// }


// Cannon2D.cpp
#include "engine/objects/Cannon2D.hpp"
#include <cmath>
#include <iostream>

namespace engine::objects {

    // Pi constant for angle calculations
    constexpr float PI = 3.14159265358979323846f;

    Cannon2D::Cannon2D(const sf::Vector2f& pos)
        : position(pos) {
        active = true;
        visible = true;
    }

    void Cannon2D::rotate(float degrees) {
        angle += degrees;
        
        // Clamp rotation range between -170° and -10°
        if (angle < -170.f) angle = -170.f;
        if (angle > -10.f) angle = -10.f;
        
        std::cout << "Cannon angle: " << angle << "°" << std::endl;
    }

    void Cannon2D::update(float) {
        // Cannon doesn't need physics updates
    }

    // void Cannon2D::render(sf::RenderWindow& window) {
    //     // Draw barrel
    //     sf::RectangleShape barrel(sf::Vector2f(50.f, 8.f));
    //     barrel.setOrigin(0.f, 4.f);
    //     barrel.setPosition(position);
    //     barrel.setRotation(angle);
    //     barrel.setFillColor(sf::Color::Red);
        
    //     // Add outline to barrel for better visibility
    //     barrel.setOutlineThickness(1.0f);
    //     barrel.setOutlineColor(sf::Color::White);
        
    //     // Draw base
    //     sf::CircleShape base(15.f);
    //     base.setOrigin(15.f, 15.f);
    //     base.setPosition(position);
    //     base.setFillColor(sf::Color::Blue);
        
    //     // Add outline to base
    //     base.setOutlineThickness(1.0f);
    //     base.setOutlineColor(sf::Color::White);
        
    //     // Draw base first, then barrel
    //     window.draw(base);
    //     window.draw(barrel);
        
    //     // Optional: Draw power indicator
    //     float powerIndicatorLength = power / 20.0f; // Scale for visibility
    //     sf::RectangleShape powerIndicator(sf::Vector2f(powerIndicatorLength, 4.0f));
    //     powerIndicator.setPosition(position.x - powerIndicatorLength / 2, position.y + 20.0f);
    //     powerIndicator.setFillColor(sf::Color::Green);
    //     window.draw(powerIndicator);
    // }


    void Cannon2D::render(sf::RenderWindow& window) {
        // Create a more detailed, realistic cannon
        
        // 1. Draw the cannon mount/base
        sf::CircleShape base(20.f);
        base.setOrigin(20.f, 20.f);
        base.setPosition(position);
        base.setFillColor(sf::Color(70, 70, 75));  // Dark metallic gray
        
        // Add base plate under the mount
        sf::RectangleShape basePlate(sf::Vector2f(50.f, 10.f));
        basePlate.setOrigin(25.f, 5.f);
        basePlate.setPosition(position.x, position.y + 15.f);
        basePlate.setFillColor(sf::Color(50, 50, 55));  // Darker metal
        
        // 2. Draw the barrel assembly
        // Main barrel
        sf::RectangleShape barrel(sf::Vector2f(60.f, 12.f));
        barrel.setOrigin(0.f, 6.f);
        barrel.setPosition(position);
        barrel.setRotation(angle);
        barrel.setFillColor(sf::Color(40, 40, 45));  // Dark metal
        
        // Barrel reinforcement rings
        float ringWidth = 3.f;
        for (int i = 1; i <= 3; i++) {
            sf::RectangleShape ring(sf::Vector2f(ringWidth, 16.f));
            ring.setOrigin(0.f, 8.f);
            ring.setPosition(position);
            ring.setRotation(angle);
            ring.setFillColor(sf::Color(90, 90, 95));  // Lighter metal ring
            
            // Transform to position along barrel
            float distance = 10.f + (i * 15.f); // Spaced along barrel
            float ringAngleRad = angle * 3.14159f / 180.f;
            ring.setPosition(
                position.x + cos(ringAngleRad) * distance,
                position.y + sin(ringAngleRad) * distance
            );
            window.draw(ring);
        }
        
        // Barrel muzzle (end of barrel)
        sf::RectangleShape muzzle(sf::Vector2f(5.f, 14.f));
        muzzle.setOrigin(0.f, 7.f);
        muzzle.setPosition(position);
        muzzle.setRotation(angle);
        muzzle.setFillColor(sf::Color(100, 100, 105));  // Lighter metal
        
        // Transform to position at end of barrel
        float muzzleAngleRad = angle * 3.14159f / 180.f;
        muzzle.setPosition(
            position.x + cos(muzzleAngleRad) * 60.f,  // At end of barrel
            position.y + sin(muzzleAngleRad) * 60.f
        );
        
        // 3. Draw the recoil mechanism
        sf::RectangleShape recoilSystem(sf::Vector2f(15.f, 10.f));
        recoilSystem.setOrigin(0.f, 5.f);
        recoilSystem.setPosition(position);
        recoilSystem.setRotation(angle);
        recoilSystem.setFillColor(sf::Color(110, 110, 120));  // Lighter mechanism color
        
        // 4. Draw firing controls
        sf::CircleShape firingMechanism(5.f);
        firingMechanism.setOrigin(5.f, 5.f);
        firingMechanism.setPosition(
            position.x - cos(muzzleAngleRad) * 10.f,  // Behind the barrel
            position.y - sin(muzzleAngleRad) * 10.f
        );
        firingMechanism.setFillColor(sf::Color(180, 30, 30));  // Red control mechanism
        
        // 5. Draw elevation control wheel
        sf::CircleShape elevationWheel(6.f);
        elevationWheel.setOrigin(6.f, 6.f);
        elevationWheel.setPosition(
            position.x - cos(muzzleAngleRad) * 5.f,
            position.y - sin(muzzleAngleRad) * 5.f + 12.f
        );
        elevationWheel.setFillColor(sf::Color(200, 200, 100));  // Brass color
        
        // 6. Add a subtle shadow
        sf::CircleShape shadow(25.f);
        shadow.setOrigin(25.f, 25.f);
        shadow.setPosition(position.x + 5.f, position.y + 5.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 50));  // Translucent black
        
        // 7. Power indicator
        float powerPercent = (power - 200.f) / 800.f;  // Range from 0.0 to 1.0
        sf::RectangleShape powerBar(sf::Vector2f(50.f * powerPercent, 5.f));
        powerBar.setPosition(position.x - 25.f, position.y + 30.f);
        
        // Color gradient based on power
        sf::Color powerColor;
        if (powerPercent < 0.3f)
            powerColor = sf::Color(0, 200, 0);  // Green for low power
        else if (powerPercent < 0.7f)
            powerColor = sf::Color(200, 200, 0);  // Yellow for medium power
        else
            powerColor = sf::Color(200, 0, 0);  // Red for high power
        
        powerBar.setFillColor(powerColor);
        
        // Power indicator background
        sf::RectangleShape powerBarBg(sf::Vector2f(50.f, 5.f));
        powerBarBg.setPosition(position.x - 25.f, position.y + 30.f);
        powerBarBg.setFillColor(sf::Color(50, 50, 50));
        powerBarBg.setOutlineThickness(1.f);
        powerBarBg.setOutlineColor(sf::Color(100, 100, 100));
        
        // 8. Draw components in correct order (back to front)
        window.draw(shadow);
        window.draw(basePlate);
        window.draw(base);
        window.draw(recoilSystem);
        window.draw(barrel);
        window.draw(muzzle);
        window.draw(firingMechanism);
        window.draw(elevationWheel);
        window.draw(powerBarBg);
        window.draw(powerBar);
        
        
        static float timeSinceFired = 0.f;
        if (timeSinceFired < 0.5f) {
            sf::CircleShape smoke(timeSinceFired * 20.f);
            smoke.setOrigin(timeSinceFired * 20.f, timeSinceFired * 20.f);
            smoke.setPosition(getMuzzlePosition());
            smoke.setFillColor(sf::Color(200, 200, 200, 255 * (1.0f - timeSinceFired * 2.0f)));
            window.draw(smoke);
        }
        
    }
    sf::Vector2f Cannon2D::getFiringVelocity() const {
        // Convert angle from degrees to radians
        float rad = angle * PI / 180.f;
        
        // Calculate directional components with adjusted vertical component
        // Use 0.6f as a scaling factor to make trajectories more believable
        return { 
            std::cos(rad) * power * 0.2f, 
            std::sin(rad) * power * 0.2f
        };
    }

    sf::Vector2f Cannon2D::getMuzzlePosition() const {
        // Convert angle from degrees to radians
        float rad = angle * PI / 180.f;
        
        // Length of the barrel
        float barrelLength = 60.0f;
        
        // Calculate position at the end of the barrel
        return {
            position.x + std::cos(rad) * barrelLength,
            position.y + std::sin(rad) * barrelLength
        };
    }
    
    void Cannon2D::adjustPower(float amount) {
        power += amount;
        
        // Clamp power between reasonable limits
        if (power < 200.0f) power = 200.0f;
        if (power > 1000.0f) power = 1000.0f;
        
        std::cout << "Cannon power adjusted to: " << power << std::endl;
    }
}