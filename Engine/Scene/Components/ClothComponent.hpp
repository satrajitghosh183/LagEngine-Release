#pragma once

#include "Component.hpp"
#include "../../Physics/SoftBody/SoftBody.hpp"
#include "../../Graphics/Mesh3D.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace GameEngine {

    /**
     * @brief Component for cloth simulation
     * 
     * Attaches a cloth simulation to an entity with automatic mesh updating.
     */
    class ClothComponent : public Component {
    public:
        ClothComponent() = default;
        ~ClothComponent() = default;
        
        COMPONENT_TYPE(ClothComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Initialize cloth with dimensions
         */
        void Initialize(float width, float height, int resolutionX, int resolutionY);
        
        /**
         * @brief Get the cloth body
         */
        Ref<Physics::ClothBody> GetClothBody() const { return m_ClothBody; }
        
        /**
         * @brief Get/Set mesh for rendering
         */
        void SetMesh(const Ref<Mesh3D>& mesh) { m_Mesh = mesh; }
        Ref<Mesh3D> GetMesh() const { return m_Mesh; }
        
        /**
         * @brief Update the mesh vertices from cloth particles
         */
        void UpdateMesh();
        
        /**
         * @brief Cloth properties
         */
        float Mass = 1.0f;
        float Stiffness = 50.0f;
        float Damping = 0.98f;
        float WindStrength = 1.0f;
        glm::vec3 WindDirection = glm::vec3(1, 0, 0);
        bool WindEnabled = false;
        
        /**
         * @brief Pin corners (top-left and top-right by default)
         */
        bool PinTopLeft = true;
        bool PinTopRight = true;
        bool PinBottomLeft = false;
        bool PinBottomRight = false;
        
        /**
         * @brief Simulation state
         */
        bool IsSimulating = true;
        
        /**
         * @brief Called when component is created
         */
        void OnCreate();
        
        /**
         * @brief Called every fixed update
         */
        void OnFixedUpdate(float fixedDeltaTime);
        
        /**
         * @brief Called when component is destroyed
         */
        void OnDestroy();
        
    private:
        Ref<Physics::ClothBody> m_ClothBody;
        Ref<Mesh3D> m_Mesh;
        
        int m_ResolutionX = 10;
        int m_ResolutionY = 10;
        float m_Width = 2.0f;
        float m_Height = 2.0f;
    };

}
