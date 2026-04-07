#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/Terrain.hpp"
#include "../../Engine/Graphics/Shader.hpp"
#include "../../Engine/Graphics/Camera3D.hpp"
#include "../../Engine/Graphics/RenderCommand.hpp"
#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

using namespace GameEngine;

class TerrainDemoApp : public Application {
public:
    TerrainDemoApp() : Application("Terrain Demo") {}

protected:
    void OnInit() override {
        // Generate procedural terrain
        m_Terrain = CreateRef<Terrain>();
        m_Terrain->generateFromNoise(256.0f, 256.0f, 128, 42);

        GE_CORE_INFO("Terrain generated: {}x{}, {} vertices",
                      m_Terrain->getWidth(), m_Terrain->getDepth(), m_Terrain->getResolution());
    }

    void OnUpdate(float dt) override {
        // Simple camera control
        float speed = 20.0f * dt;
        if (GetWindow().IsKeyPressed(GLFW_KEY_W)) m_CameraPos.z -= speed;
        if (GetWindow().IsKeyPressed(GLFW_KEY_S)) m_CameraPos.z += speed;
        if (GetWindow().IsKeyPressed(GLFW_KEY_A)) m_CameraPos.x -= speed;
        if (GetWindow().IsKeyPressed(GLFW_KEY_D)) m_CameraPos.x += speed;
        if (GetWindow().IsKeyPressed(GLFW_KEY_Q)) m_CameraPos.y -= speed;
        if (GetWindow().IsKeyPressed(GLFW_KEY_E)) m_CameraPos.y += speed;

        // Keep camera above terrain
        float terrainH = m_Terrain->getHeightAt(m_CameraPos.x, m_CameraPos.z);
        if (m_CameraPos.y < terrainH + 5.0f) {
            m_CameraPos.y = terrainH + 5.0f;
        }
    }

    void OnRender() override {
        if (!m_Terrain) return;

        // Render terrain
        m_Terrain->render();
    }

    void OnImGuiRender() override {
        ImGui::Begin("Terrain Demo");
        ImGui::Text("Camera: (%.1f, %.1f, %.1f)", m_CameraPos.x, m_CameraPos.y, m_CameraPos.z);
        ImGui::Text("WASD - Move, QE - Up/Down");

        float h = m_Terrain ? m_Terrain->getHeightAt(m_CameraPos.x, m_CameraPos.z) : 0;
        ImGui::Text("Ground height at camera: %.2f", h);

        if (ImGui::Button("Regenerate Terrain")) {
            m_Seed++;
            m_Terrain->generateFromNoise(256.0f, 256.0f, 128, m_Seed);
        }
        ImGui::End();
    }

private:
    Ref<Terrain> m_Terrain;
    glm::vec3 m_CameraPos = {0, 30, 50};
    uint32_t m_Seed = 42;
};

Application* GameEngine::CreateApplication() {
    return new TerrainDemoApp();
}
