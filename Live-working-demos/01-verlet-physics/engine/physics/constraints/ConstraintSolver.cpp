#include "ConstraintSolver.hpp"
#include "../RigidBody.hpp"
#include <algorithm>
#include <chrono>

namespace engine::physics {

ConstraintSolver::ConstraintSolver() {
    solverContacts.reserve(1000);
}

void ConstraintSolver::solve(std::vector<ContactManifold>& contacts,
                            std::vector<std::shared_ptr<Constraint>>& constraints,
                            float dt,
                            int velIterations,
                            int posIterations) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    velocityIterations = velIterations;
    positionIterations = posIterations;
    
    // Setup contact constraints
    setupContacts(contacts, dt);
    
    // Prepare joint constraints
    for (auto& constraint : constraints) {
        if (constraint && constraint->isEnabled() && !constraint->isBroken()) {
            constraint->prepare(dt);
        }
    }
    
    // Warm start
    warmStartContacts();
    for (auto& constraint : constraints) {
        if (constraint && constraint->isEnabled() && !constraint->isBroken()) {
            constraint->warmStart();
        }
    }
    
    // Velocity iterations
    for (int i = 0; i < velocityIterations; ++i) {
        solveVelocityConstraints(constraints);
    }
    
    // Position iterations
    for (int i = 0; i < positionIterations; ++i) {
        solvePositionConstraints(contacts);
    }
    
    // Store impulses for next frame
    storeContactImpulses(contacts);
    for (auto& constraint : constraints) {
        if (constraint && constraint->isEnabled() && !constraint->isBroken()) {
            constraint->storeImpulses();
        }
    }
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    stats.solveTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    updateStatistics(contacts, constraints);
}

void ConstraintSolver::setupContacts(std::vector<ContactManifold>& contacts, float dt) {
    solverContacts.clear();
    solverContacts.reserve(contacts.size());
    
    for (auto& manifold : contacts) {
        if (!manifold.hasContacts()) continue;
        
        SolverContact solverContact;
        solverContact.manifold = &manifold;
        solverContact.normal = manifold.getNormal();
        solverContact.friction = manifold.getFriction();
        solverContact.restitution = manifold.getRestitution();
        
        // Calculate tangent vectors
        calculateContactTangents(solverContact.normal, 
                               solverContact.tangent1, 
                               solverContact.tangent2);
        
        // Setup contact points
        const auto& contactPoints = manifold.getContacts();
        solverContact.points.reserve(contactPoints.size());
        
        for (const auto& point : contactPoints) {
            SolverContact::ContactData contactData;
            
            // Calculate relative positions
            RigidBody* bodyA = manifold.getBodyA();
            RigidBody* bodyB = manifold.getBodyB();
            
            contactData.rA = point.worldPointA - bodyA->getPosition();
            contactData.rB = point.worldPointB - bodyB->getPosition();
            
            // Calculate effective masses
            float invMassA = bodyA->getInverseMass();
            float invMassB = bodyB->getInverseMass();
            
            // Normal mass
            glm::vec3 rnA = glm::cross(contactData.rA, solverContact.normal);
            glm::vec3 rnB = glm::cross(contactData.rB, solverContact.normal);
            
            contactData.normalMass = invMassA + invMassB;
            contactData.normalMass += glm::dot(rnA, bodyA->getInverseInertiaTensor() * rnA);
            contactData.normalMass += glm::dot(rnB, bodyB->getInverseInertiaTensor() * rnB);
            contactData.normalMass = contactData.normalMass > 0.0f ? 1.0f / contactData.normalMass : 0.0f;
            
            // Tangent masses
            glm::vec3 rt1A = glm::cross(contactData.rA, solverContact.tangent1);
            glm::vec3 rt1B = glm::cross(contactData.rB, solverContact.tangent1);
            
            contactData.tangentMass1 = invMassA + invMassB;
            contactData.tangentMass1 += glm::dot(rt1A, bodyA->getInverseInertiaTensor() * rt1A);
            contactData.tangentMass1 += glm::dot(rt1B, bodyB->getInverseInertiaTensor() * rt1B);
            contactData.tangentMass1 = contactData.tangentMass1 > 0.0f ? 1.0f / contactData.tangentMass1 : 0.0f;
            
            glm::vec3 rt2A = glm::cross(contactData.rA, solverContact.tangent2);
            glm::vec3 rt2B = glm::cross(contactData.rB, solverContact.tangent2);
            
            contactData.tangentMass2 = invMassA + invMassB;
            contactData.tangentMass2 += glm::dot(rt2A, bodyA->getInverseInertiaTensor() * rt2A);
            contactData.tangentMass2 += glm::dot(rt2B, bodyB->getInverseInertiaTensor() * rt2B);
            contactData.tangentMass2 = contactData.tangentMass2 > 0.0f ? 1.0f / contactData.tangentMass2 : 0.0f;
            
            // Calculate bias for position correction
            float baumgarte = 0.2f;
            float slop = 0.005f;
            contactData.bias = 0.0f;
            
            if (point.penetrationDepth > slop) {
                contactData.bias = (baumgarte / dt) * (point.penetrationDepth - slop);
            }
            
            // Calculate velocity bias for restitution
            glm::vec3 velA = bodyA->getVelocityAtPoint(point.worldPointA);
            glm::vec3 velB = bodyB->getVelocityAtPoint(point.worldPointB);
            float relativeVelocity = glm::dot(velB - velA, solverContact.normal);
            
            if (relativeVelocity < -1.0f) { // Only apply restitution for significant impact
                contactData.velocityBias = -solverContact.restitution * relativeVelocity;
            } else {
                contactData.velocityBias = 0.0f;
            }
            
            // Initialize accumulated impulses
            contactData.normalImpulse = point.normalImpulse;
            contactData.tangentImpulse1 = point.tangentImpulse1;
            contactData.tangentImpulse2 = point.tangentImpulse2;
            
            solverContact.points.push_back(contactData);
        }
        
        solverContacts.push_back(solverContact);
    }
}

void ConstraintSolver::warmStartContacts() {
    for (auto& contact : solverContacts) {
        RigidBody* bodyA = contact.manifold->getBodyA();
        RigidBody* bodyB = contact.manifold->getBodyB();
        
        for (auto& point : contact.points) {
            // Apply normal impulse
            glm::vec3 impulse = contact.normal * point.normalImpulse;
            impulse += contact.tangent1 * point.tangentImpulse1;
            impulse += contact.tangent2 * point.tangentImpulse2;
            
            if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyA->setLinearVelocity(bodyA->getLinearVelocity() - impulse * bodyA->getInverseMass());
               bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                        bodyA->getInverseInertiaTensor() * glm::cross(point.rA, impulse));
           }
           
           if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyB->setLinearVelocity(bodyB->getLinearVelocity() + impulse * bodyB->getInverseMass());
               bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                        bodyB->getInverseInertiaTensor() * glm::cross(point.rB, impulse));
           }
       }
   }
}

void ConstraintSolver::solveVelocityConstraints(std::vector<std::shared_ptr<Constraint>>& constraints) {
   // Solve contact constraints
   for (auto& contact : solverContacts) {
       solveContactVelocityConstraint(contact);
   }
   
   // Solve joint constraints
   for (auto& constraint : constraints) {
       if (constraint && constraint->isEnabled() && !constraint->isBroken()) {
           constraint->solve(1.0f / 60.0f); // Use fixed timestep for stability
       }
   }
}

void ConstraintSolver::solveContactVelocityConstraint(SolverContact& contact) {
   RigidBody* bodyA = contact.manifold->getBodyA();
   RigidBody* bodyB = contact.manifold->getBodyB();
   
   for (auto& point : contact.points) {
       // Calculate relative velocity
       glm::vec3 velA = bodyA->getLinearVelocity() + glm::cross(bodyA->getAngularVelocity(), point.rA);
       glm::vec3 velB = bodyB->getLinearVelocity() + glm::cross(bodyB->getAngularVelocity(), point.rB);
       glm::vec3 relativeVelocity = velB - velA;
       
       // Solve normal constraint
       float normalVelocity = glm::dot(relativeVelocity, contact.normal);
       float normalLambda = -(normalVelocity + point.bias + point.velocityBias) * point.normalMass;
       
       // Clamp accumulated impulse
       float oldNormalImpulse = point.normalImpulse;
       point.normalImpulse = glm::max(0.0f, oldNormalImpulse + normalLambda);
       normalLambda = point.normalImpulse - oldNormalImpulse;
       
       // Apply normal impulse
       glm::vec3 normalImpulse = normalLambda * contact.normal;
       
       if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
           bodyA->setLinearVelocity(bodyA->getLinearVelocity() - normalImpulse * bodyA->getInverseMass());
           bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                    bodyA->getInverseInertiaTensor() * glm::cross(point.rA, normalImpulse));
       }
       
       if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
           bodyB->setLinearVelocity(bodyB->getLinearVelocity() + normalImpulse * bodyB->getInverseMass());
           bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                    bodyB->getInverseInertiaTensor() * glm::cross(point.rB, normalImpulse));
       }
       
       // Solve friction constraints if friction > 0
       if (contact.friction > 0.0f) {
           // Recalculate relative velocity after normal impulse
           velA = bodyA->getLinearVelocity() + glm::cross(bodyA->getAngularVelocity(), point.rA);
           velB = bodyB->getLinearVelocity() + glm::cross(bodyB->getAngularVelocity(), point.rB);
           relativeVelocity = velB - velA;
           
           // Tangent 1
           float tangent1Velocity = glm::dot(relativeVelocity, contact.tangent1);
           float tangent1Lambda = -tangent1Velocity * point.tangentMass1;
           
           float maxFriction = contact.friction * point.normalImpulse;
           float oldTangent1Impulse = point.tangentImpulse1;
           point.tangentImpulse1 = glm::clamp(oldTangent1Impulse + tangent1Lambda, 
                                             -maxFriction, maxFriction);
           tangent1Lambda = point.tangentImpulse1 - oldTangent1Impulse;
           
           // Apply tangent impulse 1
           glm::vec3 tangent1Impulse = tangent1Lambda * contact.tangent1;
           
           if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyA->setLinearVelocity(bodyA->getLinearVelocity() - tangent1Impulse * bodyA->getInverseMass());
               bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                        bodyA->getInverseInertiaTensor() * glm::cross(point.rA, tangent1Impulse));
           }
           
           if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyB->setLinearVelocity(bodyB->getLinearVelocity() + tangent1Impulse * bodyB->getInverseMass());
               bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                        bodyB->getInverseInertiaTensor() * glm::cross(point.rB, tangent1Impulse));
           }
           
           // Tangent 2
           float tangent2Velocity = glm::dot(relativeVelocity, contact.tangent2);
           float tangent2Lambda = -tangent2Velocity * point.tangentMass2;
           
           float oldTangent2Impulse = point.tangentImpulse2;
           point.tangentImpulse2 = glm::clamp(oldTangent2Impulse + tangent2Lambda, 
                                             -maxFriction, maxFriction);
           tangent2Lambda = point.tangentImpulse2 - oldTangent2Impulse;
           
           // Apply tangent impulse 2
           glm::vec3 tangent2Impulse = tangent2Lambda * contact.tangent2;
           
           if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyA->setLinearVelocity(bodyA->getLinearVelocity() - tangent2Impulse * bodyA->getInverseMass());
               bodyA->setAngularVelocity(bodyA->getAngularVelocity() - 
                                        bodyA->getInverseInertiaTensor() * glm::cross(point.rA, tangent2Impulse));
           }
           
           if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
               bodyB->setLinearVelocity(bodyB->getLinearVelocity() + tangent2Impulse * bodyB->getInverseMass());
               bodyB->setAngularVelocity(bodyB->getAngularVelocity() + 
                                        bodyB->getInverseInertiaTensor() * glm::cross(point.rB, tangent2Impulse));
           }
       }
   }
}

void ConstraintSolver::solvePositionConstraints(std::vector<ContactManifold>& contacts) {
   for (auto& manifold : contacts) {
       solveContactPositionConstraint(manifold);
   }
}

void ConstraintSolver::solveContactPositionConstraint(ContactManifold& manifold) {
   if (!manifold.hasContacts()) return;
   
   RigidBody* bodyA = manifold.getBodyA();
   RigidBody* bodyB = manifold.getBodyB();
   
   const auto& contacts = manifold.getContacts();
   glm::vec3 normal = manifold.getNormal();
   
   for (const auto& contact : contacts) {
       if (contact.penetrationDepth <= contactTolerance) continue;
       
       // Calculate current positions
       glm::vec3 worldPointA = bodyA->localToWorld(contact.localPointA);
       glm::vec3 worldPointB = bodyB->localToWorld(contact.localPointB);
       
       // Calculate separation
       glm::vec3 separation = worldPointB - worldPointA;
       float currentPenetration = -glm::dot(separation, normal);
       
       if (currentPenetration <= contactTolerance) continue;
       
       // Calculate position correction
       float correction = currentPenetration * 0.8f; // 80% position correction
       glm::vec3 correctionVector = normal * correction;
       
       // Calculate effective mass for position correction
       glm::vec3 rA = worldPointA - bodyA->getPosition();
       glm::vec3 rB = worldPointB - bodyB->getPosition();
       
       float invMassA = bodyA->getBodyType() == RigidBody::BodyType::Dynamic ? bodyA->getInverseMass() : 0.0f;
       float invMassB = bodyB->getBodyType() == RigidBody::BodyType::Dynamic ? bodyB->getInverseMass() : 0.0f;
       
       float effectiveMass = invMassA + invMassB;
       
       if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
           glm::vec3 rAcrossN = glm::cross(rA, normal);
           effectiveMass += glm::dot(rAcrossN, bodyA->getInverseInertiaTensor() * rAcrossN);
       }
       
       if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
           glm::vec3 rBcrossN = glm::cross(rB, normal);
           effectiveMass += glm::dot(rBcrossN, bodyB->getInverseInertiaTensor() * rBcrossN);
       }
       
       if (effectiveMass <= 0.0f) continue;
       
       float lambda = correction / effectiveMass;
       glm::vec3 impulse = lambda * normal;
       
       // Apply position correction
       if (bodyA->getBodyType() == RigidBody::BodyType::Dynamic) {
           bodyA->setPosition(bodyA->getPosition() - impulse * invMassA);
           
           // Angular position correction
           glm::vec3 angularImpulse = bodyA->getInverseInertiaTensor() * glm::cross(rA, -impulse);
           glm::quat deltaRotation(0.0f, angularImpulse.x, angularImpulse.y, angularImpulse.z);
           deltaRotation = deltaRotation * bodyA->getOrientation();
           glm::quat newOrientation = bodyA->getOrientation() + deltaRotation * 0.5f;
           bodyA->setOrientation(glm::normalize(newOrientation));
       }
       
       if (bodyB->getBodyType() == RigidBody::BodyType::Dynamic) {
           bodyB->setPosition(bodyB->getPosition() + impulse * invMassB);
           
           // Angular position correction
           glm::vec3 angularImpulse = bodyB->getInverseInertiaTensor() * glm::cross(rB, impulse);
           glm::quat deltaRotation(0.0f, angularImpulse.x, angularImpulse.y, angularImpulse.z);
           deltaRotation = deltaRotation * bodyB->getOrientation();
           glm::quat newOrientation = bodyB->getOrientation() + deltaRotation * 0.5f;
           bodyB->setOrientation(glm::normalize(newOrientation));
       }
   }
}

void ConstraintSolver::storeContactImpulses(std::vector<ContactManifold>& contacts) {
   for (size_t i = 0; i < contacts.size() && i < solverContacts.size(); ++i) {
       auto& manifold = contacts[i];
       auto& solverContact = solverContacts[i];
       
       auto& contactPoints = const_cast<std::vector<ContactPoint>&>(manifold.getContacts());
       
       for (size_t j = 0; j < contactPoints.size() && j < solverContact.points.size(); ++j) {
           contactPoints[j].normalImpulse = solverContact.points[j].normalImpulse;
           contactPoints[j].tangentImpulse1 = solverContact.points[j].tangentImpulse1;
           contactPoints[j].tangentImpulse2 = solverContact.points[j].tangentImpulse2;
       }
   }
}

float ConstraintSolver::calculateMixedRestitution(float restitutionA, float restitutionB) {
   return glm::max(restitutionA, restitutionB);
}

float ConstraintSolver::calculateMixedFriction(float frictionA, float frictionB) {
   return std::sqrt(frictionA * frictionB);
}

void ConstraintSolver::calculateContactTangents(const glm::vec3& normal, glm::vec3& tangent1, glm::vec3& tangent2) {
   if (std::abs(normal.x) >= 0.57735f) {
       tangent1 = glm::normalize(glm::vec3(normal.y, -normal.x, 0.0f));
   } else {
       tangent1 = glm::normalize(glm::vec3(0.0f, normal.z, -normal.y));
   }
   
   tangent2 = glm::cross(normal, tangent1);
}

void ConstraintSolver::updateStatistics(const std::vector<ContactManifold>& contacts,
                                      const std::vector<std::shared_ptr<Constraint>>& constraints) {
   stats.contactConstraints = static_cast<int>(contacts.size());
   stats.jointConstraints = static_cast<int>(constraints.size());
   
   // Calculate average contact error
   stats.contactError = 0.0f;
   int totalContacts = 0;
   
   for (const auto& manifold : contacts) {
       for (const auto& contact : manifold.getContacts()) {
           stats.contactError += contact.penetrationDepth;
           totalContacts++;
       }
   }
   
   if (totalContacts > 0) {
       stats.contactError /= totalContacts;
   }
   
   // Calculate average joint error
   stats.jointError = 0.0f;
   int activeJoints = 0;
   
   for (const auto& constraint : constraints) {
       if (constraint && constraint->isEnabled() && !constraint->isBroken()) {
           stats.jointError += constraint->getAppliedImpulse();
           activeJoints++;
       }
   }
   
   if (activeJoints > 0) {
       stats.jointError /= activeJoints;
   }
}

} // namespace engine::physics