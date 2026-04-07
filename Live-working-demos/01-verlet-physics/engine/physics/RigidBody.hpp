#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>

namespace engine::physics {

class CollisionShape;

/**
 * @brief Rigid body with 6DOF physics simulation
 * Supports linear and angular dynamics with proper mass properties
 */
class RigidBody {
public:
    enum class BodyType {
        Static,      // Never moves, infinite mass
        Kinematic,   // Moves but not affected by forces
        Dynamic      // Full physics simulation
    };

    /**
     * @brief Constructor
     * @param type Body type (static/kinematic/dynamic)
     * @param mass Mass in kg (ignored for static/kinematic)
     */
    RigidBody(BodyType type = BodyType::Dynamic, float mass = 1.0f);
    ~RigidBody() = default;

    // Integration
    void integrate(float dt);
    void clearForces();

    // Force application
    void applyForce(const glm::vec3& force);
    void applyForceAtPoint(const glm::vec3& force, const glm::vec3& worldPoint);
    void applyTorque(const glm::vec3& torque);
    void applyImpulse(const glm::vec3& impulse);
    void applyImpulseAtPoint(const glm::vec3& impulse, const glm::vec3& worldPoint);

    // Position and orientation
    const glm::vec3& getPosition() const { return position; }
    void setPosition(const glm::vec3& pos);
    
    const glm::quat& getOrientation() const { return orientation; }
    void setOrientation(const glm::quat& orient);
    
    glm::mat4 getTransform() const;

    // Velocity
    const glm::vec3& getLinearVelocity() const { return linearVelocity; }
    void setLinearVelocity(const glm::vec3& velocity);
    
    const glm::vec3& getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(const glm::vec3& velocity);

    // Mass properties
    float getMass() const { return mass; }
    float getInverseMass() const { return inverseMass; }
    void setMass(float newMass);
    
    const glm::mat3& getInertiaTensor() const { return inertiaTensor; }
    const glm::mat3& getInverseInertiaTensor() const { return inverseInertiaTensor; }
    void setInertiaTensor(const glm::mat3& tensor);

    // Body type
    BodyType getBodyType() const { return bodyType; }
    void setBodyType(BodyType type);

    // Physics properties
    float getLinearDamping() const { return linearDamping; }
    void setLinearDamping(float damping) { linearDamping = damping; }
    
    float getAngularDamping() const { return angularDamping; }
    void setAngularDamping(float damping) { angularDamping = damping; }

    // Collision shape
    void setCollisionShape(std::shared_ptr<CollisionShape> shape);
    std::shared_ptr<CollisionShape> getCollisionShape() const { return collisionShape; }

    // Utility
    glm::vec3 getVelocityAtPoint(const glm::vec3& worldPoint) const;
    glm::vec3 worldToLocal(const glm::vec3& worldPoint) const;
    glm::vec3 localToWorld(const glm::vec3& localPoint) const;

    // Sleep system (for performance)
    bool isSleeping() const { return sleeping; }
    void setSleeping(bool sleep);
    void wakeUp();

private:
    // Transform
    glm::vec3 position{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};

    // Linear motion
    glm::vec3 linearVelocity{0.0f};
    glm::vec3 force{0.0f};

    // Angular motion
    glm::vec3 angularVelocity{0.0f};
    glm::vec3 torque{0.0f};

    // Mass properties
    float mass = 1.0f;
    float inverseMass = 1.0f;
    glm::mat3 inertiaTensor{1.0f};
    glm::mat3 inverseInertiaTensor{1.0f};
    glm::mat3 worldInverseInertiaTensor{1.0f};

    // Body properties
    BodyType bodyType = BodyType::Dynamic;
    float linearDamping = 0.01f;
    float angularDamping = 0.05f;

    // Collision
    std::shared_ptr<CollisionShape> collisionShape;

    // Sleep system
    bool sleeping = false;
    float sleepTime = 0.0f;
    static constexpr float SLEEP_THRESHOLD = 2.0f;
    static constexpr float SLEEP_LINEAR_VELOCITY = 0.01f;
    static constexpr float SLEEP_ANGULAR_VELOCITY = 0.01f;

    // Helper methods
    void updateInertiaTensor();
    void updateSleepState(float dt);
    glm::mat3 calculateBoxInertia(const glm::vec3& size) const;
    glm::mat3 calculateSphereInertia(float radius) const;
};

} // namespace engine::physics