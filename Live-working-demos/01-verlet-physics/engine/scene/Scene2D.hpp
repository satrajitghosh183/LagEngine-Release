// Scene2D.hpp
#pragma once
#include <vector>
#include <algorithm>
#include <iostream>
#include "Object2D.hpp"

namespace engine::scene {

    /**
     * @brief Manages a collection of 2D objects
     */
    class Scene2D {
    private:
        std::vector<Object2D*> objects;

    public:
        Scene2D() = default;

        ~Scene2D() {
            clear();
        }

        void add(Object2D* obj) {
            if (obj) {
                objects.push_back(obj);
            }
        }

        void update(float dt) {
            for (auto* obj : objects) {
                if (obj && obj->active) {
                    obj->update(dt);
                }
            }

            size_t originalSize = objects.size();
            objects.erase(
                std::remove_if(objects.begin(), objects.end(),
                    [](Object2D* obj) { return !obj || !obj->active; }),
                objects.end()
            );

            size_t removedCount = originalSize - objects.size();
            if (removedCount > 0) {
                std::cout << "Removed " << removedCount << " inactive objects. "
                          << objects.size() << " objects remaining." << std::endl;
            }
        }

        /// Collect all line vertices from all visible objects
        std::vector<Object2D::Vertex2D> gatherLineVertices() const {
            std::vector<Object2D::Vertex2D> all;
            for (auto* obj : objects) {
                if (obj && obj->visible) {
                    auto v = obj->getLineVertices();
                    all.insert(all.end(), v.begin(), v.end());
                }
            }
            return all;
        }

        /// Collect all triangle vertices from all visible objects
        std::vector<Object2D::Vertex2D> gatherTriangleVertices() const {
            std::vector<Object2D::Vertex2D> all;
            for (auto* obj : objects) {
                if (obj && obj->visible) {
                    auto v = obj->getTriangleVertices();
                    all.insert(all.end(), v.begin(), v.end());
                }
            }
            return all;
        }

        size_t size() const {
            return objects.size();
        }

        void clear() {
            for (auto* obj : objects) {
                delete obj;
            }
            objects.clear();
        }
    };

}
