// #pragma once

// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>

// namespace engine::physics {

// class RigidBody;

// struct Transform {
//     glm::vec3 position{0.0f};
//     glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
//     glm::vec3 scale{1.0f};

//     glm::mat4 getMatrix() const;
//     Transform operator*(const Transform& other) const;
// };

// struct BoundingBox {
//     glm::vec3 min, max;
    
//     BoundingBox() : min(FLT_MAX), max(-FLT_MAX) {}
//     BoundingBox(const glm::vec3& minimum, const glm::vec3& maximum) : min(minimum), max(maximum) {}
    
//     bool intersects(const BoundingBox& other) const;
//     BoundingBox merge(const BoundingBox& other) const;
//     glm::vec3 getCenter() const { return (min + max) * 0.5f; }
//     glm::vec3 getSize() const { return max - min; }
//     float getVolume() const;
//     void expand(const glm::vec3& point);
// };

// struct Ray {
//     glm::vec3 origin;
//     glm::vec3 direction;
//     float maxDistance = FLT_MAX;
    
//     Ray(const glm::vec3& orig, const glm::vec3& dir, float maxDist = FLT_MAX)
//         : origin(orig), direction(glm::normalize(dir)), maxDistance(maxDist) {}
    
//     glm::vec3 getPoint(float t) const { return origin + direction * t; }
// };

// struct RaycastHit {
//     bool hit = false;
//     float distance = 0.0f;
//     glm::vec3 point{0.0f};
//     glm::vec3 normal{0.0f};
//     RigidBody* body = nullptr;
    
//     RaycastHit() = default;
//     RaycastHit(float dist, const glm::vec3& pt, const glm::vec3& norm, RigidBody* rb = nullptr)
//         : hit(true), distance(dist), point(pt), normal(norm), body(rb) {}
// };

// /**
//  * @brief Base class for all collision shapes
//  */
// class CollisionShape {
// public:
//     enum class ShapeType {
//         Sphere,
//         Box,
//         Capsule,
//         Cylinder,
//         Cone,
//         ConvexMesh,
//         ConcaveMesh,
//         Heightfield
//     };

//     virtual ~CollisionShape() = default;

//     // Core interface
//     virtual ShapeType getType() const = 0;
//     virtual BoundingBox getAABB(const Transform& transform) const = 0;
//     virtual bool raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const = 0;
    
//     // Support function for GJK (implemented in derived classes)
//     virtual glm::vec3 support(const glm::vec3& direction, const Transform& transform) const = 0;
    
//     // Volume and inertia calculation
//     virtual float calculateVolume() const = 0;
//     virtual glm::mat3 calculateInertiaTensor(float mass) const = 0;
    
//     // Collision margins for numerical stability
//     virtual float getMargin() const { return margin; }
//     virtual void setMargin(float newMargin) { margin = newMargin; }

// protected:
//     float margin = 0.01f;  // Small margin for numerical stability
// };

// } // namespace engine::physics

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>

namespace engine::physics {

class RigidBody;
struct ContactManifold;

struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 getMatrix() const {
        glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 r = glm::mat4_cast(rotation);
        glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
        return t * r * s;
    }
    
    Transform operator*(const Transform& other) const {
        Transform result;
        result.position = position + rotation * (scale * other.position);
        result.rotation = rotation * other.rotation;
        result.scale = scale * other.scale;
        return result;
    }
};

struct BoundingBox {
    glm::vec3 min, max;
    
    BoundingBox() : min(FLT_MAX), max(-FLT_MAX) {}
    BoundingBox(const glm::vec3& minimum, const glm::vec3& maximum) : min(minimum), max(maximum) {}
    
    bool intersects(const BoundingBox& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
    
    BoundingBox merge(const BoundingBox& other) const {
        return BoundingBox(
            glm::min(min, other.min),
            glm::max(max, other.max)
        );
    }
    
    glm::vec3 getCenter() const { return (min + max) * 0.5f; }
    glm::vec3 getSize() const { return max - min; }
    
    float getVolume() const {
        glm::vec3 size = getSize();
        return size.x * size.y * size.z;
    }
    
    void expand(const glm::vec3& point) {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
    
    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
    float maxDistance = FLT_MAX;
    
    Ray(const glm::vec3& orig, const glm::vec3& dir, float maxDist = FLT_MAX)
        : origin(orig), direction(glm::normalize(dir)), maxDistance(maxDist) {}
    
    glm::vec3 getPoint(float t) const { return origin + direction * t; }
};

struct RaycastHit {
    bool hit = false;
    float distance = 0.0f;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    RigidBody* body = nullptr;
    
    RaycastHit() = default;
    RaycastHit(float dist, const glm::vec3& pt, const glm::vec3& norm, RigidBody* rb = nullptr)
        : hit(true), distance(dist), point(pt), normal(norm), body(rb) {}
};

/**
 * @brief Base class for all collision shapes
 */
class CollisionShape {
public:
    enum class ShapeType {
        Sphere,
        Box,
        Capsule,
        Cylinder,
        Cone,
        ConvexMesh,
        ConcaveMesh,
        Heightfield
    };

    virtual ~CollisionShape() = default;

    // Core interface
    virtual ShapeType getType() const = 0;
    virtual BoundingBox getAABB(const Transform& transform) const = 0;
    virtual bool raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const = 0;
    
    // Support function for GJK (implemented in derived classes)
    virtual glm::vec3 support(const glm::vec3& direction, const Transform& transform) const = 0;
    
    // Volume and inertia calculation
    virtual float calculateVolume() const = 0;
    virtual glm::mat3 calculateInertiaTensor(float mass) const = 0;
    
    // Collision margins for numerical stability
    virtual float getMargin() const { return margin; }
    virtual void setMargin(float newMargin) { margin = newMargin; }

protected:
    float margin = 0.01f;  // Small margin for numerical stability
};

} // namespace engine::physics