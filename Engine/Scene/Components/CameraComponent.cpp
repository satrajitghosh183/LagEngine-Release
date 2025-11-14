#include "CameraComponent.hpp"
#include "TransformComponent.hpp"
#include "../Entity.hpp"

namespace GameEngine {

    CameraComponent::CameraComponent() {
    }

    nlohmann::json CameraComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        j["isMainCamera"] = IsMainCamera;
        j["fov"] = FOV;
        j["aspectRatio"] = AspectRatio;
        j["nearClip"] = NearClip;
        j["farClip"] = FarClip;
        j["projectionType"] = static_cast<int>(ProjectionType);
        return j;
    }

    void CameraComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("isMainCamera"))
            IsMainCamera = data["isMainCamera"];
        
        if (data.contains("fov"))
            FOV = data["fov"];
        
        if (data.contains("aspectRatio"))
            AspectRatio = data["aspectRatio"];
        
        if (data.contains("nearClip"))
            NearClip = data["nearClip"];
        
        if (data.contains("farClip"))
            FarClip = data["farClip"];
        
        if (data.contains("projectionType"))
            ProjectionType = static_cast<CameraProjectionType>(data["projectionType"].get<int>());
    }

    void CameraComponent::OnCreate() {
        m_Camera = CreateRef<Camera3D>(FOV, AspectRatio, NearClip, FarClip);
        
        // Sync with transform
        if (Owner && Owner->HasComponent<TransformComponent>()) {
            auto& transform = Owner->GetComponent<TransformComponent>();
            m_Camera->SetPosition(transform.GetWorldPosition());
            
            // Set rotation from transform
            glm::vec3 euler = transform.GetEulerAngles();
            m_Camera->SetRotation(euler.x, euler.y, euler.z);
        }
    }

    void CameraComponent::OnUpdate(float deltaTime) {
        if (!m_Camera || !Owner) return;
        
        // Update camera properties
        m_Camera->SetFOV(FOV);
        m_Camera->SetAspectRatio(AspectRatio);
        
        // Sync with transform
        if (Owner->HasComponent<TransformComponent>()) {
            auto& transform = Owner->GetComponent<TransformComponent>();
            m_Camera->SetPosition(transform.GetWorldPosition());
            
            // Set rotation from transform
            glm::vec3 euler = transform.GetEulerAngles();
            m_Camera->SetRotation(euler.x, euler.y, euler.z);
        }
    }
}