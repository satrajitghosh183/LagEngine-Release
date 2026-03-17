#include "CollisionDetector.hpp"
#include "../../Core/Logger.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

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
        
        // Plane vs Sphere (swap)
        if (typeA == CollisionShape::ShapeType::Plane && typeB == CollisionShape::ShapeType::Sphere) {
            bool result = SpherePlane(
                static_cast<const SphereShape*>(shapeB.get()), posB,
                static_cast<const PlaneShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }

        // Sphere vs Capsule
        if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Capsule) {
            return SphereCapsule(
                static_cast<const SphereShape*>(shapeA.get()), posA,
                static_cast<const CapsuleShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Capsule vs Sphere (swap)
        if (typeA == CollisionShape::ShapeType::Capsule && typeB == CollisionShape::ShapeType::Sphere) {
            bool result = SphereCapsule(
                static_cast<const SphereShape*>(shapeB.get()), posB,
                static_cast<const CapsuleShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }

        // Box vs Box
        if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Box) {
            return BoxBox(
                static_cast<const BoxShape*>(shapeA.get()), posA, rotA,
                static_cast<const BoxShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Box vs Plane
        if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Plane) {
            return BoxPlane(
                static_cast<const BoxShape*>(shapeA.get()), posA, rotA,
                static_cast<const PlaneShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Plane vs Box (swap)
        if (typeA == CollisionShape::ShapeType::Plane && typeB == CollisionShape::ShapeType::Box) {
            bool result = BoxPlane(
                static_cast<const BoxShape*>(shapeB.get()), posB, rotB,
                static_cast<const PlaneShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }

        // Capsule vs Capsule
        if (typeA == CollisionShape::ShapeType::Capsule && typeB == CollisionShape::ShapeType::Capsule) {
            return CapsuleCapsule(
                static_cast<const CapsuleShape*>(shapeA.get()), posA, rotA,
                static_cast<const CapsuleShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Capsule vs Box
        if (typeA == CollisionShape::ShapeType::Capsule && typeB == CollisionShape::ShapeType::Box) {
            return CapsuleBox(
                static_cast<const CapsuleShape*>(shapeA.get()), posA, rotA,
                static_cast<const BoxShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Box vs Capsule (swap)
        if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Capsule) {
            bool result = CapsuleBox(
                static_cast<const CapsuleShape*>(shapeB.get()), posB, rotB,
                static_cast<const BoxShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }

        // Capsule vs Plane
        if (typeA == CollisionShape::ShapeType::Capsule && typeB == CollisionShape::ShapeType::Plane) {
            return CapsulePlane(
                static_cast<const CapsuleShape*>(shapeA.get()), posA, rotA,
                static_cast<const PlaneShape*>(shapeB.get()), posB, rotB,
                manifold
            );
        }

        // Plane vs Capsule (swap)
        if (typeA == CollisionShape::ShapeType::Plane && typeB == CollisionShape::ShapeType::Capsule) {
            bool result = CapsulePlane(
                static_cast<const CapsuleShape*>(shapeB.get()), posB, rotB,
                static_cast<const PlaneShape*>(shapeA.get()), posA, rotA,
                manifold
            );
            for (auto& contact : manifold.GetContacts()) {
                contact.Normal = -contact.Normal;
                std::swap(contact.PositionA, contact.PositionB);
            }
            return result;
        }

        return false;
    }

    bool CollisionDetector::SphereSphere(const SphereShape* a, const glm::vec3& posA,
                                        const SphereShape* b, const glm::vec3& posB,
                                        ContactManifold& manifold) {
        glm::vec3 delta = posB - posA;
        float distSq = glm::dot(delta, delta);
        float radiusSum = a->GetRadius() + b->GetRadius();
        
        if (distSq > radiusSum * radiusSum) {
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
            float rB = std::abs(glm::dot(axesB[0], axis)) * halfB.x +
                      std::abs(glm::dot(axesB[1], axis)) * halfB.y +
                      std::abs(glm::dot(axesB[2], axis)) * halfB.z;

            float separation = std::abs(glm::dot(delta, axis)) - (rA + rB);
            
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
            
            float rA = std::abs(glm::dot(axesA[0], axis)) * halfA.x +
                      std::abs(glm::dot(axesA[1], axis)) * halfA.y +
                      std::abs(glm::dot(axesA[2], axis)) * halfA.z;
            float rB = halfB[i];

            float separation = std::abs(glm::dot(delta, axis)) - (rA + rB);
            
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
        
        // Test 9 edge-edge cross product axes
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                glm::vec3 axis = glm::cross(axesA[i], axesB[j]);
                float lenSq = glm::dot(axis, axis);
                if (lenSq < 1e-8f) continue;  // Parallel edges, skip
                axis /= std::sqrt(lenSq);
                
                float rA = std::abs(glm::dot(axesA[(i+1)%3], axis)) * halfA[(i+1)%3] +
                           std::abs(glm::dot(axesA[(i+2)%3], axis)) * halfA[(i+2)%3];
                float rB = std::abs(glm::dot(axesB[(j+1)%3], axis)) * halfB[(j+1)%3] +
                           std::abs(glm::dot(axesB[(j+2)%3], axis)) * halfB[(j+2)%3];

                float separation = std::abs(glm::dot(delta, axis)) - (rA + rB);
                
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

    // ---- Helper: closest point on line segment to a point ----
    glm::vec3 CollisionDetector::ClosestPointOnSegment(const glm::vec3& point,
                                                        const glm::vec3& segA, const glm::vec3& segB) {
        glm::vec3 ab = segB - segA;
        float t = glm::dot(point - segA, ab);
        float denom = glm::dot(ab, ab);
        if (denom < 1e-8f) return segA;
        t = glm::clamp(t / denom, 0.0f, 1.0f);
        return segA + ab * t;
    }

    // ---- Helper: closest points between two line segments ----
    void CollisionDetector::ClosestPointsOnSegments(
        const glm::vec3& p1, const glm::vec3& q1,
        const glm::vec3& p2, const glm::vec3& q2,
        glm::vec3& closestA, glm::vec3& closestB) {

        glm::vec3 d1 = q1 - p1;
        glm::vec3 d2 = q2 - p2;
        glm::vec3 r = p1 - p2;

        float a = glm::dot(d1, d1);
        float e = glm::dot(d2, d2);
        float f = glm::dot(d2, r);

        float s, t;

        if (a <= 1e-8f && e <= 1e-8f) {
            closestA = p1;
            closestB = p2;
            return;
        }
        if (a <= 1e-8f) {
            s = 0.0f;
            t = glm::clamp(f / e, 0.0f, 1.0f);
        } else {
            float c = glm::dot(d1, r);
            if (e <= 1e-8f) {
                t = 0.0f;
                s = glm::clamp(-c / a, 0.0f, 1.0f);
            } else {
                float b = glm::dot(d1, d2);
                float denom = a * e - b * b;
                if (denom != 0.0f) {
                    s = glm::clamp((b * f - c * e) / denom, 0.0f, 1.0f);
                } else {
                    s = 0.0f;
                }
                t = (b * s + f) / e;
                if (t < 0.0f) {
                    t = 0.0f;
                    s = glm::clamp(-c / a, 0.0f, 1.0f);
                } else if (t > 1.0f) {
                    t = 1.0f;
                    s = glm::clamp((b - c) / a, 0.0f, 1.0f);
                }
            }
        }

        closestA = p1 + d1 * s;
        closestB = p2 + d2 * t;
    }

    // ---- Sphere vs Capsule ----
    bool CollisionDetector::SphereCapsule(const SphereShape* sphere, const glm::vec3& posS,
                                          const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                                          ContactManifold& manifold) {
        // Capsule axis is along local Y
        glm::vec3 up = rotC * glm::vec3(0, 1, 0);
        float halfCyl = capsule->GetCylinderHeight() * 0.5f;
        glm::vec3 capA = posC - up * halfCyl;
        glm::vec3 capB = posC + up * halfCyl;

        glm::vec3 closest = ClosestPointOnSegment(posS, capA, capB);

        glm::vec3 delta = posS - closest;
        float distSq = glm::dot(delta, delta);
        float radiusSum = sphere->GetRadius() + capsule->GetRadius();

        if (distSq >= radiusSum * radiusSum) return false;

        float dist = std::sqrt(distSq);
        ContactPoint contact;

        if (dist > 1e-6f) {
            contact.Normal = delta / dist;
        } else {
            contact.Normal = glm::vec3(0, 1, 0);
        }
        contact.Penetration = radiusSum - dist;
        contact.PositionA = posS - contact.Normal * sphere->GetRadius();
        contact.PositionB = closest + contact.Normal * capsule->GetRadius();

        manifold.AddContact(contact);
        return true;
    }

    // ---- Capsule vs Capsule ----
    bool CollisionDetector::CapsuleCapsule(const CapsuleShape* a, const glm::vec3& posA, const glm::quat& rotA,
                                            const CapsuleShape* b, const glm::vec3& posB, const glm::quat& rotB,
                                            ContactManifold& manifold) {
        glm::vec3 upA = rotA * glm::vec3(0, 1, 0);
        glm::vec3 upB = rotB * glm::vec3(0, 1, 0);
        float halfCylA = a->GetCylinderHeight() * 0.5f;
        float halfCylB = b->GetCylinderHeight() * 0.5f;

        glm::vec3 a0 = posA - upA * halfCylA;
        glm::vec3 a1 = posA + upA * halfCylA;
        glm::vec3 b0 = posB - upB * halfCylB;
        glm::vec3 b1 = posB + upB * halfCylB;

        glm::vec3 closestA, closestB;
        ClosestPointsOnSegments(a0, a1, b0, b1, closestA, closestB);

        glm::vec3 delta = closestB - closestA;
        float distSq = glm::dot(delta, delta);
        float radiusSum = a->GetRadius() + b->GetRadius();

        if (distSq >= radiusSum * radiusSum) return false;

        float dist = std::sqrt(distSq);
        ContactPoint contact;

        if (dist > 1e-6f) {
            contact.Normal = delta / dist;
        } else {
            contact.Normal = glm::vec3(0, 1, 0);
        }
        contact.Penetration = radiusSum - dist;
        contact.PositionA = closestA + contact.Normal * a->GetRadius();
        contact.PositionB = closestB - contact.Normal * b->GetRadius();

        manifold.AddContact(contact);
        return true;
    }

    // ---- Capsule vs Box ----
    bool CollisionDetector::CapsuleBox(const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                                        const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                                        ContactManifold& manifold) {
        // Reduce capsule to its medial segment, find closest point on segment
        // to box, then treat as sphere-box with that closest point.
        glm::vec3 up = rotC * glm::vec3(0, 1, 0);
        float halfCyl = capsule->GetCylinderHeight() * 0.5f;
        glm::vec3 capA = posC - up * halfCyl;
        glm::vec3 capB = posC + up * halfCyl;

        // Transform capsule segment to box local space
        glm::quat invRotB = glm::inverse(rotB);
        glm::mat3 rotBMat = glm::mat3_cast(rotB);
        glm::mat3 invRotBMat = glm::mat3_cast(invRotB);

        glm::vec3 localCapA = invRotBMat * (capA - posB);
        glm::vec3 localCapB = invRotBMat * (capB - posB);
        glm::vec3 halfExtents = box->GetHalfExtents();

        // Find the point on capsule segment closest to box
        // Sample several points and pick the closest to the clamped box surface
        float bestDistSq = std::numeric_limits<float>::max();
        glm::vec3 bestSegPoint;

        // Check endpoints and midpoint plus a few intermediate points
        constexpr int NUM_SAMPLES = 5;
        for (int i = 0; i <= NUM_SAMPLES; i++) {
            float t = static_cast<float>(i) / static_cast<float>(NUM_SAMPLES);
            glm::vec3 segPoint = localCapA + (localCapB - localCapA) * t;
            glm::vec3 clamped = glm::clamp(segPoint, -halfExtents, halfExtents);
            float dSq = glm::dot(segPoint - clamped, segPoint - clamped);
            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                bestSegPoint = segPoint;
            }
        }

        // Refine: closest point on segment to the clamped point of the best sample
        glm::vec3 bestClamped = glm::clamp(bestSegPoint, -halfExtents, halfExtents);
        glm::vec3 refinedSeg = ClosestPointOnSegment(bestClamped, localCapA, localCapB);
        glm::vec3 closestOnBox = glm::clamp(refinedSeg, -halfExtents, halfExtents);

        // Transform back to world space
        glm::vec3 worldSegPoint = posB + rotBMat * refinedSeg;
        glm::vec3 worldBoxPoint = posB + rotBMat * closestOnBox;

        glm::vec3 delta = worldSegPoint - worldBoxPoint;
        float distSq = glm::dot(delta, delta);
        float radius = capsule->GetRadius();

        if (distSq >= radius * radius) return false;

        float dist = std::sqrt(distSq);
        ContactPoint contact;

        if (dist > 1e-6f) {
            contact.Normal = delta / dist;
        } else {
            // Segment inside box - find shortest exit axis
            glm::vec3 localPt = refinedSeg;
            float minDist2 = std::abs(halfExtents.x - std::abs(localPt.x));
            int axis = 0;
            if (std::abs(halfExtents.y - std::abs(localPt.y)) < minDist2) {
                minDist2 = std::abs(halfExtents.y - std::abs(localPt.y));
                axis = 1;
            }
            if (std::abs(halfExtents.z - std::abs(localPt.z)) < minDist2) {
                axis = 2;
            }
            glm::vec3 localNormal(0.0f);
            localNormal[axis] = localPt[axis] > 0 ? 1.0f : -1.0f;
            contact.Normal = rotBMat * localNormal;
        }

        contact.Penetration = radius - dist;
        contact.PositionA = worldSegPoint - contact.Normal * radius;
        contact.PositionB = worldBoxPoint;

        manifold.AddContact(contact);
        return true;
    }

    // ---- Capsule vs Plane ----
    bool CollisionDetector::CapsulePlane(const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                                          const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                                          ContactManifold& manifold) {
        glm::mat3 rotPMat = glm::mat3_cast(rotP);
        glm::vec3 normal = rotPMat * plane->GetNormal();
        glm::vec3 planePoint = posP + normal * plane->GetDistance();

        glm::vec3 up = rotC * glm::vec3(0, 1, 0);
        float halfCyl = capsule->GetCylinderHeight() * 0.5f;
        float radius = capsule->GetRadius();

        // Test both endpoints of the capsule segment
        glm::vec3 endpoints[2] = {
            posC - up * halfCyl,
            posC + up * halfCyl
        };

        bool hasContact = false;
        for (int i = 0; i < 2; i++) {
            float dist = glm::dot(endpoints[i] - planePoint, normal);
            if (dist < radius) {
                ContactPoint contact;
                contact.Normal = normal;
                contact.Penetration = radius - dist;
                contact.PositionA = endpoints[i] - normal * radius;
                contact.PositionB = endpoints[i] - normal * dist;
                manifold.AddContact(contact);
                hasContact = true;
            }
        }

        return hasContact;
    }

    // ---- Box vs Plane ----
    bool CollisionDetector::BoxPlane(const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                                      const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                                      ContactManifold& manifold) {
        glm::mat3 rotPMat = glm::mat3_cast(rotP);
        glm::vec3 normal = rotPMat * plane->GetNormal();
        glm::vec3 planePoint = posP + normal * plane->GetDistance();

        glm::mat3 rotBMat = glm::mat3_cast(rotB);
        glm::vec3 half = box->GetHalfExtents();

        // Test all 8 corners of the box
        bool hasContact = false;
        for (int i = 0; i < 8; i++) {
            glm::vec3 localCorner(
                (i & 1) ? half.x : -half.x,
                (i & 2) ? half.y : -half.y,
                (i & 4) ? half.z : -half.z
            );
            glm::vec3 worldCorner = posB + rotBMat * localCorner;
            float dist = glm::dot(worldCorner - planePoint, normal);

            if (dist < 0.0f) {
                ContactPoint contact;
                contact.Normal = normal;
                contact.Penetration = -dist;
                contact.PositionA = worldCorner;
                contact.PositionB = worldCorner - normal * dist;
                manifold.AddContact(contact);
                hasContact = true;
            }
        }

        return hasContact;
    }

}}
