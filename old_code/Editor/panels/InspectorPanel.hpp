#pragma once

#include "EditorPanel.hpp"
#include "../core/SceneRenderer.hpp"
#include <memory>
#include <string>
#include <functional>

namespace editor {

// Forward declaration
class SceneRenderer;

/**
 * @brief Inspector Panel - shows properties of selected objects
 * Similar to Unity's Inspector or LumixEngine's Properties panel
 */
class InspectorPanel : public EditorPanel {
public:
    InspectorPanel();
    
    void render() override;
    void update(float dt) override;
    
    // Scene renderer connection
    void setSceneRenderer(SceneRenderer* renderer) { m_sceneRenderer = renderer; }

private:
    // Component renderers
    void renderTransformComponent(SceneObject* obj);
    void renderAppearanceComponent(SceneObject* obj);
    void renderObjectInfo(SceneObject* obj);
    
    // Widget helpers
    bool drawVec3Control(const char* label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);
    bool drawFloatControl(const char* label, float* value, float min = 0.0f, float max = 100.0f, const char* format = "%.2f");
    bool drawColorControl(const char* label, glm::vec3& color);
    bool drawCheckbox(const char* label, bool* value);
    void drawReadonlyText(const char* label, const char* text);
    
    // Scene renderer reference
    SceneRenderer* m_sceneRenderer = nullptr;
};

} // namespace editor
