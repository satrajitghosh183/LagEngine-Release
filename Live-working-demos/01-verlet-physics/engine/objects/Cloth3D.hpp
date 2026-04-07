#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "engine/physics/Particle3D.hpp"
#include "engine/physics/Spring3D.hpp"
#include "engine/graphics/Mesh.hpp"
#include "engine/scene/SceneManager3D.hpp"

namespace engine::objects {

/**
 * @brief Represents a realistic 3D cloth simulation.
 */
class Cloth3D {
public:
    int width, height;
    float spacing;
    glm::vec3 origin;

    std::vector<std::shared_ptr<physics::Particle3D>> particles;

    std::vector<physics::Spring3D> springs;

    engine::graphics::Mesh mesh;

    std::reference_wrapper<scene::SceneManager3D> sceneManagerRef;

    /**
     * @brief Constructor
     */
    Cloth3D(int width, int height, float spacing, const glm::vec3& origin, scene::SceneManager3D& sceneManager);

    /**
     * @brief Update the mesh and physics (position updates happen externally)
     */
    void update(float dt);

    /**
     * @brief Render the cloth using a shader
     */
    void render(const engine::graphics::Shader& shader);

private:
    void createParticles();
    void createSprings();
    void createMesh();

    void updateMesh();
};

} // namespace engine::objects
