// #include <SFML/Graphics.hpp>
// #include "engine/scene/SceneManager2D.hpp"
// #include "engine/objects/Ball2D.hpp"
// #include "engine/objects/Cloth2D.hpp"
// #include "engine/objects/Cannon2D.hpp"
// #include <vector>

// using namespace engine;

// int main() {
//     sf::RenderWindow window(sf::VideoMode(1280, 720), "2D Physics Scene");
//     window.setFramerateLimit(60);

//     scene::SceneManager2D sceneManager;
//     scene::Scene2D* scene = sceneManager.get2D();

//     std::vector<objects::Ball2D*> projectiles; 

//     auto cloth = new objects::Cloth2D(20, 10, 20.0f, {200, 50});
//     auto cannon = new objects::Cannon2D({300, 700});

//     cloth->setProjectiles(&projectiles); 

//     scene->add(cloth);
//     scene->add(cannon);

//     sf::Clock clock;
//     while (window.isOpen()) {
//         float dt = clock.restart().asSeconds();
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed)
//                 window.close();

//             if (event.type == sf::Event::KeyPressed) {
//                 if (event.key.code == sf::Keyboard::Left)
//                     cannon->rotate(-5.f);
//                 if (event.key.code == sf::Keyboard::Right)
//                     cannon->rotate(5.f);
//                 if (event.key.code == sf::Keyboard::Space) {
//                     auto velocity = cannon->getFiringVelocity();
//                     auto ball = new objects::Ball2D(cannon->position, velocity, 10.0f);
//                     scene->add(ball);
//                     projectiles.push_back(ball); 
//                 }
//             }
//         }

//         sceneManager.update2D(dt);
//         window.clear(sf::Color(30, 30, 30));
//         sceneManager.render2D(window);
//         window.display();
//     }

//     return 0;
// }




// // main2D.cpp
// #include <SFML/Graphics.hpp>
// #include <iostream>
// #include <vector>
// #include <random>
// #include <ctime>
// #include <memory>
// #include <string>

// #include "engine/scene/SceneManager2D.hpp"
// #include "engine/objects/Ball2D.hpp"
// #include "engine/objects/Cloth2D.hpp"
// #include "engine/objects/Cannon2D.hpp"

// using namespace engine;

// /**
//  * @brief Entry point for the 2D physics demo
//  */
// int main() {
//     // Create window with antialiasing
//     sf::ContextSettings settings;
//     settings.antialiasingLevel = 4;
//     sf::RenderWindow window(sf::VideoMode(1280, 720), "2D Physics Simulation", sf::Style::Default, settings);
//     window.setFramerateLimit(60);
    
//     // Initialize random number generator
//     std::srand(static_cast<unsigned int>(std::time(nullptr)));
//     std::mt19937 rng(std::rand());
//     std::uniform_int_distribution<int> colorDist(100, 255);
    
//     // Create scene manager and get main scene
//     scene::SceneManager2D sceneManager;
//     scene::Scene2D* scene = sceneManager.get2D();
    
//     // Create projectiles vector for cloth interaction
//     std::vector<objects::Ball2D*> projectiles;
    
//     // Create cloth positioned in the upper part of the screen
//     auto cloth = new objects::Cloth2D(20, 15, 20.0f, {300, 50});
//     cloth->setProjectiles(&projectiles);
    
//     // Create cannon positioned at the bottom
//     auto cannon = new objects::Cannon2D({300, 650});
    
//     // Add objects to scene
//     scene->add(cloth);
//     scene->add(cannon);
    
//     // Input state tracking
//     bool spacePressed = false;
//     bool upPressed = false;
//     bool downPressed = false;
    
//     // Load font for debug text
//     sf::Font font;
//     bool fontLoaded = false;
//     // Try multiple common font locations
//     const std::vector<std::string> fontPaths = {
//         "assets/fonts/arial.ttf",
//         "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
//         "/usr/share/fonts/TTF/DejaVuSans.ttf",
//         "/System/Library/Fonts/Helvetica.ttc",
//         "C:/Windows/Fonts/arial.ttf"
//     };
    
//     for (const auto& path : fontPaths) {
//         if (font.loadFromFile(path)) {
//             fontLoaded = true;
//             std::cout << "Loaded font from: " << path << std::endl;
//             break;
//         }
//     }
    
//     if (!fontLoaded) {
//         std::cout << "Failed to load font! Debug text will not be displayed." << std::endl;
//     }
    
//     // Debug text setup
//     sf::Text debugText;
//     debugText.setFont(font);
//     debugText.setCharacterSize(18);
//     debugText.setFillColor(sf::Color::White);
//     debugText.setPosition(10, 10);
    
//     // Reference sphere to demonstrate balls are working
//     sf::CircleShape referenceCircle(10.0f);
//     referenceCircle.setOrigin(10.0f, 10.0f);
//     referenceCircle.setPosition(50, 50);
//     referenceCircle.setFillColor(sf::Color::Green);
    
//     // Instruction text
//     sf::Text instructionText;
//     instructionText.setFont(font);
//     instructionText.setCharacterSize(14);
//     instructionText.setFillColor(sf::Color::Yellow);
//     instructionText.setPosition(10, 680);
//     instructionText.setString("Controls: Left/Right Arrow - Rotate Cannon | Up/Down Arrow - Adjust Power | Space - Fire Ball");
    

    
//     // Statistics
//     int ballsFired = 0;
//     int ballsActive = 0;
    
//     // Main game loop
//     sf::Clock clock;
//     while (window.isOpen()) {
//         // Calculate delta time with cap for stability
//         float dt = clock.restart().asSeconds();
//         if (dt > 0.1f) dt = 0.1f;
        
//         // Process events
//         sf::Event event;
//         while (window.pollEvent(event)) {
//             if (event.type == sf::Event::Closed) {
//                 window.close();
//             }
            
//             // Key press events
//             if (event.type == sf::Event::KeyPressed) {
//                 switch (event.key.code) {
//                     case sf::Keyboard::Left:
//                         cannon->rotate(-5.0f);
//                         break;
                        
//                     case sf::Keyboard::Right:
//                         cannon->rotate(5.0f);
//                         break;
                        
//                     case sf::Keyboard::Space:
//                         if (!spacePressed) {
//                             spacePressed = true;
                            
//                             // Get firing parameters
//                             sf::Vector2f muzzlePos = cannon->getMuzzlePosition();
//                             sf::Vector2f velocity = cannon->getFiringVelocity();
                            
//                             // Create a ball with random color
//                             sf::Color ballColor(
//                                 colorDist(rng),
//                                 colorDist(rng),
//                                 colorDist(rng)
//                             );
                            
//                             // Create and add ball
//                             auto ball = new objects::Ball2D(
//                                 muzzlePos,
//                                 velocity,
//                                 10.0f,
//                                 0.8f,
//                                 ballColor
//                             );
                            
//                             scene->add(ball);
//                             projectiles.push_back(ball);
//                             ballsFired++;
                            
//                             // Debug output
//                             std::cout << "Ball fired! Total: " << ballsFired << std::endl;
//                             std::cout << "Position: (" << muzzlePos.x << ", " << muzzlePos.y << ")" << std::endl;
//                             std::cout << "Velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
//                         }
//                         break;
                        
//                     case sf::Keyboard::Up:
//                         if (!upPressed) {
//                             upPressed = true;
//                             cannon->adjustPower(50.0f);
//                         }
//                         break;
                        
//                     case sf::Keyboard::Down:
//                         if (!downPressed) {
//                             downPressed = true;
//                             cannon->adjustPower(-50.0f);
//                         }
//                         break;
                        
//                     default:
//                         break;
//                 }
//             }
            
//             // Key release events
//             if (event.type == sf::Event::KeyReleased) {
//                 switch (event.key.code) {
//                     case sf::Keyboard::Space:
//                         spacePressed = false;
//                         break;
                        
//                     case sf::Keyboard::Up:
//                         upPressed = false;
//                         break;
                        
//                     case sf::Keyboard::Down:
//                         downPressed = false;
//                         break;
                        
//                     default:
//                         break;
//                 }
//             }
//         }
        
//         // Update scene
//         sceneManager.update2D(dt);
        
//         // Update debug info
//         ballsActive = projectiles.size();
//         if (fontLoaded) {
//             debugText.setString(
//                 "FPS: " + std::to_string(static_cast<int>(1.0f / dt)) + 
//                 "\nBalls Active: " + std::to_string(ballsActive) + 
//                 "\nBalls Fired: " + std::to_string(ballsFired) + 
//                 "\nCannon Angle: " + std::to_string(static_cast<int>(cannon->angle)) + "°" +
//                 "\nCannon Power: " + std::to_string(static_cast<int>(cannon->power))
//             );
//         }
        
//         // Render
//         window.clear(sf::Color(30, 30, 30));
        
//         // Render reference circle to verify drawing is working
//         window.draw(referenceCircle);
        
//         // Render scene
//         sceneManager.render2D(window);
        
//         // Render UI elements
//         if (fontLoaded) {
//             window.draw(debugText);
//             window.draw(instructionText);
//         }
        
//         // Swap buffers
//         window.display();
        
//         // Clean up inactive projectiles from tracking vector
//         auto oldSize = projectiles.size();
//         projectiles.erase(
//             std::remove_if(projectiles.begin(), projectiles.end(),
//                 [](objects::Ball2D* ball) { return !ball || !ball->active; }),
//             projectiles.end()
//         );
        
//         // Log cleanup
//         if (oldSize != projectiles.size()) {
//             std::cout << "Removed " << (oldSize - projectiles.size()) 
//                       << " inactive balls. Remaining: " << projectiles.size() << std::endl;
//         }
//     }
    
//     // Cleanup - objects owned by scene will be deleted automatically
//     // But projectiles vector might have pointers the scene doesn't have anymore
//     projectiles.clear();
    
//     std::cout << "Application terminated successfully" << std::endl;
//     return 0;
// }



// main2D.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <memory>
#include <string>

#include "engine/scene/SceneManager2D.hpp"
#include "engine/objects/Ball2D.hpp"
#include "engine/objects/Cloth2D.hpp"
#include "engine/objects/Cannon2D.hpp"

using namespace engine;

/**
 * @brief Function to respawn the cloth
 * @param scene Pointer to the scene
 * @param projectiles Pointer to projectiles vector
 * @param cloth Pointer to the current cloth object (will be replaced)
 * @param position Position for the new cloth
 */
void respawnCloth(scene::Scene2D* scene, std::vector<objects::Ball2D*>* projectiles, 
                  objects::Cloth2D** cloth, const sf::Vector2f& position = {300, 50}) {
    // Create new cloth first
    objects::Cloth2D* newCloth = new objects::Cloth2D(20, 15, 20.0f, position);
    newCloth->setProjectiles(projectiles);
    
    // Add new cloth to scene
    scene->add(newCloth);
    
    // Now set the old cloth to inactive - it will be removed by the scene's update method
    if (*cloth) {
        (*cloth)->active = false;
        // Don't delete it here - let the scene manager handle deletion
    }
    
    // Update the cloth pointer to point to the new cloth
    *cloth = newCloth;
    
    std::cout << "Cloth respawned at position (" << position.x << ", " << position.y << ")" << std::endl;
}

/**
 * @brief Entry point for the 2D physics demo
 */
int main() {
    // Create window with antialiasing
    sf::ContextSettings settings;
    settings.antialiasingLevel = 4;
    sf::RenderWindow window(sf::VideoMode(1280, 720), "2D Physics Simulation", sf::Style::Default, settings);
    window.setFramerateLimit(60);
    
    // Initialize random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::mt19937 rng(std::rand());
    std::uniform_int_distribution<int> colorDist(100, 255);
    
    // Create scene manager and get main scene
    scene::SceneManager2D sceneManager;
    scene::Scene2D* scene = sceneManager.get2D();
    
    // Create projectiles vector for cloth interaction
    std::vector<objects::Ball2D*> projectiles;
    
    // Create cloth positioned in the upper part of the screen
    auto cloth = new objects::Cloth2D(20, 15, 20.0f, {300, 50});
    cloth->setProjectiles(&projectiles);
    
    // Create cannon positioned at the bottom
    auto cannon = new objects::Cannon2D({300, 650});
    
    // Add objects to scene
    scene->add(cloth);
    scene->add(cannon);
    
    // Input state tracking
    bool spacePressed = false;
    bool upPressed = false;
    bool downPressed = false;
    bool rKeyPressed = false; // Added for cloth respawn
    
    // Load font for debug text
    sf::Font font;
    bool fontLoaded = false;
    // Try multiple common font locations
    const std::vector<std::string> fontPaths = {
        "assets/fonts/arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "C:/Windows/Fonts/arial.ttf"
    };
    
    for (const auto& path : fontPaths) {
        if (font.loadFromFile(path)) {
            fontLoaded = true;
            std::cout << "Loaded font from: " << path << std::endl;
            break;
        }
    }
    
    if (!fontLoaded) {
        std::cout << "Failed to load font! Debug text will not be displayed." << std::endl;
    }
    
    // Debug text setup
    sf::Text debugText;
    debugText.setFont(font);
    debugText.setCharacterSize(18);
    debugText.setFillColor(sf::Color::White);
    debugText.setPosition(10, 10);
    
    // Reference sphere to demonstrate balls are working
    sf::CircleShape referenceCircle(10.0f);
    referenceCircle.setOrigin(10.0f, 10.0f);
    referenceCircle.setPosition(50, 50);
    referenceCircle.setFillColor(sf::Color::Green);
    
    // Instruction text
    sf::Text instructionText;
    instructionText.setFont(font);
    instructionText.setCharacterSize(14);
    instructionText.setFillColor(sf::Color::Yellow);
    instructionText.setPosition(10, 680);
    instructionText.setString("Controls: Left/Right Arrow - Rotate Cannon | Up/Down Arrow - Adjust Power | Space - Fire Ball | R - Respawn Cloth");
    
    // Statistics
    int ballsFired = 0;
    int ballsActive = 0;
    
    // Main game loop
    sf::Clock clock;
    while (window.isOpen()) {
        // Calculate delta time with cap for stability
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;
        
        // Process events
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            // Key press events
            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::Left:
                        cannon->rotate(-5.0f);
                        break;
                        
                    case sf::Keyboard::Right:
                        cannon->rotate(5.0f);
                        break;
                        
                    case sf::Keyboard::Space:
                        if (!spacePressed) {
                            spacePressed = true;
                            
                            // Get firing parameters
                            sf::Vector2f muzzlePos = cannon->getMuzzlePosition();
                            sf::Vector2f velocity = cannon->getFiringVelocity();
                            
                            // Create a ball with random color
                            sf::Color ballColor(
                                colorDist(rng),
                                colorDist(rng),
                                colorDist(rng)
                            );
                            
                            // Create and add ball
                            auto ball = new objects::Ball2D(
                                muzzlePos,
                                velocity,
                                10.0f,
                                0.8f,
                                ballColor
                            );
                            
                            scene->add(ball);
                            projectiles.push_back(ball);
                            ballsFired++;
                            
                            // Debug output
                            std::cout << "Ball fired! Total: " << ballsFired << std::endl;
                            std::cout << "Position: (" << muzzlePos.x << ", " << muzzlePos.y << ")" << std::endl;
                            std::cout << "Velocity: (" << velocity.x << ", " << velocity.y << ")" << std::endl;
                        }
                        break;
                        
                    case sf::Keyboard::Up:
                        if (!upPressed) {
                            upPressed = true;
                            cannon->adjustPower(50.0f);
                        }
                        break;
                        
                    case sf::Keyboard::Down:
                        if (!downPressed) {
                            downPressed = true;
                            cannon->adjustPower(-50.0f);
                        }
                        break;
                        
                    case sf::Keyboard::R:
                        if (!rKeyPressed) {
                            rKeyPressed = true;
                            respawnCloth(scene, &projectiles, &cloth);
                        }
                        break;
                        
                    default:
                        break;
                }
            }
            
            // Key release events
            if (event.type == sf::Event::KeyReleased) {
                switch (event.key.code) {
                    case sf::Keyboard::Space:
                        spacePressed = false;
                        break;
                        
                    case sf::Keyboard::Up:
                        upPressed = false;
                        break;
                        
                    case sf::Keyboard::Down:
                        downPressed = false;
                        break;
                        
                    case sf::Keyboard::R:
                        rKeyPressed = false;
                        break;
                        
                    default:
                        break;
                }
            }
        }
        
        // Update scene
        sceneManager.update2D(dt);
        
        // Update debug info
        ballsActive = projectiles.size();
        if (fontLoaded) {
            debugText.setString(
                "FPS: " + std::to_string(static_cast<int>(1.0f / dt)) + 
                "\nBalls Active: " + std::to_string(ballsActive) + 
                "\nBalls Fired: " + std::to_string(ballsFired) + 
                "\nCannon Angle: " + std::to_string(static_cast<int>(cannon->angle)) + "°" +
                "\nCannon Power: " + std::to_string(static_cast<int>(cannon->power))
            );
        }
        
        // Render
        window.clear(sf::Color(30, 30, 30));
        
        // Render reference circle to verify drawing is working
        window.draw(referenceCircle);
        
        // Render scene
        sceneManager.render2D(window);
        
        // Render UI elements
        if (fontLoaded) {
            window.draw(debugText);
            window.draw(instructionText);
        }
        
        // Swap buffers
        window.display();
        
        // Clean up inactive projectiles from tracking vector
        auto oldSize = projectiles.size();
        projectiles.erase(
            std::remove_if(projectiles.begin(), projectiles.end(),
                [](objects::Ball2D* ball) { return !ball || !ball->active; }),
            projectiles.end()
        );
        
        // Log cleanup
        if (oldSize != projectiles.size()) {
            std::cout << "Removed " << (oldSize - projectiles.size()) 
                      << " inactive balls. Remaining: " << projectiles.size() << std::endl;
        }
    }
    
    // Cleanup - objects owned by scene will be deleted automatically
    // But projectiles vector might have pointers the scene doesn't have anymore
    projectiles.clear();
    
    std::cout << "Application terminated successfully" << std::endl;
    return 0;
}