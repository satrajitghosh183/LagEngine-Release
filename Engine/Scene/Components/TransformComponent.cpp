#include "TransformComponent.hpp"
#include "../Scene.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    TransformComponent::TransformComponent() {
    }

    TransformComponent::TransformComponent(const glm::vec3& position)
        : Position(position) {
    }

    nlohmann::json TransformComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        j["position"] = { Position.x, Position.y, Position.z };
        j["rotation"] = { Rotation.w, Rotation.x, Rotation.y, Rotation.z };
        j["scale"] = { Scale.x, Scale.y, Scale.z };
        return j;
    }

    void TransformComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("position")) {
            auto pos = data["position"];
            Position = glm::vec3(pos[0], pos[1], pos[2]);
        }
        
        if (data.contains("rotation")) {
            auto rot = data["rotation"];
            Rotation = glm::quat(rot[0], rot[1], rot[2], rot[3]);
        }
        
        if (data.contains("scale")) {
            auto scl = data["scale"];
            Scale = glm::vec3(scl[0], scl[1], scl[2]);
        }
        
        SetDirty();
    }

    glm::mat4 TransformComponent::GetLocalTransform() const {
        if (m_Dirty) {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), Position);
            glm::mat4 rotation = glm::toMat4(Rotation);
            glm::mat4 scale = glm::scale(glm::mat4(1.0f), Scale);
            
            m_CachedTransform = translation * rotation * scale;
            m_Dirty = false;
        }
        
        return m_CachedTransform;
    }

    glm::mat4 TransformComponent::GetWorldTransform() const {
        glm::mat4 localTransform = GetLocalTransform();
        
        if (GetOwnerEntity() && GetOwnerEntity().HasParent()) {
            Entity parent = GetOwnerEntity().GetParent();
            if (parent.HasComponent<TransformComponent>()) {
                auto& parentTransform = parent.GetComponent<TransformComponent>();
                return parentTransform.GetWorldTransform() * localTransform;
            }
        }
        
        return localTransform;
    }

    glm::vec3 TransformComponent::GetWorldPosition() const {
        glm::mat4 worldTransform = GetWorldTransform();
        return glm::vec3(worldTransform[3]);
    }

    glm::quat TransformComponent::GetWorldRotation() const {
        if (GetOwnerEntity() && GetOwnerEntity().HasParent()) {
            Entity parent = GetOwnerEntity().GetParent();
            if (parent.HasComponent<TransformComponent>()) {
                auto& parentTransform = parent.GetComponent<TransformComponent>();
                return parentTransform.GetWorldRotation() * Rotation;
            }
        }
        
        return Rotation;
    }

    glm::vec3 TransformComponent::GetWorldScale() const {
        if (GetOwnerEntity() && GetOwnerEntity().HasParent()) {
            Entity parent = GetOwnerEntity().GetParent();
            if (parent.HasComponent<TransformComponent>()) {
                auto& parentTransform = parent.GetComponent<TransformComponent>();
                return parentTransform.GetWorldScale() * Scale;
            }
        }
        
        return Scale;
    }

    void TransformComponent::SetWorldPosition(const glm::vec3& position) {
        if (GetOwnerEntity() && GetOwnerEntity().HasParent()) {
            Entity parent = GetOwnerEntity().GetParent();
            if (parent.HasComponent<TransformComponent>()) {
                auto& parentTransform = parent.GetComponent<TransformComponent>();
                glm::mat4 parentWorld = parentTransform.GetWorldTransform();
                glm::mat4 parentInverse = glm::inverse(parentWorld);
                glm::vec4 localPos = parentInverse * glm::vec4(position, 1.0f);
                Position = glm::vec3(localPos);
                SetDirty();
                return;
            }
        }
        
        Position = position;
        SetDirty();
    }

    glm::vec3 TransformComponent::GetForward() const {
        return glm::normalize(Rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    }

    glm::vec3 TransformComponent::GetRight() const {
        return glm::normalize(Rotation * glm::vec3(1.0f, 0.0f, 0.0f));
    }

    glm::vec3 TransformComponent::GetUp() const {
        return glm::normalize(Rotation * glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void TransformComponent::Translate(const glm::vec3& delta) {
        Position += delta;
        SetDirty();
    }

    void TransformComponent::Rotate(const glm::vec3& axis, float angle) {
        Rotation = glm::rotate(Rotation, glm::radians(angle), axis);
        SetDirty();
    }

    void TransformComponent::RotateEuler(const glm::vec3& eulerAngles) {
        glm::quat rotX = glm::angleAxis(glm::radians(eulerAngles.x), glm::vec3(1, 0, 0));
        glm::quat rotY = glm::angleAxis(glm::radians(eulerAngles.y), glm::vec3(0, 1, 0));
        glm::quat rotZ = glm::angleAxis(glm::radians(eulerAngles.z), glm::vec3(0, 0, 1));
        Rotation = rotY * rotX * rotZ;
        SetDirty();
    }

    void TransformComponent::SetScale(const glm::vec3& scale) {
        Scale = scale;
        SetDirty();
    }

    void TransformComponent::LookAt(const glm::vec3& target, const glm::vec3& up) {
        glm::vec3 direction = glm::normalize(target - Position);
        Rotation = glm::quatLookAt(direction, up);
        SetDirty();
    }

    glm::vec3 TransformComponent::GetEulerAngles() const {
        return glm::degrees(glm::eulerAngles(Rotation));
    }

    void TransformComponent::SetEulerAngles(const glm::vec3& eulerAngles) {
        Rotation = glm::quat(glm::radians(eulerAngles));
        SetDirty();
    }

    void TransformComponent::SetDirty() {
        m_Dirty = true;
        
        // Mark children as dirty too
        if (GetOwnerEntity()) {
            for (auto child : GetOwnerEntity().GetChildren()) {
                if (child.HasComponent<TransformComponent>()) {
                    child.GetComponent<TransformComponent>().SetDirty();
                }
            }
        }
    }
}