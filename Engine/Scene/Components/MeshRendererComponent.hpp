#pragma once

#include "Component.hpp"
#include "../../Graphics/Mesh3D.hpp"
#include "../../Graphics/Material.hpp"

namespace GameEngine {

    /**
     * @brief Mesh renderer component
     * 
     * Renders a 3D mesh with a material
     */
    class MeshRendererComponent : public Component {
    public:
        MeshRendererComponent() = default;
        MeshRendererComponent(const Ref<Mesh3D>& mesh, const Ref<Material>& material);
        
        COMPONENT_TYPE(MeshRendererComponent)
        
        nlohmann::json Serialize() const override;
        void Deserialize(const nlohmann::json& data) override;
        
        /**
         * @brief Set mesh
         */
        void SetMesh(const Ref<Mesh3D>& mesh) { m_Mesh = mesh; }
        
        /**
         * @brief Get mesh
         */
        Ref<Mesh3D> GetMesh() const { return m_Mesh; }
        
        /**
         * @brief Set material
         */
        void SetMaterial(const Ref<Material>& material) { m_Material = material; }
        
        /**
         * @brief Get material
         */
        Ref<Material> GetMaterial() const { return m_Material; }
        
        /**
         * @brief Render settings
         */
        bool CastShadows = true;
        bool ReceiveShadows = true;
        int RenderLayer = 0;
        
    private:
        Ref<Mesh3D> m_Mesh;
        Ref<Material> m_Material;
    };
}