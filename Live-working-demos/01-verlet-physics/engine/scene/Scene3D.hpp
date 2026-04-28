#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../graphics/Camera.hpp"
#include "../graphics/MeshGenerator3D.hpp"
#include "../physics/PhysicsWorld3D.hpp"
#include "../physics/ClothSolver3D.hpp"
#include "../objects/Ball3D.hpp"

namespace engine::scene {

class Scene3D {
public:
    Scene3D(float aspectRatio);

    void update(float dt);

    /// Get physics world
    std::shared_ptr<engine::physics::PhysicsWorld3D> getPhysicsWorld();
    std::shared_ptr<engine::graphics::Camera> getCamera();

    std::shared_ptr<engine::graphics::Camera> camera;
    std::shared_ptr<engine::physics::PhysicsWorld3D> physicsWorld;

    int clothWidth;
    int clothHeight;

    struct Material {
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
        float shininess;
    };

    struct Light {
        glm::vec3 position;
        glm::vec3 ambient;
        glm::vec3 diffuse;
        glm::vec3 specular;
    };

    Material clothMaterial;
    Light sceneLight;

    /// Updated vertex data (position + normal), refreshed every frame
    struct VertexPN {
        glm::vec3 position;
        glm::vec3 normal;
    };
    std::vector<VertexPN> clothVertices;
    std::vector<unsigned int> clothIndices;
};

} // namespace engine::scene
