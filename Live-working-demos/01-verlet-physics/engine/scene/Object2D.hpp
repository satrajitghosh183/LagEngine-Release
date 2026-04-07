
// #pragma once
// #include <SFML/Graphics.hpp>
// #include <glm/gtc/matrix_transform.hpp>

// namespace engine::scene {

//     class Object2D {
//     public:
//         virtual ~Object2D() = default;

//         virtual void update(float dt) = 0;
//         virtual void render(sf::RenderWindow& window) = 0;

//         bool active = true;
//         bool visible = true; 
//     };

// }


// Object2D.hpp
#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>

namespace engine::scene {

    /**
     * @brief Base class for all 2D objects in the scene
     */
    class Object2D {
    public:
        bool active = true;    // Whether the object is updated
        bool visible = true;   // Whether the object is rendered
        std::string name = ""; // Optional identifier

        /**
         * @brief Virtual destructor to ensure proper cleanup
         */
        virtual ~Object2D() = default;

        /**
         * @brief Update the object state
         * @param dt Delta time in seconds
         */
        virtual void update(float dt) = 0;

        /**
         * @brief Render the object to the window
         * @param window Target rendering window
         */
        virtual void render(sf::RenderWindow& window) = 0;
    };

}