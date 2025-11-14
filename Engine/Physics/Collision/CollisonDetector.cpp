#include "CollisionDetector.hpp"
#include "../../Core/Logger.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {
namespace Physics {

    bool CollisionDetector::DetectCollision(
        const Ref<CollisionShape>& shapeA, const glm::vec3& posA, const glm::quat& rotA,
        const Ref<CollisionShape>& shapeB, const glm::vec3& posB, const glm::quat& rotB,
        ContactManifold& manifold) {
        
        manifold.Clear();
        
        auto typeA = shapeA->GetType();
        auto typeB = shapeB->GetType();
        
        // Sphere vs Sphere
        if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Sphere) {
            return SphereSphere(
                static_cast<const SphereShape*>(shapeA.get()), posA,
                static_cast<const SphereShape*>(shapeB.get()), posB,
                manifold
            );
        }
        
        // Sphere vs Box
        if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Box) {
            return SphereBox(
                static_cast<const SphereShape*>(shapeA.get()), posA, rotA,
                static_cast<const BoxShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }
        
        // Box vs Sphere (swap)
        if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Sphere) {
            bool result = SphereBox(
                static_cast<const SphereShape*>(shapeB.get()), posB, rotB,
                static_cast<const BoxShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            // Flip contact normals
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }
        
        // Sphere vs Plane
        if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Plane) {
            return SpherePlane(
                static_cast<const SphereShape*>(shapeA.get()), posA,
                static_cast<const PlaneShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }
        
        // Box vs Box
        if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Box) {
            return BoxBox(
                static_cast<const BoxShape*>(shapeA.get()), posA, rotA,
                static_cast<const BoxShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }
        
        return false;
    }

    bool CollisionDetector::SphereSphere(const SphereShape* a, const glm::vec3& posA,
                                        const SphereShape* b, const glm::vec3& posB,
                                        ContactManifold& manifold) {
        glm::vec3 delta = posB - posA;
        float distSq = glm::dot(delta, delta);
        float radiusSum = a->GetRadius() + b->GetRadius();
        
        if (distSq >= radiusSum * radiusSum) {
            return false;
        }
        
        float dist = sqrt(distSq);
        
        ContactPoint contact;
        
        if (dist > 0.0001f) {
            contact.Normal = delta / dist;
            contact.Penetration = radiusSum - dist;
            contact.PositionA = posA + contact.Normal * a->GetRadius();
            contact.PositionB = posB - contact.Normal * b->GetRadius();
        } else {
            // Spheres at same position
            contact.Normal = glm::vec3(0, 1, 0);
            contact.Penetration = radiusSum;
            contact.PositionA = posA;
            contact.PositionB = posB;
        }
        
        manifold.AddContact(contact);
        return true;
    }

    bool CollisionDetector::SphereBox(const SphereShape* sphere, const glm::vec3& posS, const glm::quat& rotS,
                                     const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                                     ContactManifold& manifold) {
        // Transform sphere center to box local space
        glm::quat invRotB = glm::inverse(rotB);
        glm::mat3 rotBMat = glm::mat3_cast(rotB);
        glm::mat3 invRotBMat = glm::mat3_cast(invRotB);
        glm::vec3 localSpherePos = invRotBMat * (posS - posB);
        
        // Find closest point on box to sphere center
        glm::vec3 halfExtents = box->GetHalfExtents();
        glm::vec3 closestPoint = glm::clamp(localSpherePos, -halfExtents, halfExtents);
        
        // Transform back to world space
        glm::vec3 worldClosestPoint = posB + rotBMat * closestPoint;
        
        // Check distance
        glm::vec3 delta = posS - worldClosestPoint;
        float distSq = glm::dot(delta, delta);
        float radius = sphere->GetRadius();
        
        if (distSq >= radius * radius) {
            return false;
        }
        
        float dist = sqrt(distSq);
        
        ContactPoint contact;
        
        if (dist > 0.0001f) {
            contact.Normal = delta / dist;
            contact.Penetration = radius - dist;
            contact.PositionA = posS - contact.Normal * radius;
            contact.PositionB = worldClosestPoint;
        } else {
            // Sphere center inside box
            glm::vec3 localDelta = localSpherePos - closestPoint;
            
            // Find shortest axis to surface
            float minDist = abs(halfExtents.x - abs(localSpherePos.x));
            int axis = 0;
            
            if (abs(halfExtents.y - abs(localSpherePos.y)) < minDist) {
                minDist = abs(halfExtents.y - abs(localSpherePos.y));
                axis = 1;
            }
            if (abs(halfExtents.z - abs(localSpherePos.z)) < minDist) {
                axis = 2;
            }
            
            glm::vec3 localNormal(0.0f);
            localNormal[axis] = localSpherePos[axis] > 0 ? 1.0f : -1.0f;
            
            contact.Normal = rotBMat * localNormal;
            contact.Penetration = radius + minDist;
            contact.PositionA = posS - contact.Normal * radius;
            contact.PositionB = posS + contact.Normal * minDist;
        }
        
        manifold.AddContact(contact);
        return true;
    }

    bool CollisionDetector::SpherePlane(const SphereShape* sphere, const glm::vec3& posS,
                                       const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                                       ContactManifold& manifold) {
        glm::mat3 rotPMat = glm::mat3_cast(rotP);
        glm::vec3 normal = rotPMat * plane->GetNormal();
        glm::vec3 planePoint = posP + normal * plane->GetDistance();
        
        float dist = glm::dot(posS - planePoint, normal);
        
        if (dist >= sphere->GetRadius()) {
            return false;
        }
        
        ContactPoint contact;
        contact.Normal = normal;
        contact.Penetration = sphere->GetRadius() - dist;
        contact.PositionA = posS - normal * sphere->GetRadius();
        contact.PositionB = posS - normal * dist;
        
        manifold.AddContact(contact);
        return true;
    }

    bool CollisionDetector::BoxBox(const BoxShape* a, const glm::vec3& posA, const glm::quat& rotA,
                                   const BoxShape* b, const glm::vec3& posB, const glm::quat& rotB,
                                   ContactManifold& manifold) {
        // Use SAT (Separating Axis Theorem)
        return SATTest(a, posA, rotA, b, posB, rotB, manifold);
    }

    bool CollisionDetector::SATTest(const BoxShape* a, const glm::vec3& posA, const glm::quat& rotA,
                                    const BoxShape* b, const glm::vec3& posB, const glm::quat& rotB,
                                    ContactManifold& manifold) {
        // Simplified SAT implementation
        // Full implementation would test all 15 axes (6 face normals + 9 edge cross products)
        
        // Get axes
        glm::mat3 rotAMat = glm::mat3_cast(rotA);
        glm::mat3 rotBMat = glm::mat3_cast(rotB);
        glm::vec3 axesA[3] = {
            rotAMat * glm::vec3(1, 0, 0),
            rotAMat * glm::vec3(0, 1, 0),
            rotAMat * glm::vec3(0, 0, 1)
        };
        
        glm::vec3 axesB[3] = {
            rotBMat * glm::vec3(1, 0, 0),
            rotBMat * glm::vec3(0, 1, 0),
            rotBMat * glm::vec3(0, 0, 1)
        };
        
        glm::vec3 halfA = a->GetHalfExtents();
        glm::vec3 halfB = b->GetHalfExtents();
        glm::vec3 delta = posB - posA;
        
        float minPenetration = std::numeric_limits<float>::max();
        glm::vec3 minAxis;
        
        // Test face normals of A
        for (int i = 0; i < 3; i++) {
            glm::vec3 axis = axesA[i];
            
            float rA = halfA[i];
            float rB = abs(glm::dot(axesB[0], axis)) * halfB.x +
                      abs(glm::dot(axesB[1], axis)) * halfB.y +
                      abs(glm::dot(axesB[2], axis)) * halfB.z;
            
            float separation = abs(glm::dot(delta, axis)) - (rA + rB);
            
            if (separation > 0) {
                return false;  // Separating axis found
            }
            
            if (-separation < minPenetration) {
                minPenetration = -separation;
                minAxis = axis;
                if (glm::dot(delta, axis) < 0) {
                    minAxis = -minAxis;
                }
            }
        }
        
        // Test face normals of B
        for (int i = 0; i < 3; i++) {
            glm::vec3 axis = axesB[i];
            
            float rA = abs(glm::dot(axesA[0], axis)) * halfA.x +
                      abs(glm::dot(axesA[1], axis)) * halfA.y +
                      abs(glm::dot(axesA[2], axis)) * halfA.z;
            float rB = halfB[i];
            
            float separation = abs(glm::dot(delta, axis)) - (rA + rB);
            
            if (separation > 0) {
                return false;
            }
            
            if (-separation < minPenetration) {
                minPenetration = -separation;
                minAxis = axis;
                if (glm::dot(delta, axis) < 0) {
                    minAxis = -minAxis;
                }
            }
        }
        
        // Create contact
        ContactPoint contact;
        contact.Normal = minAxis;
        contact.Penetration = minPenetration;
        contact.PositionA = posA + minAxis * (halfA.x * 0.5f);  // Simplified
        contact.PositionB = posB - minAxis * (halfB.x * 0.5f);
        
        manifold.AddContact(contact);
        return true;
    }

}}