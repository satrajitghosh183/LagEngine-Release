#include "Scene3D.hpp"
#include "../graphics/MeshGenerator3D.hpp"

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace engine::scene {

Scene3D::Scene3D(float aspectRatio)
    : clothWidth(30), clothHeight(30)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    camera = std::make_shared<engine::graphics::Camera>(45.0f, aspectRatio, 0.1f, 100.0f);
    camera->setPosition(glm::vec3(0.0f, 1.5f, 5.0f));
    camera->setTarget(glm::vec3(0.0f, 1.0f, 0.0f));

    physicsWorld = std::make_shared<engine::physics::PhysicsWorld3D>();

    clothMaterial.ambient = glm::vec3(0.2f, 0.2f, 0.2f);
    clothMaterial.diffuse = glm::vec3(0.3f, 0.5f, 0.8f);
    clothMaterial.specular = glm::vec3(0.5f, 0.5f, 0.5f);
    clothMaterial.shininess = 16.0f;

    sceneLight.position = glm::vec3(2.0f, 5.0f, 3.0f);
    sceneLight.ambient = glm::vec3(0.2f, 0.2f, 0.3f);
    sceneLight.diffuse = glm::vec3(1.0f, 0.9f, 0.8f);
    sceneLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);

    // Cloth setup
    auto cloth = std::make_shared<engine::physics::ClothSolver3D>(
        *physicsWorld,
        clothWidth, clothHeight,
        0.1f,
        0.3f, 0.4f, 0.3f
    );

    cloth->createCloth(
        glm::vec3(-clothWidth * 0.05f, 3.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -1.0f, 0.0f)
    );
    physicsWorld->addCloth(cloth);

    auto ball = std::make_shared<engine::objects::Ball3D>(glm::vec3(0.0f, 1.5f, 0.0f), 0.3f);
    ball->applyForce(glm::vec3(0.0f, -9.81f, 0.0f));
    physicsWorld->addBall(ball);

    // Build initial cloth mesh data
    {
        const auto& particles = cloth->getParticles();
        std::vector<engine::graphics::Vertex3D> verts;
        std::vector<unsigned int> indices;
        engine::graphics::MeshGenerator3D::generateClothMesh(clothWidth, clothHeight, particles, verts, indices);

        clothVertices.resize(verts.size());
        for (size_t i = 0; i < verts.size(); ++i) {
            clothVertices[i].position = verts[i].position;
            clothVertices[i].normal = verts[i].normal;
        }
        clothIndices = indices;
    }
}

void Scene3D::update(float dt) {
    physicsWorld->update(dt);

    if (!physicsWorld->getCloths().empty()) {
        auto cloth = physicsWorld->getCloths()[0];
        const auto& particles = cloth->getParticles();

        // Wind force
        float windStrength = 1.0f;
        double t = glfwGetTime();
        glm::vec3 windDir = glm::normalize(glm::vec3(
            0.5f * std::sin((float)t),
            0.0f,
            0.5f * std::cos((float)t)
        ));

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

        // Update vertex data
        clothVertices.resize(particles.size());
        for (size_t i = 0; i < particles.size(); ++i) {
            clothVertices[i].position = particles[i]->getPosition();
            clothVertices[i].normal = glm::vec3(0.0f);
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
                    clothVertices[i2].position - clothVertices[i0].position,
                    clothVertices[i1].position - clothVertices[i0].position
                ));
                glm::vec3 normal2 = glm::normalize(glm::cross(
                    clothVertices[i3].position - clothVertices[i1].position,
                    clothVertices[i2].position - clothVertices[i1].position
                ));

                clothVertices[i0].normal += normal1;
                clothVertices[i2].normal += normal1;
                clothVertices[i1].normal += normal1;

                clothVertices[i1].normal += normal2;
                clothVertices[i2].normal += normal2;
                clothVertices[i3].normal += normal2;
            }
        }

        for (auto& v : clothVertices) {
            if (glm::length(v.normal) > 1e-6f)
                v.normal = glm::normalize(v.normal);
            else
                v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
}

std::shared_ptr<engine::physics::PhysicsWorld3D> Scene3D::getPhysicsWorld() {
    return physicsWorld;
}

std::shared_ptr<engine::graphics::Camera> Scene3D::getCamera() {
    return camera;
}

} // namespace engine::scene
