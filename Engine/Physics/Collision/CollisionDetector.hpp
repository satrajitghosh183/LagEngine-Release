#pragma once

#include "../../Core/Base.hpp"
#include "ContactManifold.hpp"
#include "../Shapes/CollisionShape.hpp"
#include "../Shapes/SphereShape.hpp"
#include "../Shapes/BoxShape.hpp"
#include "../Shapes/CapsuleShape.hpp"
#include "../Shapes/PlaneShape.hpp"

namespace GameEngine {
namespace Physics {

    /**
     * @brief Narrow phase collision detection
     * 
     * Detects collisions between specific shape pairs
     * and generates contact manifolds
     */
    class CollisionDetector {
    public:
        /**
         * @brief Detect collision between two shapes
         */
        static bool DetectCollision(
            const Ref<CollisionShape>& shapeA, const glm::vec3& posA, const glm::quat& rotA,
            const Ref<CollisionShape>& shapeB, const glm::vec3& posB, const glm::quat& rotB,
            ContactManifold& manifold
        );
        
    private:
        // Sphere vs X
        static bool SphereSphere(const SphereShape* a, const glm::vec3& posA,
                                const SphereShape* b, const glm::vec3& posB,
                                ContactManifold& manifold);
        
        static bool SphereBox(const SphereShape* sphere, const glm::vec3& posS, const glm::quat& rotS,
                             const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                             ContactManifold& manifold);
        
        static bool SpherePlane(const SphereShape* sphere, const glm::vec3& posS,
                               const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                               ContactManifold& manifold);
        
        static bool SphereCapsule(const SphereShape* sphere, const glm::vec3& posS,
                                 const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                                 ContactManifold& manifold);

        // Box vs X
        static bool BoxBox(const BoxShape* a, const glm::vec3& posA, const glm::quat& rotA,
                          const BoxShape* b, const glm::vec3& posB, const glm::quat& rotB,
                          ContactManifold& manifold);

        static bool BoxPlane(const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                            const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                            ContactManifold& manifold);

        // Capsule vs X
        static bool CapsuleCapsule(const CapsuleShape* a, const glm::vec3& posA, const glm::quat& rotA,
                                  const CapsuleShape* b, const glm::vec3& posB, const glm::quat& rotB,
                                  ContactManifold& manifold);

        static bool CapsuleBox(const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                              const BoxShape* box, const glm::vec3& posB, const glm::quat& rotB,
                              ContactManifold& manifold);

        static bool CapsulePlane(const CapsuleShape* capsule, const glm::vec3& posC, const glm::quat& rotC,
                                const PlaneShape* plane, const glm::vec3& posP, const glm::quat& rotP,
                                ContactManifold& manifold);

        // Helper: closest point between two line segments
        static void ClosestPointsOnSegments(
            const glm::vec3& p1, const glm::vec3& q1,
            const glm::vec3& p2, const glm::vec3& q2,
            glm::vec3& closestA, glm::vec3& closestB);

        // Helper: closest point on segment to a point
        static glm::vec3 ClosestPointOnSegment(const glm::vec3& point,
                                               const glm::vec3& segA, const glm::vec3& segB);

        // Helper: SAT (Separating Axis Theorem) test
        static bool SATTest(const BoxShape* a, const glm::vec3& posA, const glm::quat& rotA,
                           const BoxShape* b, const glm::vec3& posB, const glm::quat& rotB,
                           ContactManifold& manifold);
    };

}}