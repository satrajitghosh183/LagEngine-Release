// #pragma once
// #include "Scene2D.hpp"
// #include <SFML/Graphics/RenderWindow.hpp>

// namespace engine::scene {

//     class SceneManager2D {
//     private:
//         Scene2D scene;

//     public:
//         Scene2D* get2D();                                // Declaration
//         void update2D(float dt);                         // Declaration
//         void render2D(sf::RenderWindow& window);         // Declaration
//     };

// } // namespace engine::scene



// SceneManager2D.hpp
#pragma once
#include "Scene2D.hpp"
#include <SFML/Graphics/RenderWindow.hpp>
#include <memory>

namespace engine::scene {

    /**
     * @brief Manages scenes and provides global access
     */
    class SceneManager2D {
    private:
        Scene2D mainScene;
        
    public:
        /**
         * @brief Get the main 2D scene
         * @return Pointer to the scene
         */
        Scene2D* get2D();
        
        /**
         * @brief Update the main 2D scene
         * @param dt Delta time in seconds
         */
        void update2D(float dt);
        
        /**
         * @brief Render the main 2D scene
         * @param window Target rendering window
         */
        void render2D(sf::RenderWindow& window);
    };

} // namespace engine::scene