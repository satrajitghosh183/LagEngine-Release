// Object2D.hpp
#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace engine::scene {

    /**
     * @brief Base class for all 2D objects in the scene
     *
     * Rendering is decoupled: objects produce vertex data via
     * getLineVertices() / getTriangleVertices() instead of drawing
     * directly. The main loop collects and uploads them to Vulkan.
     */
    class Object2D {
    public:
        bool active = true;    // Whether the object is updated
        bool visible = true;   // Whether the object is rendered
        std::string name = ""; // Optional identifier

        virtual ~Object2D() = default;

        virtual void update(float dt) = 0;

        /// Vertex with position and color (used for both lines and triangles)
        struct Vertex2D {
            glm::vec3 pos;   // z = 0 for 2D
            glm::vec3 color;
        };

        /// Return line-segment vertices (pairs of vertices)
        virtual std::vector<Vertex2D> getLineVertices() const { return {}; }

        /// Return triangle vertices (triples of vertices)
        virtual std::vector<Vertex2D> getTriangleVertices() const { return {}; }
    };

}
