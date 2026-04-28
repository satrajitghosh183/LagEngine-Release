#include "SceneRenderer.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

namespace editor {

// ============ Transform Implementation ============

glm::mat4 Transform::getMatrix() const {
    glm::mat4 mat = glm::mat4(1.0f);
    mat = glm::translate(mat, position);
    mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0, 0, 1));
    mat = glm::scale(mat, scale);
    return mat;
}

// ============ EditorCamera Implementation ============

EditorCamera::EditorCamera() {
    updateVectors();
}

void EditorCamera::update(float /*dt*/) {
    updateVectors();
}

void EditorCamera::processMouseMovement(float xoffset, float yoffset) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Constrain pitch
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateVectors();
}

void EditorCamera::processMouseScroll(float yoffset) {
    position += front * yoffset * moveSpeed * 0.5f;
}

void EditorCamera::moveForward(float delta) {
    position += front * delta * moveSpeed;
}

void EditorCamera::moveRight(float delta) {
    position += right * delta * moveSpeed;
}

void EditorCamera::moveUp(float delta) {
    position += up * delta * moveSpeed;
}

glm::mat4 EditorCamera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 EditorCamera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void EditorCamera::focusOn(const glm::vec3& target) {
    glm::vec3 dir = target - position;
    float dist = glm::length(dir);
    if (dist < 0.01f) return;

    position = target - glm::normalize(dir) * 5.0f;
    front = glm::normalize(dir);

    pitch = glm::degrees(asin(front.y));
    yaw = glm::degrees(atan2(front.z, front.x));

    updateVectors();
}

void EditorCamera::updateVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, front));
}

// ============ SceneRenderer Implementation ============

SceneRenderer::SceneRenderer() {}

SceneRenderer::~SceneRenderer() {
    shutdown();
}

bool SceneRenderer::initialize() {
    if (m_initialized) return true;

    // Create a few default objects
    addObject("Ground", "Plane");
    m_objects[0].transform.scale = glm::vec3(10.0f, 1.0f, 10.0f);
    m_objects[0].color = glm::vec3(0.4f, 0.4f, 0.45f);

    addObject("Cube", "Cube");
    m_objects[1].transform.position = glm::vec3(0.0f, 0.5f, 0.0f);
    m_objects[1].color = glm::vec3(0.8f, 0.3f, 0.3f);

    addObject("Sphere", "Sphere");
    m_objects[2].transform.position = glm::vec3(3.0f, 1.0f, 0.0f);
    m_objects[2].color = glm::vec3(0.3f, 0.8f, 0.4f);

    addObject("Box", "Cube");
    m_objects[3].transform.position = glm::vec3(-3.0f, 0.5f, 2.0f);
    m_objects[3].color = glm::vec3(0.3f, 0.5f, 0.9f);

    m_initialized = true;
    std::cout << "[SceneRenderer] Initialized with " << m_objects.size() << " objects" << std::endl;
    return true;
}

void SceneRenderer::shutdown() {
    m_objects.clear();
    m_initialized = false;
}

int SceneRenderer::addObject(const std::string& name, const std::string& type) {
    SceneObject obj;
    obj.name = name;
    obj.type = type;

    if (type == "Cube") obj.meshIndex = 0;
    else if (type == "Sphere") obj.meshIndex = 1;
    else if (type == "Plane") obj.meshIndex = 2;
    else obj.meshIndex = 0;  // Default to cube

    m_objects.push_back(obj);
    return static_cast<int>(m_objects.size() - 1);
}

void SceneRenderer::removeObject(int index) {
    if (index >= 0 && index < (int)m_objects.size()) {
        m_objects.erase(m_objects.begin() + index);
        if (m_selectedIndex == index) m_selectedIndex = -1;
        else if (m_selectedIndex > index) m_selectedIndex--;
    }
}

SceneObject* SceneRenderer::getObject(int index) {
    if (index >= 0 && index < (int)m_objects.size()) {
        return &m_objects[index];
    }
    return nullptr;
}

void SceneRenderer::selectObject(int index) {
    // Deselect previous
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_objects.size()) {
        m_objects[m_selectedIndex].selected = false;
    }

    m_selectedIndex = index;

    // Select new
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_objects.size()) {
        m_objects[m_selectedIndex].selected = true;
    }
}

void SceneRenderer::clearSelection() {
    if (m_selectedIndex >= 0 && m_selectedIndex < (int)m_objects.size()) {
        m_objects[m_selectedIndex].selected = false;
    }
    m_selectedIndex = -1;
}

SceneObject* SceneRenderer::getSelectedObject() {
    return getObject(m_selectedIndex);
}

} // namespace editor
