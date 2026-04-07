#include "ContactManifold.hpp"
#include "../RigidBody.hpp"
#include "../../core/Logger.hpp"
#include <algorithm>
#include <cmath>

namespace engine::physics {

ContactManifold::ContactManifold() 
    : bodyA(nullptr), bodyB(nullptr), normal(0, 1, 0), 
      friction(0.5f), restitution(0.3f), solverDataValid(false) {
    contacts.reserve(4);  // Maximum 4 contacts for stability
}

ContactManifold::ContactManifold(RigidBody* a, RigidBody* b)
    : bodyA(a), bodyB(b), normal(0, 1, 0), 
      friction(0.5f), restitution(0.3f), solverDataValid(false) {
    contacts.reserve(4);
    if (a && b) {
        calculateMaterialProperties();
    }
}

void ContactManifold::setBodies(RigidBody* a, RigidBody* b) {
    bodyA = a;
    bodyB = b;
    if (a && b) {
        calculateMaterialProperties();
    }
    solverDataValid = false;
}

void ContactManifold::addContact(const ContactPoint& contact) {
    // Check for duplicate contacts
    const float duplicateThreshold = 0.01f;
    for (const auto& existing : contacts) {
        float distance = glm::length(contact.worldPointA - existing.worldPointA);
        if (distance < duplicateThreshold) {
            return; // Too close to existing contact
        }
    }
    
    if (contacts.size() < 4) {
        contacts.push_back(contact);
    } else {
        // Replace the contact with least penetration
        auto it = std::min_element(contacts.begin(), contacts.end(),
            [](const ContactPoint& a, const ContactPoint& b) {
                return a.penetrationDepth < b.penetrationDepth;
            });
        
        if (contact.penetrationDepth > it->penetrationDepth) {
            *it = contact;
        }
    }
    
    solverDataValid = false;
}

void ContactManifold::addContact(const glm::vec3& worldPointA, const glm::vec3& worldPointB, 
                                float penetration) {
    ContactPoint contact;
    contact.worldPointA = worldPointA;
    contact.worldPointB = worldPointB;
    contact.penetrationDepth = penetration;
    
    // Calculate local points if bodies are valid
    if (bodyA && bodyB) {
        contact.localPointA = bodyA->worldToLocal(worldPointA);
        contact.localPointB = bodyB->worldToLocal(worldPointB);
    }
    
    addContact(contact);
}

void ContactManifold::removeDeepestContact() {
    if (contacts.empty()) return;
    
    auto it = std::max_element(contacts.begin(), contacts.end(),
        [](const ContactPoint& a, const ContactPoint& b) {
            return a.penetrationDepth < b.penetrationDepth;
        });
    
    contacts.erase(it);
    solverDataValid = false;
}

void ContactManifold::removeDuplicateContacts(float tolerance) {
    for (auto it1 = contacts.begin(); it1 != contacts.end(); ++it1) {
        for (auto it2 = it1 + 1; it2 != contacts.end();) {
            float distance = glm::length(it1->worldPointA - it2->worldPointA);
            if (distance < tolerance) {
                // Keep the contact with greater penetration
                if (it1->penetrationDepth < it2->penetrationDepth) {
                    *it1 = *it2;
                }
                it2 = contacts.erase(it2);
            } else {
                ++it2;
            }
        }
    }
    solverDataValid = false;
}

void ContactManifold::prepare(float dt) {
    if (!bodyA || !bodyB || contacts.empty()) return;
    
    // Calculate tangent vectors for friction
    if (std::abs(normal.x) >= 0.57735f) {
        tangent1 = glm::vec3(normal.y, -normal.x, 0.0f);
    } else {
        tangent1 = glm::vec3(0.0f, normal.z, -normal.y);
    }
    tangent1 = glm::normalize(tangent1);
    tangent2 = glm::cross(normal, tangent1);
    
    // Update local points to world space
    for (auto& contact : contacts) {
        if (bodyA) contact.worldPointA = bodyA->localToWorld(contact.localPointA);
        if (bodyB) contact.worldPointB = bodyB->localToWorld(contact.localPointB);
    }
    
    solverDataValid = true;
}

void ContactManifold::warmStart() {
    if (!solverDataValid || !bodyA || !bodyB) return;
    
    // Apply cached impulses for solver stability
    for (auto& contact : contacts) {
        // Apply normal impulse
        glm::vec3 normalImpulse = normal * contact.normalImpulse;
        glm::vec3 frictionImpulse = tangent1 * contact.tangentImpulse1 + 
                                   tangent2 * contact.tangentImpulse2;
        glm::vec3 totalImpulse = normalImpulse + frictionImpulse;
        
        if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
            bodyA->applyImpulseAtPoint(-totalImpulse, contact.worldPointA);
        }
        if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
            bodyB->applyImpulseAtPoint(totalImpulse, contact.worldPointB);
        }
    }
}

void ContactManifold::solve(float dt) {
    if (!solverDataValid || !bodyA || !bodyB || contacts.empty()) return;
    
    const float baumgarte = 0.2f;  // Position correction factor
    const float slop = 0.005f;     // Allowable penetration
    
    for (auto& contact : contacts) {
        // Calculate relative velocity at contact point
        glm::vec3 velA = bodyA->getVelocityAtPoint(contact.worldPointA);
        glm::vec3 velB = bodyB->getVelocityAtPoint(contact.worldPointB);
        glm::vec3 relativeVelocity = velB - velA;
        
        // Normal constraint
        float velocityAlongNormal = glm::dot(relativeVelocity, normal);
        
        // Calculate bias for position correction
        float bias = 0.0f;
        if (contact.penetrationDepth > slop) {
            bias = (baumgarte / dt) * (contact.penetrationDepth - slop);
        }
        
        // Calculate impulse
        float impulseNumerator = -(velocityAlongNormal + bias);
        float impulseDenominator = bodyA->getInverseMass() + bodyB->getInverseMass();
        
        // Add angular contribution
        glm::vec3 rA = contact.worldPointA - bodyA->getPosition();
        glm::vec3 rB = contact.worldPointB - bodyB->getPosition();
        
        glm::vec3 rAcrossN = glm::cross(rA, normal);
        glm::vec3 rBcrossN = glm::cross(rB, normal);
        
        impulseDenominator += glm::dot(rAcrossN, bodyA->getInverseInertiaTensor() * rAcrossN);
        impulseDenominator += glm::dot(rBcrossN, bodyB->getInverseInertiaTensor() * rBcrossN);
        
        float normalImpulse = impulseNumerator / impulseDenominator;
        
        // Clamp accumulated impulse
        float oldImpulse = contact.normalImpulse;
        contact.normalImpulse = glm::max(0.0f, oldImpulse + normalImpulse);
        normalImpulse = contact.normalImpulse - oldImpulse;
        
        // Apply normal impulse
        glm::vec3 impulse = normalImpulse * normal;
        if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
            bodyA->applyImpulseAtPoint(-impulse, contact.worldPointA);
        }
        if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
            bodyB->applyImpulseAtPoint(impulse, contact.worldPointB);
        }
        
        // Friction constraints
        if (friction > 0.0f) {
            // Recalculate relative velocity after normal impulse
            velA = bodyA->getVelocityAtPoint(contact.worldPointA);
            velB = bodyB->getVelocityAtPoint(contact.worldPointB);
            relativeVelocity = velB - velA;
            
            // Tangent 1
            float tangentVel1 = glm::dot(relativeVelocity, tangent1);
            float tangentImpulse1 = -tangentVel1 / impulseDenominator;
            
            float maxFriction = friction * contact.normalImpulse;
            float oldTangentImpulse1 = contact.tangentImpulse1;
            contact.tangentImpulse1 = glm::clamp(oldTangentImpulse1 + tangentImpulse1, 
                                                -maxFriction, maxFriction);
            tangentImpulse1 = contact.tangentImpulse1 - oldTangentImpulse1;
            
            // Apply tangent impulse 1
            impulse = tangentImpulse1 * tangent1;
            if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
                bodyA->applyImpulseAtPoint(-impulse, contact.worldPointA);
            }
            if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
                bodyB->applyImpulseAtPoint(impulse, contact.worldPointB);
            }
            
            // Tangent 2
            float tangentVel2 = glm::dot(relativeVelocity, tangent2);
            float tangentImpulse2 = -tangentVel2 / impulseDenominator;
            
            float oldTangentImpulse2 = contact.tangentImpulse2;
            contact.tangentImpulse2 = glm::clamp(oldTangentImpulse2 + tangentImpulse2, 
                                                -maxFriction, maxFriction);
            tangentImpulse2 = contact.tangentImpulse2 - oldTangentImpulse2;
            
            // Apply tangent impulse 2
            impulse = tangentImpulse2 * tangent2;
            if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
                bodyA->applyImpulseAtPoint(-impulse, contact.worldPointA);
            }
            if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
                bodyB->applyImpulseAtPoint(impulse, contact.worldPointB);
            }
        }
    }
}

void ContactManifold::postSolve() {
    // Clean up any invalid contacts
    contacts.erase(
        std::remove_if(contacts.begin(), contacts.end(),
            [](const ContactPoint& contact) {
                return contact.penetrationDepth < -0.1f; // Too separated
            }),
        contacts.end()
    );
}

float ContactManifold::getTotalContactArea() const {
    // Simplified area calculation
    return static_cast<float>(contacts.size()) * 0.01f; // Assume each contact is 1cm²
}

glm::vec3 ContactManifold::getCenterOfContacts() const {
    if (contacts.empty()) return glm::vec3(0.0f);
    
    glm::vec3 center(0.0f);
    for (const auto& contact : contacts) {
        center += contact.worldPointA;
    }
    return center / static_cast<float>(contacts.size());
}

float ContactManifold::getAverageDepth() const {
    if (contacts.empty()) return 0.0f;
    
    float totalDepth = 0.0f;
    for (const auto& contact : contacts) {
        totalDepth += contact.penetrationDepth;
    }
    return totalDepth / static_cast<float>(contacts.size());
}

bool ContactManifold::isValid() const {
    return bodyA && bodyB && !contacts.empty() && 
           glm::length(normal) > 0.9f && glm::length(normal) < 1.1f;
}

void ContactManifold::calculateMaterialProperties() {
    if (!bodyA || !bodyB) return;
    
    // For now, use default values - will be enhanced with PhysicsMaterial system later
    float frictionA = 0.5f;  // Default friction for bodyA
    float frictionB = 0.5f;  // Default friction for bodyB
    float restitutionA = 0.3f;  // Default restitution for bodyA
    float restitutionB = 0.3f;  // Default restitution for bodyB
    
    friction = combineFriction(frictionA, frictionB);
    restitution = combineRestitution(restitutionA, restitutionB);
}

float ContactManifold::combineFriction(float frictionA, float frictionB) const {
    // Geometric mean for friction combination
    return std::sqrt(frictionA * frictionB);
}

float ContactManifold::combineRestitution(float restitutionA, float restitutionB) const {
    // Maximum restitution (more bouncy)
    return glm::max(restitutionA, restitutionB);
}

} // namespace engine::physics