#include "CollisionShape.hpp"
#include <algorithm>

namespace engine::physics {

glm::mat4 Transform::getMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::mat4_cast(rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}

Transform Transform::operator*(const Transform& other) const {
    Transform result;
    result.position = position + rotation * (scale * other.position);
    result.rotation = rotation * other.rotation;
    result.scale = scale * other.scale;
    return result;
}

bool BoundingBox::intersects(const BoundingBox& other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y) &&
           (min.z <= other.max.z && max.z >= other.min.z);
}

BoundingBox BoundingBox::merge(const BoundingBox& other) const {
    return BoundingBox(
        glm::min(min, other.min),
        glm::max(max, other.max)
    );
}

float BoundingBox::getVolume() const {
    glm::vec3 size = getSize();
    return size.x * size.y * size.z;
}

void BoundingBox::expand(const glm::vec3& point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

} // namespace engine::physics