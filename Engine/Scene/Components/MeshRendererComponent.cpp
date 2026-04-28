#include "MeshRendererComponent.hpp"
#include "../../Assets/AssetDatabase.hpp"
#include "../../Graphics/MeshGenerator3D.hpp"

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

        // Serialize mesh reference
        if (m_Mesh) {
            nlohmann::json meshJson;
            // Check if mesh came from an asset file
            if (!m_Mesh->GetSourcePath().empty()) {
                auto guid = AssetDatabase::GetGUID(m_Mesh->GetSourcePath());
                if (guid.has_value()) {
                    meshJson["assetGUID"] = guid.value().ToString();
                }
                meshJson["path"] = m_Mesh->GetSourcePath();
            } else {
                // Procedural mesh - serialize by primitive name
                meshJson["primitive"] = m_Mesh->GetName();
            }
            j["mesh"] = meshJson;
        }

        // Serialize material properties inline
        if (m_Material) {
            nlohmann::json matJson;
            matJson["name"] = m_Material->GetName();

            auto albedo = m_Material->GetAlbedo();
            matJson["albedo"] = { albedo.x, albedo.y, albedo.z };
            matJson["metallic"] = m_Material->GetMetallic();
            matJson["roughness"] = m_Material->GetRoughness();
            matJson["ao"] = m_Material->GetAO();

            // Vulkan materials carry pipeline references rather than a Shader
            // object, so shader-name serialization is no longer applicable.

            j["material"] = matJson;
        }

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

        // Deserialize mesh reference
        if (data.contains("mesh")) {
            auto& meshJson = data["mesh"];

            if (meshJson.contains("assetGUID")) {
                // File-based mesh: look up by GUID in AssetDatabase
                GUID guid(meshJson["assetGUID"].get<std::string>());
                auto path = AssetDatabase::GetPath(guid);
                if (path.has_value()) {
                    m_MeshAssetPath = path.value();
                }
            } else if (meshJson.contains("path")) {
                m_MeshAssetPath = meshJson["path"].get<std::string>();
            } else if (meshJson.contains("primitive")) {
                // Recreate procedural mesh from primitive type
                std::string primitive = meshJson["primitive"];
                if (primitive == "Cube") {
                    m_Mesh = MeshGenerator3D::CreateCube(1.0f);
                } else if (primitive == "Sphere") {
                    m_Mesh = MeshGenerator3D::CreateSphere(0.5f, 32, 16);
                } else if (primitive == "Plane") {
                    m_Mesh = MeshGenerator3D::CreatePlane(5.0f, 5.0f);
                } else if (primitive == "Cylinder") {
                    m_Mesh = MeshGenerator3D::CreateCylinder(0.5f, 2.0f, 32);
                } else if (primitive == "Capsule") {
                    m_Mesh = MeshGenerator3D::CreateCapsule(0.5f, 2.0f, 32, 8);
                }
                if (m_Mesh) {
                    m_Mesh->SetName(primitive);
                }
            }
        }

        // Deserialize material properties
        if (data.contains("material")) {
            auto& matJson = data["material"];

            if (!m_Material) {
                m_Material = CreateRef<Material>();
            }

            if (matJson.contains("name"))
                m_Material->SetName(matJson["name"]);

            if (matJson.contains("albedo")) {
                auto& a = matJson["albedo"];
                m_Material->SetAlbedo(glm::vec3(a[0], a[1], a[2]));
            }
            if (matJson.contains("metallic"))
                m_Material->SetMetallic(matJson["metallic"]);
            if (matJson.contains("roughness"))
                m_Material->SetRoughness(matJson["roughness"]);
            if (matJson.contains("ao"))
                m_Material->SetAO(matJson["ao"]);

            // Shader is not loaded here - the editor or runtime will
            // assign a shader to the material after loading the scene.
            if (matJson.contains("shaderName"))
                m_ShaderName = matJson["shaderName"];
        }
    }
}
