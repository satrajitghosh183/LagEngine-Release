#include "RandomUtils.hpp"
#include <glm/gtc/quaternion.hpp>

namespace GameEngine {
namespace Random {

    RandomGenerator Global;

    RandomGenerator::RandomGenerator()
        : m_Engine(std::random_device{}())
        , m_Distribution(0.0f, 1.0f) {
    }

    RandomGenerator::RandomGenerator(uint32_t seed)
        : m_Engine(seed)
        , m_Distribution(0.0f, 1.0f) {
    }

    void RandomGenerator::SetSeed(uint32_t seed) {
        m_Engine.seed(seed);
    }

    float RandomGenerator::Float() {
        return m_Distribution(m_Engine);
    }

    float RandomGenerator::Range(float min, float max) {
        return min + (max - min) * Float();
    }

    int RandomGenerator::Int(int min, int max) {
        std::uniform_int_distribution<int> dist(min, max);
        return dist(m_Engine);
    }

    bool RandomGenerator::Bool() {
        return Float() > 0.5f;
    }

    glm::vec2 RandomGenerator::InsideUnitCircle() {
        float angle = Range(0.0f, glm::two_pi<float>());
        float radius = sqrt(Float());
        return glm::vec2(cos(angle), sin(angle)) * radius;
    }

    glm::vec2 RandomGenerator::OnUnitCircle() {
        float angle = Range(0.0f, glm::two_pi<float>());
        return glm::vec2(cos(angle), sin(angle));
    }

    glm::vec3 RandomGenerator::InsideUnitSphere() {
        // Marsaglia method
        float x, y, z, length2;
        do {
            x = Range(-1.0f, 1.0f);
            y = Range(-1.0f, 1.0f);
            z = Range(-1.0f, 1.0f);
            length2 = x * x + y * y + z * z;
        } while (length2 >= 1.0f);
        
        return glm::vec3(x, y, z);
    }

    glm::vec3 RandomGenerator::OnUnitSphere() {
        return glm::normalize(InsideUnitSphere());
    }

    glm::vec3 RandomGenerator::Color() {
        return glm::vec3(Float(), Float(), Float());
    }

    glm::vec4 RandomGenerator::ColorWithAlpha() {
        return glm::vec4(Float(), Float(), Float(), Float());
    }

    glm::quat RandomGenerator::Rotation() {
        // Random quaternion using uniform distribution on 4D sphere
        glm::vec4 u = glm::vec4(Float(), Float(), Float(), Float());
        
        float u1 = u.x;
        float r1 = sqrt(1.0f - u1);
        float r2 = sqrt(u1);
        float t1 = glm::two_pi<float>() * u.y;
        float t2 = glm::two_pi<float>() * u.z;
        
        return glm::quat(
            r2 * cos(t2),
            r1 * sin(t1),
            r1 * cos(t1),
            r2 * sin(t2)
        );
    }

}}