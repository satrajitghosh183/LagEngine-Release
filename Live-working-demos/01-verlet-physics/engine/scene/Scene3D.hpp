
#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include "../graphics/Texture2D.hpp"  // ✅ Add this
#include "../graphics/Camera.hpp"
#include "../graphics/Shader.hpp"
#include "../graphics/Mesh3D.hpp"
#include "../physics/PhysicsWorld3D.hpp"
#include "../physics/ClothSolver3D.hpp"
#include "../objects/Ball3D.hpp"

namespace engine::scene {

class Scene3D {
public:
    Scene3D(float aspectRatio);

    void update(float dt);
    void render() const;

    std::shared_ptr<engine::physics::PhysicsWorld3D> getPhysicsWorld();
    std::shared_ptr<engine::graphics::Camera> getCamera();

    std::shared_ptr<engine::graphics::Texture2D> clothTexture;

    std::shared_ptr<engine::graphics::Camera> camera;
    std::shared_ptr<engine::graphics::Shader> clothShader;
    std::shared_ptr<engine::graphics::Shader> ballShader;

    std::shared_ptr<engine::physics::PhysicsWorld3D> physicsWorld;
    std::shared_ptr<engine::graphics::Mesh3D> clothMesh;

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
    
};

} // namespace engine::scene
