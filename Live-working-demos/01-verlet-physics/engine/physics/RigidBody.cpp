#include "RigidBody.hpp"
#include "CollisionShape.hpp"
#include "engine/core/Logger.hpp"
#include <glm/gtx/matrix_decompose.hpp>
#include <algorithm>

namespace engine::physics {

RigidBody::RigidBody(BodyType type, float bodyMass) 
    : bodyType(type) {
    setMass(bodyMass);
    
    // Initialize identity orientation
    orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    
    // Set appropriate properties based on body type
    switch (bodyType) {
        case BodyType::Static:
            setMass(0.0f);  // Infinite mass
            break;
        case BodyType::Kinematic:
            setMass(0.0f);  // Infinite mass
            break;
        case BodyType::Dynamic:
            setMass(bodyMass);
            break;
    }
}

void RigidBody::integrate(float dt) {
    if (bodyType != BodyType::Dynamic || sleeping) {
        clearForces();
        return;
    }

    // Linear integration using semi-implicit Euler
    // v = v + a * dt
    // p = p + v * dt
    glm::vec3 acceleration = force * inverseMass;
    linearVelocity += acceleration * dt;
    
    // Apply linear damping
    linearVelocity *= std::pow(1.0f - linearDamping, dt);
    
    // Update position
    position += linearVelocity * dt;

    // Angular integration
    // ω = ω + I^-1 * τ * dt
    // q = q + 0.5 * ω * q * dt
    glm::vec3 angularAcceleration = worldInverseInertiaTensor * torque;
    angularVelocity += angularAcceleration * dt;
    
    // Apply angular damping
    angularVelocity *= std::pow(1.0f - angularDamping, dt);
    
    // Update orientation using quaternion integration
    if (glm::length(angularVelocity) > 0.0001f) {
        glm::quat angularVelQuat(0.0f, angularVelocity.x, angularVelocity.y, angularVelocity.z);
        glm::quat deltaOrientation = 0.5f * angularVelQuat * orientation * dt;
        orientation += deltaOrientation;
        orientation = glm::normalize(orientation);
    }

    // Update world-space inverse inertia tensor
    updateInertiaTensor();

    // Update sleep state
    updateSleepState(dt);

    // Clear forces for next frame
    clearForces();
}

void RigidBody::clearForces() {
    force = glm::vec3(0.0f);
    torque = glm::vec3(0.0f);
}

void RigidBody::applyForce(const glm::vec3& f) {
    if (bodyType != BodyType::Dynamic) return;
    
    force += f;
    wakeUp();
}

void RigidBody::applyForceAtPoint(const glm::vec3& f, const glm::vec3& worldPoint) {
    if (bodyType != BodyType::Dynamic) return;
    
    applyForce(f);
    
    // Calculate torque: τ = r × F
    glm::vec3 r = worldPoint - position;
    applyTorque(glm::cross(r, f));
}

void RigidBody::applyTorque(const glm::vec3& t) {
    if (bodyType != BodyType::Dynamic) return;
    
    torque += t;
    wakeUp();
}

void RigidBody::applyImpulse(const glm::vec3& impulse) {
    if (bodyType != BodyType::Dynamic) return;
    
    linearVelocity += impulse * inverseMass;
    wakeUp();
}

void RigidBody::applyImpulseAtPoint(const glm::vec3& impulse, const glm::vec3& worldPoint) {
    if (bodyType != BodyType::Dynamic) return;
    
    applyImpulse(impulse);
    
    // Calculate angular impulse
    glm::vec3 r = worldPoint - position;
    glm::vec3 angularImpulse = glm::cross(r, impulse);
    angularVelocity += worldInverseInertiaTensor * angularImpulse;
    wakeUp();
}

void RigidBody::setPosition(const glm::vec3& pos) {
    position = pos;
    wakeUp();
}

void RigidBody::setOrientation(const glm::quat& orient) {
    orientation = glm::normalize(orient);
    updateInertiaTensor();
    wakeUp();
}

glm::mat4 RigidBody::getTransform() const {
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotation = glm::mat4_cast(orientation);
    return translation * rotation;
}

void RigidBody::setLinearVelocity(const glm::vec3& velocity) {
    if (bodyType != BodyType::Dynamic) return;
    linearVelocity = velocity;
    wakeUp();
}

void RigidBody::setAngularVelocity(const glm::vec3& velocity) {
    if (bodyType != BodyType::Dynamic) return;
    angularVelocity = velocity;
    wakeUp();
}

void RigidBody::setMass(float newMass) {
    if (newMass <= 0.0f) {
        // Infinite mass (static/kinematic)
        mass = 0.0f;
        inverseMass = 0.0f;
        inertiaTensor = glm::mat3(0.0f);
        inverseInertiaTensor = glm::mat3(0.0f);
        worldInverseInertiaTensor = glm::mat3(0.0f);
    } else {
        mass = newMass;
        inverseMass = 1.0f / newMass;
        
        // Set default inertia tensor for unit cube
        float i = (mass / 12.0f) * (2.0f); // For unit cube: (1^2 + 1^2)
        inertiaTensor = glm::mat3(
            i, 0, 0,
            0, i, 0,
            0, 0, i
        );
        inverseInertiaTensor = glm::inverse(inertiaTensor);
        updateInertiaTensor();
    }
}

void RigidBody::setInertiaTensor(const glm::mat3& tensor) {
    inertiaTensor = tensor;
    if (mass > 0.0f) {
        inverseInertiaTensor = glm::inverse(tensor);
        updateInertiaTensor();
    }
}

void RigidBody::setBodyType(BodyType type) {
    bodyType = type;
    
    switch (type) {
        case BodyType::Static:
        case BodyType::Kinematic:
            setMass(0.0f);
            linearVelocity = glm::vec3(0.0f);
            angularVelocity = glm::vec3(0.0f);
            break;
        case BodyType::Dynamic:
            if (mass <= 0.0f) {
                setMass(1.0f);  // Default mass
            }
            break;
    }
}

void RigidBody::setCollisionShape(std::shared_ptr<CollisionShape> shape) {
    collisionShape = shape;
    
    // Auto-calculate inertia tensor based on shape
    if (shape && mass > 0.0f) {
        // This will be implemented when we add specific shapes
        // For now, keep the default inertia tensor
    }
}

glm::vec3 RigidBody::getVelocityAtPoint(const glm::vec3& worldPoint) const {
    glm::vec3 r = worldPoint - position;
    return linearVelocity + glm::cross(angularVelocity, r);
}

glm::vec3 RigidBody::worldToLocal(const glm::vec3& worldPoint) const {
    glm::vec3 relative = worldPoint - position;
    return glm::inverse(orientation) * relative;
}

glm::vec3 RigidBody::localToWorld(const glm::vec3& localPoint) const {
    return position + orientation * localPoint;
}

void RigidBody::setSleeping(bool sleep) {
    sleeping = sleep;
    if (!sleep) {
        sleepTime = 0.0f;
    }
}

void RigidBody::wakeUp() {
    setSleeping(false);
}

void RigidBody::updateInertiaTensor() {
    if (mass <= 0.0f) {
        worldInverseInertiaTensor = glm::mat3(0.0f);
        return;
    }

    // Transform inertia tensor to world space: I_world = R * I_body * R^T
    glm::mat3 rotationMatrix = glm::mat3_cast(orientation);
    glm::mat3 worldInertiaTensor = rotationMatrix * inertiaTensor * glm::transpose(rotationMatrix);
    worldInverseInertiaTensor = glm::inverse(worldInertiaTensor);
}

void RigidBody::updateSleepState(float dt) {
    if (bodyType != BodyType::Dynamic) return;

    float linearKE = 0.5f * mass * glm::dot(linearVelocity, linearVelocity);
    float angularKE = 0.5f * glm::dot(angularVelocity, inertiaTensor * angularVelocity);
    
    bool canSleep = (glm::length(linearVelocity) < SLEEP_LINEAR_VELOCITY) && 
                    (glm::length(angularVelocity) < SLEEP_ANGULAR_VELOCITY);

    if (canSleep) {
        sleepTime += dt;
        if (sleepTime > SLEEP_THRESHOLD) {
            setSleeping(true);
            linearVelocity = glm::vec3(0.0f);
            angularVelocity = glm::vec3(0.0f);
        }
    } else {
        sleepTime = 0.0f;
    }
}

glm::mat3 RigidBody::calculateBoxInertia(const glm::vec3& size) const {
    float x2 = size.x * size.x;
    float y2 = size.y * size.y;
    float z2 = size.z * size.z;
    float factor = mass / 12.0f;
    
    return glm::mat3(
        factor * (y2 + z2), 0, 0,
        0, factor * (x2 + z2), 0,
        0, 0, factor * (x2 + y2)
    );
}

glm::mat3 RigidBody::calculateSphereInertia(float radius) const {
    float i = (2.0f / 5.0f) * mass * radius * radius;
    return glm::mat3(
        i, 0, 0,
        0, i, 0,
        0, 0, i
    );
}

} // namespace engine::physics