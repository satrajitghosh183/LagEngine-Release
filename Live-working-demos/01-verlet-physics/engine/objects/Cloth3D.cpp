#include "engine/objects/Cloth3D.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

namespace engine::objects {

Cloth3D::Cloth3D(int w, int h, float s, const glm::vec3& origin, scene::SceneManager3D& sceneManager)
    : width(w), height(h), spacing(s), origin(origin), sceneManagerRef(sceneManager)
{
    createParticles();
    createSprings();
    createMesh();
}

void Cloth3D::createParticles() {
    auto physicsWorld = sceneManagerRef.get().getCurrentScene()->getPhysicsWorld();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            glm::vec3 pos = origin + glm::vec3(i * spacing, 0.0f, j * spacing);
            auto particle = std::make_shared<physics::Particle3D>(pos, 1.0f, false);

            if (j == 0 && (i % 4 == 0 || i == 0 || i == width - 1)) {
                particle->setPinned(true);
            }

            particles.push_back(particle);
            physicsWorld->addParticle(particle);
        }
    }
}

void Cloth3D::createSprings() {
    auto physicsWorld = sceneManagerRef.get().getCurrentScene()->getPhysicsWorld();

    auto getIndex = [this](int x, int y) { return y * width + x; };
    const float diagSpacing = spacing * glm::root_two<float>();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            int idx = getIndex(i, j);

            if (i < width - 1) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i + 1, j)], spacing
                ));
            }
            if (j < height - 1) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i, j + 1)], spacing
                ));
            }
            if (i < width - 1 && j < height - 1) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i + 1, j + 1)], diagSpacing
                ));
            }
            if (i > 0 && j < height - 1) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i - 1, j + 1)], diagSpacing
                ));
            }
            if (i < width - 2) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i + 2, j)], spacing * 2.0f
                ));
            }
            if (j < height - 2) {
                physicsWorld->addSpring(std::make_shared<physics::Spring3D>(
                    particles[idx], particles[getIndex(i, j + 2)], spacing * 2.0f
                ));
            }
        }
    }
}

void Cloth3D::createMesh() {
    updateMesh();
    mesh.upload(true); // Use dynamic buffers
}

void Cloth3D::update(float /*dt*/) {
    updateMesh();
    mesh.updateVertices();
    mesh.updateNormals();
}

void Cloth3D::render(const engine::graphics::Shader& shader) {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    shader.setMat4("model", glm::mat4(1.0f));
    shader.setVec3("objectColor", glm::vec3(0.8f, 0.7f, 0.6f));
    shader.setFloat("roughness", 0.7f);
    shader.setFloat("metallic", 0.0f);

    mesh.draw(GL_TRIANGLES);

    glDisable(GL_CULL_FACE);
}

void Cloth3D::updateMesh() {
    mesh.vertices.clear();
    mesh.normals.clear();
    mesh.indices.clear();

    for (auto& p : particles) {
        mesh.vertices.push_back(p->getPosition());
    }

    for (int j = 0; j < height - 1; ++j) {
        for (int i = 0; i < width - 1; ++i) {
            int i0 = j * width + i;
            int i1 = j * width + (i + 1);
            int i2 = (j + 1) * width + i;
            int i3 = (j + 1) * width + (i + 1);

            // First triangle
            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            // Second triangle
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    mesh.normals.resize(mesh.vertices.size(), glm::vec3(0.0f));

    for (size_t k = 0; k < mesh.indices.size(); k += 3) {
        const glm::vec3& v0 = mesh.vertices[mesh.indices[k]];
        const glm::vec3& v1 = mesh.vertices[mesh.indices[k + 1]];
        const glm::vec3& v2 = mesh.vertices[mesh.indices[k + 2]];

        glm::vec3 n = glm::normalize(glm::cross(v1 - v0, v2 - v0));
        mesh.normals[mesh.indices[k]]     += n;
        mesh.normals[mesh.indices[k + 1]] += n;
        mesh.normals[mesh.indices[k + 2]] += n;
    }

    for (auto& n : mesh.normals) {
        float len = glm::length(n);
        if (len > 1e-4f) n /= len;
        else             n = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

} // namespace engine::objects
