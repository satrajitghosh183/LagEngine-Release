#pragma once

#include "Component.hpp"
#include "../../Graphics/Camera3D.hpp"

namespace GameEngine {

    /**
     * @brief Camera component
     * 
     * Makes an entity act as a camera
     */
    class CameraComponent : public Component {
    public:
        CameraComponent();
        
        COMPONENT_TYPE(CameraComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        void OnCreate() override;
        void OnUpdate(float deltaTime) override;
        
        /**
         * @brief Get camera
         */
        Ref<Camera3D> GetCamera() const { return m_Camera; }
        
        /**
         * @brief Set as active camera
         */
        bool IsMainCamera = false;
        
        /**
         * @brief Camera properties
         */
        float FOV = 45.0f;
        float AspectRatio = 16.0f / 9.0f;
        float NearClip = 0.1f;
        float FarClip = 1000.0f;
        
        CameraProjectionType ProjectionType = CameraProjectionType::Perspective;
        
    private:
        Ref<Camera3D> m_Camera;
    };
}