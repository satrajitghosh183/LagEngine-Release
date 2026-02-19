#include "ClothComponent.hpp"
#include "TransformComponent.hpp"
#include "../Scene.hpp"
#include "../../Core/Logger.hpp"

namespace GameEngine {

    nlohmann::json ClothComponent::Serialize() const {
        nlohmann::json data;
        data["type"] = GetTypeName();
        data["width"] = m_Width;
        data["height"] = m_Height;
        data["resolutionX"] = m_ResolutionX;
        data["resolutionY"] = m_ResolutionY;
        data["mass"] = Mass;
        data["stiffness"] = Stiffness;
        data["damping"] = Damping;
        data["windStrength"] = WindStrength;
        data["windDirection"] = { WindDirection.x, WindDirection.y, WindDirection.z };
        data["windEnabled"] = WindEnabled;
        data["pinTopLeft"] = PinTopLeft;
        data["pinTopRight"] = PinTopRight;
        data["pinBottomLeft"] = PinBottomLeft;
        data["pinBottomRight"] = PinBottomRight;
        data["isSimulating"] = IsSimulating;
        return data;
    }
    
    void ClothComponent::Deserialize(const nlohmann::json& data) {
        if (data.contains("width")) m_Width = data["width"];
        if (data.contains("height")) m_Height = data["height"];
        if (data.contains("resolutionX")) m_ResolutionX = data["resolutionX"];
        if (data.contains("resolutionY")) m_ResolutionY = data["resolutionY"];
        if (data.contains("mass")) Mass = data["mass"];
        if (data.contains("stiffness")) Stiffness = data["stiffness"];
        if (data.contains("damping")) Damping = data["damping"];
        if (data.contains("windStrength")) WindStrength = data["windStrength"];
        if (data.contains("windDirection")) {
            WindDirection.x = data["windDirection"][0];
            WindDirection.y = data["windDirection"][1];
            WindDirection.z = data["windDirection"][2];
        }
        if (data.contains("windEnabled")) WindEnabled = data["windEnabled"];
        if (data.contains("pinTopLeft")) PinTopLeft = data["pinTopLeft"];
        if (data.contains("pinTopRight")) PinTopRight = data["pinTopRight"];
        if (data.contains("pinBottomLeft")) PinBottomLeft = data["pinBottomLeft"];
        if (data.contains("pinBottomRight")) PinBottomRight = data["pinBottomRight"];
        if (data.contains("isSimulating")) IsSimulating = data["isSimulating"];
    }

    void ClothComponent::Initialize(float width, float height, int resolutionX, int resolutionY) {
        m_Width = width;
        m_Height = height;
        m_ResolutionX = resolutionX;
        m_ResolutionY = resolutionY;
        
        // Create cloth body - ClothBody takes (width, height, spacing)
        float spacing = width / static_cast<float>(resolutionX);
        m_ClothBody = CreateRef<Physics::ClothBody>(resolutionX, resolutionY, spacing);
        
        // Set properties via public members
        m_ClothBody->Damping = Damping;
        
        // Apply pin settings using PinCorner
        if (PinTopLeft) {
            m_ClothBody->PinCorner(0);  // Top-left corner
        }
        if (PinTopRight) {
            m_ClothBody->PinCorner(1);  // Top-right corner
        }
        if (PinBottomLeft) {
            m_ClothBody->PinCorner(2);  // Bottom-left corner
        }
        if (PinBottomRight) {
            m_ClothBody->PinCorner(3);  // Bottom-right corner
        }
        
        GE_CORE_INFO("ClothComponent initialized: {}x{} resolution", resolutionX, resolutionY);
    }

    void ClothComponent::UpdateMesh() {
        if (!m_ClothBody || !m_Mesh) return;
        
        const auto& particles = m_ClothBody->GetSolver().GetParticles();
        
        // Build vertex data
        std::vector<Vertex3D> vertices;
        vertices.reserve(particles.size());
        
        for (int y = 0; y < m_ResolutionY; y++) {
            for (int x = 0; x < m_ResolutionX; x++) {
                int idx = y * m_ResolutionX + x;
                const auto& particle = particles[idx];
                
                Vertex3D vertex;
                vertex.Position = particle.Position;
                
                // Calculate normal from neighboring particles
                glm::vec3 normal(0.0f, 1.0f, 0.0f);
                
                if (x > 0 && y > 0 && x < m_ResolutionX - 1 && y < m_ResolutionY - 1) {
                    int left = idx - 1;
                    int right = idx + 1;
                    int up = idx - m_ResolutionX;
                    int down = idx + m_ResolutionX;
                    
                    glm::vec3 dx = particles[right].Position - particles[left].Position;
                    glm::vec3 dy = particles[down].Position - particles[up].Position;
                    
                    normal = glm::normalize(glm::cross(dx, dy));
                }
                
                vertex.Normal = normal;
                
                // UV coordinates
                vertex.TexCoords = glm::vec2(
                    static_cast<float>(x) / (m_ResolutionX - 1),
                    static_cast<float>(y) / (m_ResolutionY - 1)
                );
                
                vertices.push_back(vertex);
            }
        }
        
        // Update mesh vertices
        m_Mesh->UpdateVertices(vertices);
    }

    void ClothComponent::OnCreate() {
        // Initialize with default dimensions if not already done
        if (!m_ClothBody) {
            Initialize(m_Width, m_Height, m_ResolutionX, m_ResolutionY);
        }
        
        GE_CORE_DEBUG("ClothComponent::OnCreate");
    }

    void ClothComponent::OnFixedUpdate(float fixedDeltaTime) {
        if (!m_ClothBody || !IsSimulating) return;
        
        // Get entity transform via Owner pointer (set by Scene)
        if (Owner && Owner->HasComponent<TransformComponent>()) {
            auto& transform = Owner->GetComponent<TransformComponent>();
            // Position offset could be applied here
        }
        
        // Apply wind
        if (WindEnabled) {
            m_ClothBody->ApplyWind(WindDirection, WindStrength);
        }
        
        // Update cloth simulation
        m_ClothBody->Update(fixedDeltaTime);
        
        // Update mesh for rendering
        UpdateMesh();
    }

    void ClothComponent::OnDestroy() {
        m_ClothBody.reset();
        m_Mesh.reset();
        GE_CORE_DEBUG("ClothComponent::OnDestroy");
    }

}
