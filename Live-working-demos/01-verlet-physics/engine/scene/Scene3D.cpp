#include "Scene3D.hpp"
#include "../graphics/MeshGenerator3D.hpp"

#include <cstdlib>
#include <ctime>
#include <glm/gtc/constants.hpp> // For pi constants if needed
#include <GLFW/glfw3.h>  // 👈 Add this line

namespace engine::scene {

Scene3D::Scene3D(float aspectRatio) 
    : clothWidth(30), clothHeight(30)
{
    // Initialize random seed once
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    camera = std::make_shared<engine::graphics::Camera>(45.0f, aspectRatio, 0.1f, 100.0f);
    camera->setPosition(glm::vec3(0.0f, 1.5f, 5.0f));
    camera->setTarget(glm::vec3(0.0f, 1.0f, 0.0f));

    clothShader = std::make_shared<engine::graphics::Shader>("shaders/cloth.vert", "shaders/cloth.frag");
    ballShader = std::make_shared<engine::graphics::Shader>("shaders/ball.vert", "shaders/ball.frag");
    clothTexture = std::make_shared<engine::graphics::Texture2D>("shaders/cloth_weave.png");

    physicsWorld = std::make_shared<engine::physics::PhysicsWorld3D>();

    // // Material properties
    // clothMaterial.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    // clothMaterial.diffuse = glm::vec3(0.6f, 0.5f, 0.4f);
    // clothMaterial.specular = glm::vec3(0.2f, 0.2f, 0.2f);
    // clothMaterial.shininess = 32.0f;
    // Material properties for a more attractive cloth
    clothMaterial.ambient = glm::vec3(0.2f, 0.2f, 0.2f);        // Darker ambient for depth
    clothMaterial.diffuse = glm::vec3(0.3f, 0.5f, 0.8f);        // Blue-ish color (or try other colors)
    clothMaterial.specular = glm::vec3(0.5f, 0.5f, 0.5f);       // More prominent highlights
    clothMaterial.shininess = 16.0f;                            // Less concentrated highlights (was 32.0f)

    // // Light properties
    // sceneLight.position = glm::vec3(5.0f, 5.0f, 5.0f);
    // sceneLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    // sceneLight.diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    // sceneLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);


        // Light properties for better illumination
    sceneLight.position = glm::vec3(2.0f, 5.0f, 3.0f);          // Reposition light for better shadows
    sceneLight.ambient = glm::vec3(0.2f, 0.2f, 0.3f);           // Slightly blue-tinted ambient
    sceneLight.diffuse = glm::vec3(1.0f, 0.9f, 0.8f);           // Warm-tinted main light
    sceneLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);          // Keep bright highlights
    // Cloth setup
    auto cloth = std::make_shared<engine::physics::ClothSolver3D>(
        *physicsWorld,
        clothWidth, clothHeight,
        0.1f,
        0.3f,    // Structural stiffness
        0.4f,    // Shear stiffness
        0.3f     // Bend stiffness
    );

    cloth->createCloth(
        glm::vec3(-clothWidth * 0.05f, 3.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)
    );
    physicsWorld->addCloth(cloth);

    // Optional ball (you can remove this if you only want cloth)
    auto ball = std::make_shared<engine::objects::Ball3D>(glm::vec3(0.0f, 1.5f, 0.0f), 0.3f);
    ball->applyForce(glm::vec3(0.0f, -9.81f, 0.0f));
    physicsWorld->addBall(ball);

    // Cloth mesh
    clothMesh = std::make_shared<engine::graphics::Mesh3D>();
    std::vector<engine::graphics::Vertex3D> vertices;
    std::vector<unsigned int> indices;
    engine::graphics::MeshGenerator3D::generateClothMesh(clothWidth, clothHeight, cloth->getParticles(), vertices, indices);
    clothMesh->uploadData(vertices, indices);
}

void Scene3D::update(float dt) {
    physicsWorld->update(dt);

    if (!physicsWorld->getCloths().empty()) {
        auto cloth = physicsWorld->getCloths()[0];
        const auto& particles = cloth->getParticles();

        // Wind Force (Procedural)
        float windStrength = 1.0f;
        glm::vec3 windDir = glm::normalize(glm::vec3(
            0.5f * std::sin(glfwGetTime()),
            0.0f,
            0.5f * std::cos(glfwGetTime())
        ));

        // Flutter randomness
        float flutterStrength = 0.05f;

        for (auto& particle : particles) {
            if (!particle->isPinned()) {
                particle->applyForce(windDir * windStrength);

                glm::vec3 randomFlutter = glm::vec3(
                    (float(std::rand() % 1000) / 500.0f - 1.0f),
                    (float(std::rand() % 1000) / 500.0f - 1.0f),
                    (float(std::rand() % 1000) / 500.0f - 1.0f)
                );
                particle->applyForce(randomFlutter * flutterStrength);
            }
        }

        // Update vertices
        std::vector<engine::graphics::Vertex3D> updatedVertices(particles.size());

        for (size_t i = 0; i < particles.size(); ++i) {
            updatedVertices[i].position = particles[i]->getPosition();
            updatedVertices[i].normal = glm::vec3(0.0f);
            updatedVertices[i].texCoord = glm::vec2(
                (i % (clothWidth + 1)) / float(clothWidth),
                (i / (clothWidth + 1)) / float(clothHeight)
            );
        }

        int w = clothWidth + 1;
        int h = clothHeight + 1;

        for (int y = 0; y < h - 1; ++y) {
            for (int x = 0; x < w - 1; ++x) {
                int i0 = y * w + x;
                int i1 = i0 + 1;
                int i2 = i0 + w;
                int i3 = i2 + 1;

                glm::vec3 normal1 = glm::normalize(glm::cross(
                    updatedVertices[i2].position - updatedVertices[i0].position,
                    updatedVertices[i1].position - updatedVertices[i0].position
                ));
                glm::vec3 normal2 = glm::normalize(glm::cross(
                    updatedVertices[i3].position - updatedVertices[i1].position,
                    updatedVertices[i2].position - updatedVertices[i1].position
                ));

                updatedVertices[i0].normal += normal1;
                updatedVertices[i2].normal += normal1;
                updatedVertices[i1].normal += normal1;

                updatedVertices[i1].normal += normal2;
                updatedVertices[i2].normal += normal2;
                updatedVertices[i3].normal += normal2;
            }
        }

        for (auto& v : updatedVertices) {
            if (glm::length(v.normal) > 1e-6f)
                v.normal = glm::normalize(v.normal);
            else
                v.normal = glm::vec3(0.0f, 1.0f, 0.0f); // fallback
        }

        clothMesh->updateVertices(updatedVertices);
    }
}

void Scene3D::render() const {
    if (clothShader) {
        clothTexture->bind();
        clothShader->bind();

        clothShader->setUniformMat4("model", glm::mat4(1.0f));
        clothShader->setUniformMat4("view", camera->getViewMatrix());
        clothShader->setUniformMat4("projection", camera->getProjectionMatrix());

        clothShader->setUniformVec3("light.position", sceneLight.position);
        clothShader->setUniformVec3("light.ambient", sceneLight.ambient);
        clothShader->setUniformVec3("light.diffuse", sceneLight.diffuse);
        clothShader->setUniformVec3("light.specular", sceneLight.specular);

        clothShader->setUniformVec3("viewPos", camera->getPosition());

        clothShader->setUniformVec3("material.ambient", clothMaterial.ambient);
        clothShader->setUniformVec3("material.diffuse", clothMaterial.diffuse);
        clothShader->setUniformVec3("material.specular", clothMaterial.specular);
        clothShader->setUniformFloat("material.shininess", clothMaterial.shininess);

        clothShader->setUniformInt("clothTexture", 0);

        clothMesh->draw();

        clothShader->unbind();
        clothTexture->unbind();
    }
}

std::shared_ptr<engine::physics::PhysicsWorld3D> Scene3D::getPhysicsWorld() {
    return physicsWorld;
}

std::shared_ptr<engine::graphics::Camera> Scene3D::getCamera() {
    return camera;
}

} // namespace
