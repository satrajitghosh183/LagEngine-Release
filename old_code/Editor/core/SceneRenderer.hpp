#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <memory>
#include <string>

namespace editor {

/**
 * @brief Simple mesh data for rendering primitives
 */
struct SimpleMesh {
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    std::vector<float> vertices;  // pos(3) + normal(3)
    std::vector<unsigned int> indices;
    
    void upload();
    void draw() const;
    void destroy();
};

/**
 * @brief Transform component for scene objects
 */
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // Euler angles in degrees
    glm::vec3 scale{1.0f};
    
    glm::mat4 getMatrix() const;
};

/**
 * @brief A simple scene object for the editor
 */
struct SceneObject {
    std::string name;
    std::string type;  // "Cube", "Sphere", "Plane", etc.
    Transform transform;
    glm::vec3 color{0.8f, 0.8f, 0.8f};
    bool visible = true;
    bool selected = false;
    int meshIndex = -1;  // Index into mesh array
};

/**
 * @brief Simple camera for viewport rendering
 */
class EditorCamera {
public:
    EditorCamera();
    
    void update(float dt);
    void processMouseMovement(float xoffset, float yoffset);
    void processMouseScroll(float yoffset);
    void moveForward(float delta);
    void moveRight(float delta);
    void moveUp(float delta);
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    
    void focusOn(const glm::vec3& target);
    
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float moveSpeed = 5.0f;
    float sensitivity = 0.1f;

private:
    glm::vec3 position{0.0f, 5.0f, 10.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    
    float yaw = -90.0f;
    float pitch = -20.0f;
    
    void updateVectors();
};

/**
 * @brief Main scene renderer for the editor viewport
 */
class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();
    
    bool initialize();
    void shutdown();
    
    // Rendering
    void render(int viewportWidth, int viewportHeight);
    void renderGrid();
    void renderObjects();
    void renderGizmo(const SceneObject& obj);
    
    // Scene management
    int addObject(const std::string& name, const std::string& type);
    void removeObject(int index);
    SceneObject* getObject(int index);
    const std::vector<SceneObject>& getObjects() const { return m_objects; }
    std::vector<SceneObject>& getObjects() { return m_objects; }
    
    // Selection
    void selectObject(int index);
    void clearSelection();
    int getSelectedIndex() const { return m_selectedIndex; }
    SceneObject* getSelectedObject();
    
    // Camera access
    EditorCamera& getCamera() { return m_camera; }
    
    // Settings
    void setShowGrid(bool show) { m_showGrid = show; }
    void setShowWireframe(bool show) { m_showWireframe = show; }
    void setGridSize(float size) { m_gridSize = size; }

private:
    bool createShaders();
    void createPrimitiveMeshes();
    SimpleMesh createCubeMesh();
    SimpleMesh createSphereMesh(int segments = 32);
    SimpleMesh createPlaneMesh();
    SimpleMesh createGridMesh(int lines = 20, float spacing = 1.0f);
    
    // Shaders
    unsigned int m_shaderProgram = 0;
    unsigned int m_gridShaderProgram = 0;
    
    // Meshes
    std::vector<SimpleMesh> m_meshes;  // 0=cube, 1=sphere, 2=plane
    SimpleMesh m_gridMesh;
    
    // Scene objects
    std::vector<SceneObject> m_objects;
    int m_selectedIndex = -1;
    
    // Camera
    EditorCamera m_camera;
    
    // Settings
    bool m_showGrid = true;
    bool m_showWireframe = false;
    float m_gridSize = 20.0f;
    
    // Light
    glm::vec3 m_lightPos{5.0f, 10.0f, 5.0f};
    glm::vec3 m_lightColor{1.0f, 1.0f, 1.0f};
    
    bool m_initialized = false;
};

} // namespace editor

