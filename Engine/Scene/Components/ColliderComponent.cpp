#include "ColliderComponent.hpp"
#include "RigidBodyComponent.hpp"
#include "../Entity.hpp"
#include "../Scene.hpp"

namespace GameEngine {

    ColliderComponent::ColliderComponent(const Ref<Physics::CollisionShape>& shape)
        : m_Shape(shape) {
    }

    nlohmann::json ColliderComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        j["isTrigger"] = IsTrigger;
        j["friction"] = Friction;
        j["restitution"] = Restitution;
        
        // Serialize shape type and parameters
        if (m_Shape) {
            j["shapeType"] = static_cast<int>(m_Shape->GetType());
            
            switch (m_Shape->GetType()) {
                case Physics::CollisionShape::ShapeType::Sphere: {
                    auto sphere = std::static_pointer_cast<Physics::SphereShape>(m_Shape);
                    j["radius"] = sphere->GetRadius();
                    break;
                }
                case Physics::CollisionShape::ShapeType::Box: {
                    auto box = std::static_pointer_cast<Physics::BoxShape>(m_Shape);
                    auto extents = box->GetHalfExtents();
                    j["halfExtents"] = { extents.x, extents.y, extents.z };
                    break;
                }
                case Physics::CollisionShape::ShapeType::Capsule: {
                    auto capsule = std::static_pointer_cast<Physics::CapsuleShape>(m_Shape);
                    j["radius"] = capsule->GetRadius();
                    j["height"] = capsule->GetHeight();
                    break;
                }
                case Physics::CollisionShape::ShapeType::Plane: {
                    auto plane = std::static_pointer_cast<Physics::PlaneShape>(m_Shape);
                    auto normal = plane->GetNormal();
                    j["normal"] = { normal.x, normal.y, normal.z };
                    j["distance"] = plane->GetDistance();
                    break;
                }
            }
        }
        
        return j;
    }

    void ColliderComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("isTrigger"))
            IsTrigger = data["isTrigger"];
        
        if (data.contains("friction"))
            Friction = data["friction"];
        
        if (data.contains("restitution"))
            Restitution = data["restitution"];
        
        // Deserialize shape
        if (data.contains("shapeType")) {
            int shapeType = data["shapeType"];
            
            switch (static_cast<Physics::CollisionShape::ShapeType>(shapeType)) {
                case Physics::CollisionShape::ShapeType::Sphere: {
                    float radius = data.value("radius", 0.5f);
                    m_Shape = CreateRef<Physics::SphereShape>(radius);
                    break;
                }
                case Physics::CollisionShape::ShapeType::Box: {
                    glm::vec3 extents(0.5f);
                    if (data.contains("halfExtents")) {
                        auto ext = data["halfExtents"];
                        extents = glm::vec3(ext[0], ext[1], ext[2]);
                    }
                    m_Shape = CreateRef<Physics::BoxShape>(extents);
                    break;
                }
                case Physics::CollisionShape::ShapeType::Capsule: {
                    float radius = data.value("radius", 0.5f);
                    float height = data.value("height", 2.0f);
                    m_Shape = CreateRef<Physics::CapsuleShape>(radius, height);
                    break;
                }
                case Physics::CollisionShape::ShapeType::Plane: {
                    glm::vec3 normal(0, 1, 0);
                    if (data.contains("normal")) {
                        auto n = data["normal"];
                        normal = glm::vec3(n[0], n[1], n[2]);
                    }
                    float distance = data.value("distance", 0.0f);
                    m_Shape = CreateRef<Physics::PlaneShape>(normal, distance);
                    break;
                }
            }
        }
    }

    void ColliderComponent::OnCreate() {
        // Register with physics world
        // TODO: Add collider to physics world collision detection
    }

    void ColliderComponent::OnDestroy() {
        // Unregister from physics world
        // TODO: Remove collider from physics world
    }

    void ColliderComponent::SetShape(const Ref<Physics::CollisionShape>& shape) {
        m_Shape = shape;
        
        // Update rigid body inertia if present
        if (Owner && Owner->HasComponent<RigidBodyComponent>()) {
            auto& rb = Owner->GetComponent<RigidBodyComponent>();
            if (shape && rb.Type == RigidBodyComponent::BodyType::Dynamic) {
                glm::mat3 inertia = shape->CalculateInertiaTensor(rb.Mass);
                rb.GetRigidBody()->SetInertiaTensor(inertia);
            }
        }
    }
}