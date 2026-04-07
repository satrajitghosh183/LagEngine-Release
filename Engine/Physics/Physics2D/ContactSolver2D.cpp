#include "ContactSolver2D.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace GameEngine {
namespace Physics2D {

    // =====================================================================
    // Dispatch collision test by shape types
    // =====================================================================

    bool ContactSolver2D::testCollision(Collider2D* a, const glm::vec2& posA, float rotA,
                                        Collider2D* b, const glm::vec2& posB, float rotB,
                                        ContactManifold2D& manifold) {
        auto typeA = a->getType();
        auto typeB = b->getType();

        // Circle vs Circle
        if (typeA == ShapeType2D::Circle && typeB == ShapeType2D::Circle) {
            return circleVsCircle(static_cast<CircleCollider2D*>(a), posA, rotA,
                                  static_cast<CircleCollider2D*>(b), posB, rotB, manifold);
        }

        // Circle vs Box
        if (typeA == ShapeType2D::Circle && typeB == ShapeType2D::Box) {
            bool result = circleVsBox(static_cast<CircleCollider2D*>(a), posA, rotA,
                                      static_cast<BoxCollider2D*>(b), posB, rotB, manifold);
            return result;
        }
        if (typeA == ShapeType2D::Box && typeB == ShapeType2D::Circle) {
            bool result = circleVsBox(static_cast<CircleCollider2D*>(b), posB, rotB,
                                      static_cast<BoxCollider2D*>(a), posA, rotA, manifold);
            if (result) {
                manifold.Normal = -manifold.Normal;
                std::swap(manifold.BodyA, manifold.BodyB);
                std::swap(manifold.ColliderA, manifold.ColliderB);
            }
            return result;
        }

        // Box vs Box
        if (typeA == ShapeType2D::Box && typeB == ShapeType2D::Box) {
            return boxVsBox(static_cast<BoxCollider2D*>(a), posA, rotA,
                            static_cast<BoxCollider2D*>(b), posB, rotB, manifold);
        }

        // Circle vs Polygon
        if (typeA == ShapeType2D::Circle && typeB == ShapeType2D::Polygon) {
            return circleVsPolygon(static_cast<CircleCollider2D*>(a), posA, rotA,
                                   static_cast<PolygonCollider2D*>(b), posB, rotB, manifold);
        }
        if (typeA == ShapeType2D::Polygon && typeB == ShapeType2D::Circle) {
            bool result = circleVsPolygon(static_cast<CircleCollider2D*>(b), posB, rotB,
                                          static_cast<PolygonCollider2D*>(a), posA, rotA, manifold);
            if (result) {
                manifold.Normal = -manifold.Normal;
                std::swap(manifold.BodyA, manifold.BodyB);
                std::swap(manifold.ColliderA, manifold.ColliderB);
            }
            return result;
        }

        // Polygon vs Polygon (also handles Box vs Polygon since Box can provide vertices)
        if (typeA == ShapeType2D::Polygon && typeB == ShapeType2D::Polygon) {
            return polygonVsPolygon(static_cast<PolygonCollider2D*>(a), posA, rotA,
                                    static_cast<PolygonCollider2D*>(b), posB, rotB, manifold);
        }

        return false;
    }

    // =====================================================================
    // Circle vs Circle
    // =====================================================================

    bool ContactSolver2D::circleVsCircle(CircleCollider2D* a, const glm::vec2& posA, float rotA,
                                          CircleCollider2D* b, const glm::vec2& posB, float rotB,
                                          ContactManifold2D& manifold) {
        glm::vec2 centerA = posA + Collider2D::rotatePoint(a->Offset, rotA);
        glm::vec2 centerB = posB + Collider2D::rotatePoint(b->Offset, rotB);

        glm::vec2 diff = centerB - centerA;
        float distSq = glm::dot(diff, diff);
        float radiusSum = a->Radius + b->Radius;

        if (distSq >= radiusSum * radiusSum) return false;

        float dist = std::sqrt(distSq);

        if (dist < 1e-6f) {
            manifold.Normal = glm::vec2(0.0f, 1.0f);
            manifold.Contacts[0].Position = centerA;
            manifold.Contacts[0].Penetration = radiusSum;
        } else {
            manifold.Normal = diff / dist;
            manifold.Contacts[0].Position = centerA + manifold.Normal * a->Radius;
            manifold.Contacts[0].Penetration = radiusSum - dist;
        }
        manifold.ContactCount = 1;
        return true;
    }

    // =====================================================================
    // Circle vs Box
    // =====================================================================

    bool ContactSolver2D::circleVsBox(CircleCollider2D* circle, const glm::vec2& posC, float rotC,
                                       BoxCollider2D* box, const glm::vec2& posB, float rotB,
                                       ContactManifold2D& manifold) {
        glm::vec2 circleCenter = posC + Collider2D::rotatePoint(circle->Offset, rotC);
        glm::vec2 boxCenter = posB + Collider2D::rotatePoint(box->Offset, rotB);

        // Transform circle center into box's local space
        glm::vec2 localCircle = Collider2D::rotatePoint(circleCenter - boxCenter, -rotB);

        // Clamp to box half-extents
        glm::vec2 closest;
        closest.x = std::clamp(localCircle.x, -box->HalfExtents.x, box->HalfExtents.x);
        closest.y = std::clamp(localCircle.y, -box->HalfExtents.y, box->HalfExtents.y);

        glm::vec2 diff = localCircle - closest;
        float distSq = glm::dot(diff, diff);

        if (distSq >= circle->Radius * circle->Radius) return false;

        float dist = std::sqrt(distSq);

        // Convert normal back to world space
        glm::vec2 normal;
        if (dist < 1e-6f) {
            // Circle center is inside box — find nearest edge
            float dx = box->HalfExtents.x - std::abs(localCircle.x);
            float dy = box->HalfExtents.y - std::abs(localCircle.y);
            if (dx < dy) {
                normal = (localCircle.x > 0.0f) ? glm::vec2(1.0f, 0.0f) : glm::vec2(-1.0f, 0.0f);
                manifold.Contacts[0].Penetration = circle->Radius + dx;
            } else {
                normal = (localCircle.y > 0.0f) ? glm::vec2(0.0f, 1.0f) : glm::vec2(0.0f, -1.0f);
                manifold.Contacts[0].Penetration = circle->Radius + dy;
            }
        } else {
            normal = diff / dist;
            manifold.Contacts[0].Penetration = circle->Radius - dist;
        }

        // Rotate normal back to world space
        manifold.Normal = Collider2D::rotatePoint(normal, rotB);

        // Contact point in world space
        glm::vec2 worldClosest = boxCenter + Collider2D::rotatePoint(closest, rotB);
        manifold.Contacts[0].Position = worldClosest;
        manifold.ContactCount = 1;
        return true;
    }

    // =====================================================================
    // SAT helpers
    // =====================================================================

    ContactSolver2D::ProjectionResult ContactSolver2D::projectPolygon(
        const glm::vec2* verts, int count, const glm::vec2& axis) {
        float minProj = glm::dot(verts[0], axis);
        float maxProj = minProj;
        for (int i = 1; i < count; i++) {
            float proj = glm::dot(verts[i], axis);
            minProj = std::min(minProj, proj);
            maxProj = std::max(maxProj, proj);
        }
        return {minProj, maxProj};
    }

    float ContactSolver2D::overlapOnAxis(const glm::vec2* vertsA, int countA,
                                          const glm::vec2* vertsB, int countB,
                                          const glm::vec2& axis) {
        auto projA = projectPolygon(vertsA, countA, axis);
        auto projB = projectPolygon(vertsB, countB, axis);

        if (projA.Max < projB.Min || projB.Max < projA.Min) {
            return -1.0f; // Separated
        }
        return std::min(projA.Max - projB.Min, projB.Max - projA.Min);
    }

    // =====================================================================
    // Box vs Box (SAT)
    // =====================================================================

    bool ContactSolver2D::boxVsBox(BoxCollider2D* a, const glm::vec2& posA, float rotA,
                                    BoxCollider2D* b, const glm::vec2& posB, float rotB,
                                    ContactManifold2D& manifold) {
        glm::vec2 vertsA[4], vertsB[4];
        a->getWorldVertices(posA, rotA, vertsA);
        b->getWorldVertices(posB, rotB, vertsB);

        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 bestAxis;

        // Test 4 axes: 2 from each box
        auto testAxis = [&](const glm::vec2& v0, const glm::vec2& v1) -> bool {
            glm::vec2 edge = v1 - v0;
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));
            if (glm::length(axis) < 1e-6f) return true;

            float overlap = overlapOnAxis(vertsA, 4, vertsB, 4, axis);
            if (overlap < 0.0f) return false;
            if (overlap < minOverlap) {
                minOverlap = overlap;
                bestAxis = axis;
            }
            return true;
        };

        for (int i = 0; i < 4; i++) {
            if (!testAxis(vertsA[i], vertsA[(i + 1) % 4])) return false;
            if (!testAxis(vertsB[i], vertsB[(i + 1) % 4])) return false;
        }

        // Ensure normal points from A to B
        glm::vec2 centerA = posA + Collider2D::rotatePoint(a->Offset, rotA);
        glm::vec2 centerB = posB + Collider2D::rotatePoint(b->Offset, rotB);
        if (glm::dot(bestAxis, centerB - centerA) < 0.0f) {
            bestAxis = -bestAxis;
        }

        manifold.Normal = bestAxis;
        manifold.Contacts[0].Penetration = minOverlap;

        // Find contact point(s) — use the deepest penetrating vertex
        float bestDist = -std::numeric_limits<float>::max();
        glm::vec2 bestPoint;

        for (int i = 0; i < 4; i++) {
            float d = glm::dot(vertsB[i], -manifold.Normal);
            if (d > bestDist) {
                bestDist = d;
                bestPoint = vertsB[i];
            }
        }

        manifold.Contacts[0].Position = bestPoint;
        manifold.ContactCount = 1;

        // Try to find a second contact point for edge-edge
        for (int i = 0; i < 4; i++) {
            float d = glm::dot(vertsB[i], -manifold.Normal);
            if (std::abs(d - bestDist) < 0.01f && glm::distance(vertsB[i], bestPoint) > 0.01f) {
                manifold.Contacts[1].Position = vertsB[i];
                manifold.Contacts[1].Penetration = minOverlap;
                manifold.ContactCount = 2;
                break;
            }
        }

        return true;
    }

    // =====================================================================
    // Circle vs Polygon
    // =====================================================================

    bool ContactSolver2D::circleVsPolygon(CircleCollider2D* circle, const glm::vec2& posC, float rotC,
                                           PolygonCollider2D* poly, const glm::vec2& posP, float rotP,
                                           ContactManifold2D& manifold) {
        glm::vec2 circleCenter = posC + Collider2D::rotatePoint(circle->Offset, rotC);

        glm::vec2 worldVerts[PolygonCollider2D::MaxVertices];
        int vertCount;
        poly->getWorldVertices(posP, rotP, worldVerts, vertCount);

        // Find closest point on polygon to circle center
        float minDist = std::numeric_limits<float>::max();
        glm::vec2 closestPoint;
        int closestEdge = -1;

        for (int i = 0; i < vertCount; i++) {
            int j = (i + 1) % vertCount;
            glm::vec2 a = worldVerts[i];
            glm::vec2 b = worldVerts[j];
            glm::vec2 ab = b - a;
            float t = glm::dot(circleCenter - a, ab) / glm::dot(ab, ab);
            t = std::clamp(t, 0.0f, 1.0f);
            glm::vec2 closest = a + ab * t;
            float dist = glm::distance(circleCenter, closest);
            if (dist < minDist) {
                minDist = dist;
                closestPoint = closest;
                closestEdge = i;
            }
        }

        if (minDist >= circle->Radius) return false;

        glm::vec2 diff = circleCenter - closestPoint;
        float dist = glm::length(diff);

        if (dist < 1e-6f) {
            // Circle center on edge — use edge normal
            int j = (closestEdge + 1) % vertCount;
            glm::vec2 edge = worldVerts[j] - worldVerts[closestEdge];
            manifold.Normal = glm::normalize(glm::vec2(-edge.y, edge.x));
        } else {
            manifold.Normal = diff / dist;
        }

        manifold.Contacts[0].Position = closestPoint;
        manifold.Contacts[0].Penetration = circle->Radius - minDist;
        manifold.ContactCount = 1;
        return true;
    }

    // =====================================================================
    // Polygon vs Polygon (SAT)
    // =====================================================================

    bool ContactSolver2D::polygonVsPolygon(PolygonCollider2D* a, const glm::vec2& posA, float rotA,
                                            PolygonCollider2D* b, const glm::vec2& posB, float rotB,
                                            ContactManifold2D& manifold) {
        glm::vec2 vertsA[PolygonCollider2D::MaxVertices];
        glm::vec2 vertsB[PolygonCollider2D::MaxVertices];
        int countA, countB;
        a->getWorldVertices(posA, rotA, vertsA, countA);
        b->getWorldVertices(posB, rotB, vertsB, countB);

        float minOverlap = std::numeric_limits<float>::max();
        glm::vec2 bestAxis;

        // Test normals from A
        for (int i = 0; i < countA; i++) {
            int j = (i + 1) % countA;
            glm::vec2 edge = vertsA[j] - vertsA[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float overlap = overlapOnAxis(vertsA, countA, vertsB, countB, axis);
            if (overlap < 0.0f) return false;
            if (overlap < minOverlap) {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        // Test normals from B
        for (int i = 0; i < countB; i++) {
            int j = (i + 1) % countB;
            glm::vec2 edge = vertsB[j] - vertsB[i];
            glm::vec2 axis = glm::normalize(glm::vec2(-edge.y, edge.x));

            float overlap = overlapOnAxis(vertsA, countA, vertsB, countB, axis);
            if (overlap < 0.0f) return false;
            if (overlap < minOverlap) {
                minOverlap = overlap;
                bestAxis = axis;
            }
        }

        glm::vec2 centerA = posA + Collider2D::rotatePoint(a->Offset, rotA);
        glm::vec2 centerB = posB + Collider2D::rotatePoint(b->Offset, rotB);
        if (glm::dot(bestAxis, centerB - centerA) < 0.0f) {
            bestAxis = -bestAxis;
        }

        manifold.Normal = bestAxis;
        manifold.Contacts[0].Penetration = minOverlap;

        // Find the deepest vertex of B as contact point
        float bestDist = -std::numeric_limits<float>::max();
        for (int i = 0; i < countB; i++) {
            float d = glm::dot(vertsB[i], -manifold.Normal);
            if (d > bestDist) {
                bestDist = d;
                manifold.Contacts[0].Position = vertsB[i];
            }
        }
        manifold.ContactCount = 1;
        return true;
    }

    // =====================================================================
    // Impulse-based contact resolution
    // =====================================================================

    void ContactSolver2D::resolveContact(ContactManifold2D& manifold, float /*dt*/) {
        RigidBody2D* a = manifold.BodyA;
        RigidBody2D* b = manifold.BodyB;

        if (!a || !b) return;

        float invMassA = a->getInverseMass();
        float invMassB = b->getInverseMass();
        float invInertiaA = a->getInverseInertia();
        float invInertiaB = b->getInverseInertia();

        if (invMassA + invMassB == 0.0f) return;

        float friction = std::sqrt(a->getMaterial().Friction * b->getMaterial().Friction);
        float restitution = std::max(a->getMaterial().Restitution, b->getMaterial().Restitution);

        for (int i = 0; i < manifold.ContactCount; i++) {
            glm::vec2 contactPoint = manifold.Contacts[i].Position;
            glm::vec2 rA = contactPoint - a->getPosition();
            glm::vec2 rB = contactPoint - b->getPosition();

            // Relative velocity at contact point
            glm::vec2 relVel = (b->getLinearVelocity() + glm::vec2(-b->getAngularVelocity() * rB.y,
                                                                      b->getAngularVelocity() * rB.x))
                             - (a->getLinearVelocity() + glm::vec2(-a->getAngularVelocity() * rA.y,
                                                                      a->getAngularVelocity() * rA.x));

            float contactVelNormal = glm::dot(relVel, manifold.Normal);

            // Don't resolve if separating
            if (contactVelNormal > 0.0f) continue;

            // Cross products for angular contribution
            float rAxN = rA.x * manifold.Normal.y - rA.y * manifold.Normal.x;
            float rBxN = rB.x * manifold.Normal.y - rB.y * manifold.Normal.x;

            float invMassSum = invMassA + invMassB
                             + rAxN * rAxN * invInertiaA
                             + rBxN * rBxN * invInertiaB;

            if (invMassSum == 0.0f) continue;

            // Normal impulse
            float jn = -(1.0f + restitution) * contactVelNormal / invMassSum;
            jn /= static_cast<float>(manifold.ContactCount);

            // Accumulate (clamp to non-negative)
            float oldImpulse = manifold.NormalImpulseAccum[i];
            manifold.NormalImpulseAccum[i] = std::max(oldImpulse + jn, 0.0f);
            jn = manifold.NormalImpulseAccum[i] - oldImpulse;

            glm::vec2 normalImpulse = manifold.Normal * jn;

            a->applyLinearImpulse(-normalImpulse);
            if (!a->isFixedRotation()) a->applyAngularImpulse(-(rA.x * normalImpulse.y - rA.y * normalImpulse.x));
            b->applyLinearImpulse(normalImpulse);
            if (!b->isFixedRotation()) b->applyAngularImpulse(rB.x * normalImpulse.y - rB.y * normalImpulse.x);

            // Friction impulse
            glm::vec2 tangent = relVel - manifold.Normal * contactVelNormal;
            float tangentLen = glm::length(tangent);
            if (tangentLen < 1e-6f) continue;
            tangent /= tangentLen;

            float rAxT = rA.x * tangent.y - rA.y * tangent.x;
            float rBxT = rB.x * tangent.y - rB.y * tangent.x;

            float invMassSumT = invMassA + invMassB
                              + rAxT * rAxT * invInertiaA
                              + rBxT * rBxT * invInertiaB;

            float jt = -glm::dot(relVel, tangent) / invMassSumT;
            jt /= static_cast<float>(manifold.ContactCount);

            // Coulomb friction clamp
            float maxFriction = friction * manifold.NormalImpulseAccum[i];
            float oldTangent = manifold.TangentImpulseAccum[i];
            manifold.TangentImpulseAccum[i] = std::clamp(oldTangent + jt, -maxFriction, maxFriction);
            jt = manifold.TangentImpulseAccum[i] - oldTangent;

            glm::vec2 frictionImpulse = tangent * jt;

            a->applyLinearImpulse(-frictionImpulse);
            if (!a->isFixedRotation()) a->applyAngularImpulse(-(rA.x * frictionImpulse.y - rA.y * frictionImpulse.x));
            b->applyLinearImpulse(frictionImpulse);
            if (!b->isFixedRotation()) b->applyAngularImpulse(rB.x * frictionImpulse.y - rB.y * frictionImpulse.x);
        }
    }

    // =====================================================================
    // Baumgarte position correction
    // =====================================================================

    void ContactSolver2D::correctPositions(ContactManifold2D& manifold) {
        RigidBody2D* a = manifold.BodyA;
        RigidBody2D* b = manifold.BodyB;
        if (!a || !b) return;

        float invMassA = a->getInverseMass();
        float invMassB = b->getInverseMass();
        float totalInvMass = invMassA + invMassB;
        if (totalInvMass == 0.0f) return;

        for (int i = 0; i < manifold.ContactCount; i++) {
            float penetration = manifold.Contacts[i].Penetration;
            float correction = std::max(penetration - k_Slop, 0.0f) * k_BaumgarteScale / totalInvMass;
            correction = std::min(correction, k_MaxCorrection);

            glm::vec2 correctionVec = manifold.Normal * correction;

            if (a->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                a->setPosition(a->getPosition() - correctionVec * invMassA);
            }
            if (b->getBodyType() == RigidBody2D::BodyType::Dynamic) {
                b->setPosition(b->getPosition() + correctionVec * invMassB);
            }
        }
    }

}}
