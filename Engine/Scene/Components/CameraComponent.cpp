#include "CameraComponent.hpp"
#include "TransformComponent.hpp"
#include "../Scene.hpp"

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
        
        // Sync with transform using world matrix for proper rotation handling
        if (GetOwnerEntity() && GetOwnerEntity().HasComponent<TransformComponent>()) {
            auto& transform = GetOwnerEntity().GetComponent<TransformComponent>();
            m_Camera->SetFromWorldTransform(transform.GetWorldTransform());
        }
    }

    void CameraComponent::OnUpdate(float deltaTime) {
        if (!m_Camera || !GetOwnerEntity()) return;
        
        // Update camera properties
        m_Camera->SetFOV(FOV);
        m_Camera->SetAspectRatio(AspectRatio);
        
        // Sync with transform using world matrix for proper rotation handling
        if (GetOwnerEntity().HasComponent<TransformComponent>()) {
            auto& transform = GetOwnerEntity().GetComponent<TransformComponent>();
            m_Camera->SetFromWorldTransform(transform.GetWorldTransform());
        }
    }
}