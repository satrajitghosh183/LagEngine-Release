#include "ConstraintSolver.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {
namespace Physics {

    ConstraintSolver::ConstraintSolver()
        : m_VelocityIterations(10)
        , m_PositionIterations(3) {
    }

    void ConstraintSolver::SetIterations(int velocityIterations, int positionIterations) {
        m_VelocityIterations = velocityIterations;
        m_PositionIterations = positionIterations;
    }

    void ConstraintSolver::Solve(std::vector<Ref<Constraint>>& constraints,
                                 std::vector<ContactManifold>& contacts,
                                 float deltaTime) {
        // Prepare all constraints
        for (auto& constraint : constraints) {
            if (constraint->Enabled) {
                constraint->Prepare(deltaTime);
            }
        }
        
        // Velocity iterations
        for (int i = 0; i < m_VelocityIterations; i++) {
            SolveVelocityConstraints(constraints, contacts);
        }
        
        // Position iterations (for stability)
        for (int i = 0; i < m_PositionIterations; i++) {
            SolvePositionConstraints(contacts);
        }
    }

    void ConstraintSolver::SolveVelocityConstraints(std::vector<Ref<Constraint>>& constraints,
                                                    std::vector<ContactManifold>& contacts) {
        // Solve joint constraints
        for (auto& constraint : constraints) {
            if (constraint->Enabled) {
                constraint->Solve();
            }
        }
        
        // Solve contact constraints
        for (auto& manifold : contacts) {
            SolveContactVelocity(manifold);
        }
    }

    void ConstraintSolver::SolvePositionConstraints(std::vector<ContactManifold>& contacts) {
        for (auto& manifold : contacts) {
            SolveContactPosition(manifold);
        }
    }

    void ConstraintSolver::SolveContactVelocity(ContactManifold& manifold) {
        RigidBody* bodyA = manifold.GetBodyA();
        RigidBody* bodyB = manifold.GetBodyB();
        
        if (!bodyA || !bodyB) return;
        
        for (auto& contact : manifold.GetContacts()) {
            glm::vec3 rA = contact.PositionA - bodyA->GetPosition();
            glm::vec3 rB = contact.PositionB - bodyB->GetPosition();
            
            // Calculate relative velocity
            glm::vec3 vA = bodyA->GetLinearVelocity() + glm::cross(bodyA->GetAngularVelocity(), rA);
            glm::vec3 vB = bodyB->GetLinearVelocity() + glm::cross(bodyB->GetAngularVelocity(), rB);
            glm::vec3 relVel = vB - vA;
            
            // Normal impulse
            float normalVel = glm::dot(relVel, contact.Normal);
            
            // Calculate effective mass
            float invMassA = bodyA->GetInverseMass();
            float invMassB = bodyB->GetInverseMass();
            
            glm::mat3 invInertiaA = bodyA->GetInverseInertiaTensor();
            glm::mat3 invInertiaB = bodyB->GetInverseInertiaTensor();
            
            glm::vec3 angularA = glm::cross(rA, contact.Normal);
            glm::vec3 angularB = glm::cross(rB, contact.Normal);
            
            float angularMassA = glm::dot(angularA, invInertiaA * angularA);
            float angularMassB = glm::dot(angularB, invInertiaB * angularB);
            
            float effectiveMass = invMassA + invMassB + angularMassA + angularMassB;
            
            if (effectiveMass < 0.0001f) continue;
            
            effectiveMass = 1.0f / effectiveMass;
            
            // Calculate normal impulse with restitution
            float lambda = -(normalVel + manifold.Restitution * normalVel) * effectiveMass;
            
            // Clamp accumulated impulse
            float oldImpulse = contact.NormalImpulse;
            contact.NormalImpulse = std::max(oldImpulse + lambda, 0.0f);
            lambda = contact.NormalImpulse - oldImpulse;
            
            // Apply impulse
            glm::vec3 impulse = contact.Normal * lambda;
            bodyA->ApplyImpulse(-impulse);
            bodyB->ApplyImpulse(impulse);
            bodyA->ApplyAngularImpulse(-glm::cross(rA, impulse));
            bodyB->ApplyAngularImpulse(glm::cross(rB, impulse));
            
            // Friction
            if (manifold.Friction > 0) {
                glm::vec3 tangent = relVel - contact.Normal * normalVel;
                float tangentLength = glm::length(tangent);
                
                if (tangentLength > 0.0001f) {
                    tangent /= tangentLength;
                    
                    float tangentVel = glm::dot(relVel, tangent);
                    float frictionLambda = -tangentVel * effectiveMass;
                    
                    // Coulomb friction
                    float maxFriction = manifold.Friction * contact.NormalImpulse;
                    frictionLambda = glm::clamp(frictionLambda, -maxFriction, maxFriction);
                    
                    glm::vec3 frictionImpulse = tangent * frictionLambda;
                    bodyA->ApplyImpulse(-frictionImpulse);
                    bodyB->ApplyImpulse(frictionImpulse);
                    bodyA->ApplyAngularImpulse(-glm::cross(rA, frictionImpulse));
                    bodyB->ApplyAngularImpulse(glm::cross(rB, frictionImpulse));
                }
            }
        }
    }

    void ConstraintSolver::SolveContactPosition(ContactManifold& manifold) {
        // Position correction using Baumgarte stabilization
        const float slop = 0.01f;  // Penetration slop
        const float baumgarte = 0.2f;
        
        RigidBody* bodyA = manifold.GetBodyA();
        RigidBody* bodyB = manifold.GetBodyB();
        
        if (!bodyA || !bodyB) return;
        
        for (const auto& contact : manifold.GetContacts()) {
            if (contact.Penetration > slop) {
                float correction = baumgarte * (contact.Penetration - slop);
                
                glm::vec3 correctionVec = contact.Normal * correction;
                
                float invMassA = bodyA->GetInverseMass();
                float invMassB = bodyB->GetInverseMass();
                float totalInvMass = invMassA + invMassB;
                
                if (totalInvMass > 0) {
                    glm::vec3 posA = bodyA->GetPosition();
                    glm::vec3 posB = bodyB->GetPosition();
                    
                    bodyA->SetPosition(posA - correctionVec * (invMassA / totalInvMass));
                    bodyB->SetPosition(posB + correctionVec * (invMassB / totalInvMass));
                }
            }
        }
    }

}}