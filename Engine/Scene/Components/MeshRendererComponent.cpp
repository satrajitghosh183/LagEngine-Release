#include "MeshRendererComponent.hpp"

namespace GameEngine {

    MeshRendererComponent::MeshRendererComponent(const Ref<Mesh3D>& mesh, const Ref<Material>& material)
        : m_Mesh(mesh), m_Material(material) {
    }

    nlohmann::json MeshRendererComponent::Serialize() const {
        nlohmann::json j;
        j["type"] = GetTypeName();
        j["enabled"] = Enabled;
        j["castShadows"] = CastShadows;
        j["receiveShadows"] = ReceiveShadows;
        j["renderLayer"] = RenderLayer;
        
        // TODO: Serialize mesh and material references (asset UUIDs)
        
        return j;
    }

    void MeshRendererComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("enabled"))
            Enabled = data["enabled"];
        
        if (data.contains("castShadows"))
            CastShadows = data["castShadows"];
        
        if (data.contains("receiveShadows"))
            ReceiveShadows = data["receiveShadows"];
        
        if (data.contains("renderLayer"))
            RenderLayer = data["renderLayer"];
        
        // TODO: Deserialize mesh and material references
    }
}