#include "CollisionDetector.hpp"
#include "../RigidBody.hpp"
#include "../../core/Logger.hpp"
#include <algorithm>
#include <cfloat>

namespace engine::physics {

float CollisionDetector::contactTolerance = 0.01f;
int CollisionDetector::maxContactPoints = 4;

bool CollisionDetector::detectCollision(RigidBody* bodyA, RigidBody* bodyB, ContactManifold& manifold) {
    if (!bodyA || !bodyB || bodyA == bodyB) {
        return false;
    }
    
    auto shapeA = bodyA->getCollisionShape();
    auto shapeB = bodyB->getCollisionShape();
    
    if (!shapeA || !shapeB) {
        return false;
    }
    
    // Set up manifold
    manifold.setBodies(bodyA, bodyB);
    manifold.clearContacts();
    
    // Get transforms
    Transform transformA;
    transformA.position = bodyA->getPosition();
    transformA.rotation = bodyA->getOrientation();
    transformA.scale = glm::vec3(1.0f);
    
    Transform transformB;
    transformB.position = bodyB->getPosition();
    transformB.rotation = bodyB->getOrientation();
    transformB.scale = glm::vec3(1.0f);
    
    // Dispatch to specific collision detection functions
    auto typeA = shapeA->getType();
    auto typeB = shapeB->getType();
    
    if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Sphere) {
        return sphereVsSphere(*static_cast<SphereShape*>(shapeA.get()), transformA,
                             *static_cast<SphereShape*>(shapeB.get()), transformB,
                             manifold);
    }
    else if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Box) {
        return boxVsBox(*static_cast<BoxShape*>(shapeA.get()), transformA,
                       *static_cast<BoxShape*>(shapeB.get()), transformB,
                       manifold);
    }
    else if (typeA == CollisionShape::ShapeType::Sphere && typeB == CollisionShape::ShapeType::Box) {
        return sphereVsBox(*static_cast<SphereShape*>(shapeA.get()), transformA,
                          *static_cast<BoxShape*>(shapeB.get()), transformB,
                          manifold);
    }
    else if (typeA == CollisionShape::ShapeType::Box && typeB == CollisionShape::ShapeType::Sphere) {
        // Swap parameters for sphere vs box
        bool collision = sphereVsBox(*static_cast<SphereShape*>(shapeB.get()), transformB,
                                    *static_cast<BoxShape*>(shapeA.get()), transformA,
                                    manifold);
        if (collision) {
            // Flip normal direction since we swapped bodies
            manifold.setNormal(-manifold.getNormal());
        }
        return collision;
    }
    
    return false;
}

bool CollisionDetector::sphereVsSphere(const SphereShape& sphereA, const Transform& transformA,
                                      const SphereShape& sphereB, const Transform& transformB,
                                      ContactManifold& manifold) {
    glm::vec3 centerA = transformA.position;
    glm::vec3 centerB = transformB.position;
    float radiusA = sphereA.getRadius();
    float radiusB = sphereB.getRadius();
    
    glm::vec3 direction = centerB - centerA;
    float distance = glm::length(direction);
    float combinedRadius = radiusA + radiusB;
    
    if (distance >= combinedRadius) {
        return false; // No collision
    }
    
    // Calculate collision response
    glm::vec3 normal;
    if (distance > 0.0001f) {
        normal = direction / distance;
    } else {
        // Spheres are at the same position, use arbitrary normal
        normal = glm::vec3(1.0f, 0.0f, 0.0f);
        distance = 0.0f;
    }
    
    float penetration = combinedRadius - distance;
    glm::vec3 contactPointA = centerA + normal * radiusA;
    glm::vec3 contactPointB = centerB - normal * radiusB;
    
    manifold.setNormal(normal);
    manifold.addContact(contactPointA, contactPointB, penetration);
    
    return true;
}

bool CollisionDetector::boxVsBox(const BoxShape& boxA, const Transform& transformA,
                                const BoxShape& boxB, const Transform& transformB,
                                ContactManifold& manifold) {
    // Get box vertices
    std::vector<glm::vec3> verticesA, verticesB;
    getBoxVertices(boxA, transformA, verticesA);
    getBoxVertices(boxB, transformB, verticesB);
    
    // Get potential separating axes
    std::vector<glm::vec3> axes;
    getBoxAxes(transformA, axes);
    
    std::vector<glm::vec3> axesB;
    getBoxAxes(transformB, axesB);
    axes.insert(axes.end(), axesB.begin(), axesB.end());
    
    // Add cross product axes
    std::vector<glm::vec3> axesACross;
    getBoxAxes(transformA, axesACross);
    for (const glm::vec3& axisA : axesACross) {
        for (const glm::vec3& axisB : axesB) {
            glm::vec3 crossAxis = glm::cross(axisA, axisB);
            if (glm::length(crossAxis) > 0.0001f) {
                axes.push_back(glm::normalize(crossAxis));
            }
        }
    }
    
    // Test each axis for separation
    float minSeparation = FLT_MAX;
    glm::vec3 separatingAxis;
    bool separated = false;
    
    for (const glm::vec3& axis : axes) {
        float separation;
        if (testSeparatingAxis(axis, verticesA, verticesB, separation)) {
            separated = true;
            break;
        }
        
        if (separation < minSeparation) {
            minSeparation = separation;
            separatingAxis = axis;
        }
    }
    
    if (separated) {
        return false; // Boxes are separated
    }
    
    // Ensure normal points from A to B
    glm::vec3 direction = transformB.position - transformA.position;
    if (glm::dot(separatingAxis, direction) < 0) {
        separatingAxis = -separatingAxis;
    }
    
    manifold.setNormal(separatingAxis);
    calculateBoxBoxContacts(boxA, transformA, boxB, transformB, separatingAxis, -minSeparation, manifold);
    
    return true;
}

bool CollisionDetector::sphereVsBox(const SphereShape& sphere, const Transform& sphereTransform,
                                   const BoxShape& box, const Transform& boxTransform,
                                   ContactManifold& manifold) {
    glm::vec3 sphereCenter = sphereTransform.position;
    float sphereRadius = sphere.getRadius();
    
    // Find closest point on box to sphere center
    glm::vec3 closestPoint = closestPointOnBox(sphereCenter, box, boxTransform);
    
    glm::vec3 direction = sphereCenter - closestPoint;
    float distance = glm::length(direction);
    
    if (distance > sphereRadius) {
        return false; // No collision
    }
    
    glm::vec3 normal;
    if (distance > 0.0001f) {
        normal = direction / distance;
    } else {
        // Sphere center is inside box, find best normal
        glm::vec3 localCenter = glm::inverse(boxTransform.rotation) * (sphereCenter - boxTransform.position);
        glm::vec3 halfExtents = box.getHalfExtents();
        
        // Find axis with minimum penetration
        float minPenetration = FLT_MAX;
        int bestAxis = 0;
        
        for (int i = 0; i < 3; ++i) {
            float penetration = halfExtents[i] - std::abs(localCenter[i]);
            if (penetration < minPenetration) {
                minPenetration = penetration;
                bestAxis = i;
            }
        }
        
        glm::vec3 localNormal(0.0f);
        localNormal[bestAxis] = (localCenter[bestAxis] > 0) ? 1.0f : -1.0f;
        normal = boxTransform.rotation * localNormal;
    }
    
    float penetration = sphereRadius - distance;
    glm::vec3 contactPointA = sphereCenter - normal * sphereRadius;
    glm::vec3 contactPointB = closestPoint;
    
    manifold.setNormal(normal);
    manifold.addContact(contactPointA, contactPointB, penetration);
    
    return true;
}

// ... (Continue with remaining methods in next part)
// ... (Continuation of CollisionDetector.cpp)

bool CollisionDetector::testSeparatingAxis(const glm::vec3& axis,
                                          const std::vector<glm::vec3>& verticesA,
                                          const std::vector<glm::vec3>& verticesB,
                                          float& separation) {
    float minA = FLT_MAX, maxA = -FLT_MAX;
    float minB = FLT_MAX, maxB = -FLT_MAX;
    
    // Project vertices A onto axis
    for (const glm::vec3& vertex : verticesA) {
        float projection = glm::dot(vertex, axis);
        minA = glm::min(minA, projection);
        maxA = glm::max(maxA, projection);
    }
    
    // Project vertices B onto axis
    for (const glm::vec3& vertex : verticesB) {
        float projection = glm::dot(vertex, axis);
        minB = glm::min(minB, projection);
        maxB = glm::max(maxB, projection);
    }
    
    // Check for separation
    if (maxA < minB || maxB < minA) {
        return true; // Separated
    }
    
    // Calculate overlap (negative value indicates penetration)
    separation = glm::min(maxA - minB, maxB - minA);
    return false;
}

void CollisionDetector::getBoxVertices(const BoxShape& box, const Transform& transform,
                                      std::vector<glm::vec3>& vertices) {
    vertices.clear();
    vertices.reserve(8);
    
    glm::vec3 halfExtents = box.getHalfExtents();
    
    // Generate 8 vertices of the box in local space
    vertices.push_back(glm::vec3(-halfExtents.x, -halfExtents.y, -halfExtents.z));
    vertices.push_back(glm::vec3( halfExtents.x, -halfExtents.y, -halfExtents.z));
    vertices.push_back(glm::vec3( halfExtents.x,  halfExtents.y, -halfExtents.z));
    vertices.push_back(glm::vec3(-halfExtents.x,  halfExtents.y, -halfExtents.z));
    vertices.push_back(glm::vec3(-halfExtents.x, -halfExtents.y,  halfExtents.z));
    vertices.push_back(glm::vec3( halfExtents.x, -halfExtents.y,  halfExtents.z));
    vertices.push_back(glm::vec3( halfExtents.x,  halfExtents.y,  halfExtents.z));
    vertices.push_back(glm::vec3(-halfExtents.x,  halfExtents.y,  halfExtents.z));
    
    // Transform to world space
    for (glm::vec3& vertex : vertices) {
        vertex = transform.position + transform.rotation * vertex;
    }
}

void CollisionDetector::getBoxAxes(const Transform& transform, std::vector<glm::vec3>& axes) {
    axes.clear();
    axes.reserve(3);
    
    // Get the three axes of the box (transformed to world space)
    axes.push_back(transform.rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    axes.push_back(transform.rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    axes.push_back(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
}

void CollisionDetector::calculateBoxBoxContacts(const BoxShape& boxA, const Transform& transformA,
                                               const BoxShape& boxB, const Transform& transformB,
                                               const glm::vec3& normal, float separation,
                                               ContactManifold& manifold) {
    // Simplified contact generation for box-box collision
    // This is a complex algorithm, so we'll use a basic approach
    
    std::vector<glm::vec3> verticesA, verticesB;
    getBoxVertices(boxA, transformA, verticesA);
    getBoxVertices(boxB, transformB, verticesB);
    
    std::vector<glm::vec3> contactPoints;
    
    // Find vertices of B that are inside A
    for (const glm::vec3& vertex : verticesB) {
        float distance = pointToPlaneDistance(vertex, normal, transformA.position);
        if (distance <= separation) {
            contactPoints.push_back(vertex);
        }
    }
    
    // Find vertices of A that are inside B
    for (const glm::vec3& vertex : verticesA) {
        float distance = pointToPlaneDistance(vertex, -normal, transformB.position);
        if (distance <= separation) {
            contactPoints.push_back(vertex);
        }
    }
    
    generateContactPoints(contactPoints, normal, separation, manifold);
}

void CollisionDetector::clipFaceAgainstPlane(const std::vector<glm::vec3>& inputVertices,
                                            const glm::vec3& planeNormal, float planeDistance,
                                            std::vector<glm::vec3>& outputVertices) {
    outputVertices.clear();
    
    if (inputVertices.empty()) return;
    
    for (size_t i = 0; i < inputVertices.size(); ++i) {
        const glm::vec3& current = inputVertices[i];
        const glm::vec3& next = inputVertices[(i + 1) % inputVertices.size()];
        
        float currentDistance = glm::dot(current, planeNormal) - planeDistance;
        float nextDistance = glm::dot(next, planeNormal) - planeDistance;
        
        if (currentDistance <= 0.0f) {
            outputVertices.push_back(current);
        }
        
        if ((currentDistance > 0.0f && nextDistance <= 0.0f) ||
            (currentDistance <= 0.0f && nextDistance > 0.0f)) {
            // Edge crosses the plane
            float t = currentDistance / (currentDistance - nextDistance);
            glm::vec3 intersection = current + t * (next - current);
            outputVertices.push_back(intersection);
        }
    }
}

glm::vec3 CollisionDetector::closestPointOnBox(const glm::vec3& point, const BoxShape& box, 
                                              const Transform& transform) {
    // Transform point to box local space
    glm::vec3 localPoint = glm::inverse(transform.rotation) * (point - transform.position);
    
    // Clamp to box bounds
    glm::vec3 halfExtents = box.getHalfExtents();
    glm::vec3 closestLocal = glm::clamp(localPoint, -halfExtents, halfExtents);
    
    // Transform back to world space
    return transform.position + transform.rotation * closestLocal;
}

float CollisionDetector::pointToPlaneDistance(const glm::vec3& point, const glm::vec3& planeNormal, 
                                             const glm::vec3& planePoint) {
    return glm::dot(point - planePoint, planeNormal);
}

void CollisionDetector::generateContactPoints(const std::vector<glm::vec3>& points,
                                             const glm::vec3& normal, float separation,
                                             ContactManifold& manifold) {
    if (points.empty()) return;
    
    // Sort points by distance to manifold normal
    std::vector<std::pair<float, glm::vec3>> sortedPoints;
    for (const glm::vec3& point : points) {
        float distance = glm::dot(point, normal);
        sortedPoints.emplace_back(distance, point);
    }
    
    std::sort(sortedPoints.begin(), sortedPoints.end());
    
    // Take up to maxContactPoints deepest points
    int numContacts = glm::min(static_cast<int>(sortedPoints.size()), maxContactPoints);
    
    for (int i = 0; i < numContacts; ++i) {
        glm::vec3 contactPoint = sortedPoints[i].second;
        glm::vec3 projectedPoint = contactPoint - normal * separation;
        
        manifold.addContact(contactPoint, projectedPoint, separation);
    }
}

} // namespace engine::physics