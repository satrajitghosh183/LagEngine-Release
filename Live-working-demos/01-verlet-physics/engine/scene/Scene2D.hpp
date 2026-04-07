

// #pragma once
// #include <vector>
// #include "Object2D.hpp"
// #include <SFML/Graphics/RenderWindow.hpp>

// namespace engine::scene {

//     class Scene2D {
//     public:
//         std::vector<Object2D*> objects;

//         void add(Object2D* obj) {
//             objects.push_back(obj);
//         }

//         void update(float dt) {
//             for (auto* obj : objects) {
//                 if (obj && obj->active)
//                     obj->update(dt);
//             }
//             // Cleanup dead objects
//             objects.erase(std::remove_if(objects.begin(), objects.end(),
//                 [](Object2D* obj) { return obj && !obj->active; }),
//                 objects.end());
//         }

//         void render(sf::RenderWindow& window) {
//             for (auto* obj : objects)
//                 if (obj && obj->visible)
//                     obj->render(window);
//         }
//     };

// }


// Scene2D.hpp
#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include "Object2D.hpp"
#include <SFML/Graphics/RenderWindow.hpp>

namespace engine::scene {

    /**
     * @brief Manages a collection of 2D objects
     */
    class Scene2D {
    private:
        std::vector<Object2D*> objects;
        bool needsSort = false;

    public:
        /**
         * @brief Default constructor
         */
        Scene2D() = default;
        
        /**
         * @brief Destructor that cleans up all objects
         */
        ~Scene2D() {
            clear();
        }
        
        /**
         * @brief Add an object to the scene
         * @param obj Pointer to the object
         */
        void add(Object2D* obj) {
            if (obj) {
                objects.push_back(obj);
                needsSort = true;
            }
        }
        
        /**
         * @brief Update all active objects
         * @param dt Delta time in seconds
         */
        void update(float dt) {
            // Update all active objects
            for (auto* obj : objects) {
                if (obj && obj->active) {
                    obj->update(dt);
                }
            }
            
            // Remove inactive objects (objects manage their own active state)
            size_t originalSize = objects.size();
            objects.erase(
                std::remove_if(objects.begin(), objects.end(),
                    [](Object2D* obj) { return !obj || !obj->active; }),
                objects.end()
            );
            
            // Debug output for object removal
            size_t removedCount = originalSize - objects.size();
            if (removedCount > 0) {
                std::cout << "Removed " << removedCount << " inactive objects. "
                          << objects.size() << " objects remaining." << std::endl;
            }
        }
        
        /**
         * @brief Render all visible objects
         * @param window Target rendering window
         */
        void render(sf::RenderWindow& window) {
            for (auto* obj : objects) {
                if (obj && obj->visible) {
                    obj->render(window);
                }
            }
        }
        
        /**
         * @brief Get the number of objects in the scene
         * @return Object count
         */
        size_t size() const {
            return objects.size();
        }
        
        /**
         * @brief Remove all objects from the scene
         */
        void clear() {
            for (auto* obj : objects) {
                delete obj;
            }
            objects.clear();
        }
    };

}