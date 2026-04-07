#pragma once

#include "Collider2D.hpp"
#include "RigidBody2D.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {
namespace Physics2D {

    struct ContactPoint2D {
        glm::vec2 Position;
        float Penetration = 0.0f;
    };

    struct ContactManifold2D {
        RigidBody2D* BodyA = nullptr;
        RigidBody2D* BodyB = nullptr;
        Collider2D* ColliderA = nullptr;
        Collider2D* ColliderB = nullptr;
        glm::vec2 Normal{0.0f, 1.0f};
        ContactPoint2D Contacts[2];
        int ContactCount = 0;

        // Cached impulses for warm starting
        float NormalImpulseAccum[2] = {0.0f, 0.0f};
        float TangentImpulseAccum[2] = {0.0f, 0.0f};
    };

    class ContactSolver2D {
    public:
        // Collision detection between two shapes
        static bool testCollision(Collider2D* a, const glm::vec2& posA, float rotA,
                                  Collider2D* b, const glm::vec2& posB, float rotB,
                                  ContactManifold2D& manifold);

        // Resolve contacts (apply impulses)
        static void resolveContact(ContactManifold2D& manifold, float dt);

        // Position correction (Baumgarte stabilization)
        static void correctPositions(ContactManifold2D& manifold);

    private:
        // Specific collision tests
        static bool circleVsCircle(CircleCollider2D* a, const glm::vec2& posA, float rotA,
                                   CircleCollider2D* b, const glm::vec2& posB, float rotB,
                                   ContactManifold2D& manifold);

        static bool circleVsBox(CircleCollider2D* circle, const glm::vec2& posC, float rotC,
                                BoxCollider2D* box, const glm::vec2& posB, float rotB,
                                ContactManifold2D& manifold);

        static bool boxVsBox(BoxCollider2D* a, const glm::vec2& posA, float rotA,
                             BoxCollider2D* b, const glm::vec2& posB, float rotB,
                             ContactManifold2D& manifold);

        static bool circleVsPolygon(CircleCollider2D* circle, const glm::vec2& posC, float rotC,
                                    PolygonCollider2D* poly, const glm::vec2& posP, float rotP,
                                    ContactManifold2D& manifold);

        static bool polygonVsPolygon(PolygonCollider2D* a, const glm::vec2& posA, float rotA,
                                     PolygonCollider2D* b, const glm::vec2& posB, float rotB,
                                     ContactManifold2D& manifold);

        // SAT helpers
        struct ProjectionResult {
            float Min, Max;
        };

        static ProjectionResult projectPolygon(const glm::vec2* verts, int count, const glm::vec2& axis);
        static float overlapOnAxis(const glm::vec2* vertsA, int countA,
                                   const glm::vec2* vertsB, int countB,
                                   const glm::vec2& axis);

        static constexpr float k_Slop = 0.005f;
        static constexpr float k_BaumgarteScale = 0.2f;
        static constexpr float k_MaxCorrection = 0.2f;
    };

}}
