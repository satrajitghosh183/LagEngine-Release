#include "BoxShape.hpp"
#include <algorithm>
#include <array>

namespace engine::physics {

BoxShape::BoxShape(const glm::vec3& extents) 
    : halfExtents(glm::max(extents, glm::vec3(0.001f))) {
}

BoxShape::BoxShape(float halfX, float halfY, float halfZ) 
   : BoxShape(glm::vec3(halfX, halfY, halfZ)) {
}

BoundingBox BoxShape::getAABB(const Transform& transform) const {
   if (transform.rotation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
       // No rotation, simple case
       glm::vec3 scaledExtents = halfExtents * transform.scale;
       glm::vec3 marginVec(margin);
       return BoundingBox(
           transform.position - scaledExtents - marginVec,
           transform.position + scaledExtents + marginVec
       );
   } else {
       // Rotation present, need to calculate rotated AABB
       return calculateRotatedAABB(transform);
   }
}

bool BoxShape::raycast(const Ray& ray, const Transform& transform, RaycastHit& hit) const {
   // Transform ray to local space
   glm::mat4 invTransform = glm::inverse(transform.getMatrix());
   glm::vec3 localOrigin = glm::vec3(invTransform * glm::vec4(ray.origin, 1.0f));
   glm::vec3 localDirection = glm::vec3(invTransform * glm::vec4(ray.direction, 0.0f));
   
   // AABB ray intersection in local space
   glm::vec3 invDir = 1.0f / localDirection;
   glm::vec3 t1 = (-halfExtents - localOrigin) * invDir;
   glm::vec3 t2 = (halfExtents - localOrigin) * invDir;
   
   glm::vec3 tMin = glm::min(t1, t2);
   glm::vec3 tMax = glm::max(t1, t2);
   
   float tNear = glm::max(glm::max(tMin.x, tMin.y), tMin.z);
   float tFar = glm::min(glm::min(tMax.x, tMax.y), tMax.z);
   
   if (tNear > tFar || tFar < 0.0f || tNear > ray.maxDistance) {
       return false;
   }
   
   float t = (tNear >= 0.0f) ? tNear : tFar;
   if (t > ray.maxDistance) {
       return false;
   }
   
   // Calculate hit point and normal in local space
   glm::vec3 localHitPoint = localOrigin + localDirection * t;
   
   // Determine which face was hit
   glm::vec3 localNormal(0.0f);
   glm::vec3 abs_point = glm::abs(localHitPoint);
   
   if (abs_point.x >= abs_point.y && abs_point.x >= abs_point.z) {
       localNormal.x = (localHitPoint.x > 0.0f) ? 1.0f : -1.0f;
   } else if (abs_point.y >= abs_point.z) {
       localNormal.y = (localHitPoint.y > 0.0f) ? 1.0f : -1.0f;
   } else {
       localNormal.z = (localHitPoint.z > 0.0f) ? 1.0f : -1.0f;
   }
   
   // Transform back to world space
   glm::mat4 worldTransform = transform.getMatrix();
   hit.hit = true;
   hit.distance = t * glm::length(localDirection);
   hit.point = glm::vec3(worldTransform * glm::vec4(localHitPoint, 1.0f));
   hit.normal = glm::normalize(glm::vec3(worldTransform * glm::vec4(localNormal, 0.0f)));
   
   return true;
}

glm::vec3 BoxShape::support(const glm::vec3& direction, const Transform& transform) const {
   // Transform direction to local space
   glm::vec3 localDir = glm::inverse(transform.rotation) * direction;
   
   // Find support point in local space
   glm::vec3 localSupport = glm::vec3(
       (localDir.x >= 0.0f) ? halfExtents.x : -halfExtents.x,
       (localDir.y >= 0.0f) ? halfExtents.y : -halfExtents.y,
       (localDir.z >= 0.0f) ? halfExtents.z : -halfExtents.z
   );
   
   // Scale and transform to world space
   localSupport *= transform.scale;
   return transform.position + transform.rotation * localSupport;
}

float BoxShape::calculateVolume() const {
   glm::vec3 size = halfExtents * 2.0f;
   return size.x * size.y * size.z;
}

glm::mat3 BoxShape::calculateInertiaTensor(float mass) const {
   glm::vec3 size = halfExtents * 2.0f;
   float x2 = size.x * size.x;
   float y2 = size.y * size.y;
   float z2 = size.z * size.z;
   float factor = mass / 12.0f;
   
   return glm::mat3(
       factor * (y2 + z2), 0, 0,
       0, factor * (x2 + z2), 0,
       0, 0, factor * (x2 + y2)
   );
}

void BoxShape::setHalfExtents(const glm::vec3& extents) {
   halfExtents = glm::max(extents, glm::vec3(0.001f));
}

std::vector<glm::vec3> BoxShape::getVertices(const Transform& transform) const {
   std::vector<glm::vec3> vertices(8);
   glm::vec3 scaledExtents = halfExtents * transform.scale;
   
   // Generate 8 vertices of the box in local space
   vertices[0] = glm::vec3(-scaledExtents.x, -scaledExtents.y, -scaledExtents.z);
   vertices[1] = glm::vec3( scaledExtents.x, -scaledExtents.y, -scaledExtents.z);
   vertices[2] = glm::vec3( scaledExtents.x,  scaledExtents.y, -scaledExtents.z);
   vertices[3] = glm::vec3(-scaledExtents.x,  scaledExtents.y, -scaledExtents.z);
   vertices[4] = glm::vec3(-scaledExtents.x, -scaledExtents.y,  scaledExtents.z);
   vertices[5] = glm::vec3( scaledExtents.x, -scaledExtents.y,  scaledExtents.z);
   vertices[6] = glm::vec3( scaledExtents.x,  scaledExtents.y,  scaledExtents.z);
   vertices[7] = glm::vec3(-scaledExtents.x,  scaledExtents.y,  scaledExtents.z);
   
   // Transform to world space
   for (auto& vertex : vertices) {
       vertex = transform.position + transform.rotation * vertex;
   }
   
   return vertices;
}

std::vector<glm::vec3> BoxShape::getFaceNormals(const Transform& transform) const {
   std::vector<glm::vec3> normals(6);
   
   // Local face normals
   std::array<glm::vec3, 6> localNormals = {{
       glm::vec3( 1.0f,  0.0f,  0.0f),  // +X
       glm::vec3(-1.0f,  0.0f,  0.0f),  // -X
       glm::vec3( 0.0f,  1.0f,  0.0f),  // +Y
       glm::vec3( 0.0f, -1.0f,  0.0f),  // -Y
       glm::vec3( 0.0f,  0.0f,  1.0f),  // +Z
       glm::vec3( 0.0f,  0.0f, -1.0f)   // -Z
   }};
   
   // Transform to world space
   for (int i = 0; i < 6; ++i) {
       normals[i] = transform.rotation * localNormals[i];
   }
   
   return normals;
}

BoundingBox BoxShape::calculateRotatedAABB(const Transform& transform) const {
   std::vector<glm::vec3> vertices = getVertices(transform);
   
   BoundingBox aabb;
   for (const auto& vertex : vertices) {
       aabb.expand(vertex);
   }
   
   // Add margin
   glm::vec3 marginVec(margin);
   aabb.min -= marginVec;
   aabb.max += marginVec;
   
   return aabb;
}

} // namespace engine::physics