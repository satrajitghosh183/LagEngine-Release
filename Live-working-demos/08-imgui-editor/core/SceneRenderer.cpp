#include "SceneRenderer.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

namespace editor {

// ============ SimpleMesh Implementation ============

void SimpleMesh::upload() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    if (!indices.empty()) {
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    }
    
    glBindVertexArray(0);
}

void SimpleMesh::draw() const {
    glBindVertexArray(VAO);
    if (!indices.empty()) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size() / 6));
    }
    glBindVertexArray(0);
}

void SimpleMesh::destroy() {
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
}

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
    
    if (!createShaders()) {
        std::cerr << "[SceneRenderer] Failed to create shaders" << std::endl;
        return false;
    }
    
    createPrimitiveMeshes();
    
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
    for (auto& mesh : m_meshes) {
        mesh.destroy();
    }
    m_meshes.clear();
    m_gridMesh.destroy();
    
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    if (m_gridShaderProgram) glDeleteProgram(m_gridShaderProgram);
    m_shaderProgram = 0;
    m_gridShaderProgram = 0;
    
    m_initialized = false;
}

bool SceneRenderer::createShaders() {
    // Main object shader
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aNormal;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        
        out vec3 FragPos;
        out vec3 Normal;
        
        void main() {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";
    
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec3 FragPos;
        in vec3 Normal;
        
        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 objectColor;
        uniform vec3 viewPos;
        uniform bool selected;
        
        void main() {
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;
            
            // Diffuse
            vec3 norm = normalize(Normal);
            vec3 lightDir = normalize(lightPos - FragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;
            
            // Specular
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            vec3 specular = specularStrength * spec * lightColor;
            
            vec3 result = (ambient + diffuse + specular) * objectColor;
            
            // Selection highlight
            if (selected) {
                result = mix(result, vec3(1.0, 0.8, 0.2), 0.2);
            }
            
            FragColor = vec4(result, 1.0);
        }
    )";
    
    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader error: " << infoLog << std::endl;
        return false;
    }
    
    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader error: " << infoLog << std::endl;
        return false;
    }
    
    // Link program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader link error: " << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Grid shader (simpler)
    const char* gridVertSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        uniform mat4 view;
        uniform mat4 projection;
        out vec3 FragPos;
        void main() {
            FragPos = aPos;
            gl_Position = projection * view * vec4(aPos, 1.0);
        }
    )";
    
    const char* gridFragSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 FragPos;
        void main() {
            float dist = length(FragPos.xz);
            float alpha = 1.0 - smoothstep(15.0, 25.0, dist);
            
            // Axis colors
            if (abs(FragPos.x) < 0.05) {
                FragColor = vec4(0.2, 0.2, 0.9, alpha);  // Z-axis blue
            } else if (abs(FragPos.z) < 0.05) {
                FragColor = vec4(0.9, 0.2, 0.2, alpha);  // X-axis red
            } else {
                FragColor = vec4(0.5, 0.5, 0.5, alpha * 0.5);  // Grid lines
            }
        }
    )";
    
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &gridVertSource, NULL);
    glCompileShader(vertexShader);
    
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &gridFragSource, NULL);
    glCompileShader(fragmentShader);
    
    m_gridShaderProgram = glCreateProgram();
    glAttachShader(m_gridShaderProgram, vertexShader);
    glAttachShader(m_gridShaderProgram, fragmentShader);
    glLinkProgram(m_gridShaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void SceneRenderer::createPrimitiveMeshes() {
    m_meshes.push_back(createCubeMesh());
    m_meshes.push_back(createSphereMesh(24));
    m_meshes.push_back(createPlaneMesh());
    
    for (auto& mesh : m_meshes) {
        mesh.upload();
    }
    
    m_gridMesh = createGridMesh(40, 1.0f);
    m_gridMesh.upload();
}

SimpleMesh SceneRenderer::createCubeMesh() {
    SimpleMesh mesh;
    
    // Cube vertices with normals
    mesh.vertices = {
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        // Right face
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
        // Left face
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
    };
    
    mesh.indices = {
        0, 1, 2, 2, 3, 0,       // Front
        4, 5, 6, 6, 7, 4,       // Back
        8, 9, 10, 10, 11, 8,    // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };
    
    return mesh;
}

SimpleMesh SceneRenderer::createSphereMesh(int segments) {
    SimpleMesh mesh;
    
    for (int y = 0; y <= segments; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSegment = (float)x / (float)segments;
            float ySegment = (float)y / (float)segments;
            float xPos = std::cos(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);
            float yPos = std::cos(ySegment * 3.14159f);
            float zPos = std::sin(xSegment * 2.0f * 3.14159f) * std::sin(ySegment * 3.14159f);
            
            // Position
            mesh.vertices.push_back(xPos * 0.5f);
            mesh.vertices.push_back(yPos * 0.5f);
            mesh.vertices.push_back(zPos * 0.5f);
            // Normal (same as position for unit sphere)
            mesh.vertices.push_back(xPos);
            mesh.vertices.push_back(yPos);
            mesh.vertices.push_back(zPos);
        }
    }
    
    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            mesh.indices.push_back((y + 1) * (segments + 1) + x);
            mesh.indices.push_back(y * (segments + 1) + x);
            mesh.indices.push_back(y * (segments + 1) + x + 1);
            
            mesh.indices.push_back((y + 1) * (segments + 1) + x);
            mesh.indices.push_back(y * (segments + 1) + x + 1);
            mesh.indices.push_back((y + 1) * (segments + 1) + x + 1);
        }
    }
    
    return mesh;
}

SimpleMesh SceneRenderer::createPlaneMesh() {
    SimpleMesh mesh;
    
    mesh.vertices = {
        // pos + normal
        -0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, 0.0f, -0.5f,  0.0f, 1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,  0.0f, 1.0f, 0.0f,
    };
    
    mesh.indices = {0, 1, 2, 2, 3, 0};
    
    return mesh;
}

SimpleMesh SceneRenderer::createGridMesh(int lines, float spacing) {
    SimpleMesh mesh;
    
    float halfSize = lines * spacing * 0.5f;
    
    for (int i = -lines/2; i <= lines/2; i++) {
        float pos = i * spacing;
        // X-parallel lines
        mesh.vertices.push_back(-halfSize); mesh.vertices.push_back(0); mesh.vertices.push_back(pos);
        mesh.vertices.push_back(0); mesh.vertices.push_back(1); mesh.vertices.push_back(0);
        mesh.vertices.push_back(halfSize); mesh.vertices.push_back(0); mesh.vertices.push_back(pos);
        mesh.vertices.push_back(0); mesh.vertices.push_back(1); mesh.vertices.push_back(0);
        
        // Z-parallel lines
        mesh.vertices.push_back(pos); mesh.vertices.push_back(0); mesh.vertices.push_back(-halfSize);
        mesh.vertices.push_back(0); mesh.vertices.push_back(1); mesh.vertices.push_back(0);
        mesh.vertices.push_back(pos); mesh.vertices.push_back(0); mesh.vertices.push_back(halfSize);
        mesh.vertices.push_back(0); mesh.vertices.push_back(1); mesh.vertices.push_back(0);
    }
    
    return mesh;
}

void SceneRenderer::render(int viewportWidth, int viewportHeight) {
    if (!m_initialized) {
        initialize();
    }
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    float aspectRatio = (float)viewportWidth / (float)viewportHeight;
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 projection = m_camera.getProjectionMatrix(aspectRatio);
    
    // Render grid
    if (m_showGrid) {
        renderGrid();
    }
    
    // Render objects
    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightPos"), 1, glm::value_ptr(m_lightPos));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "lightColor"), 1, glm::value_ptr(m_lightColor));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "viewPos"), 1, glm::value_ptr(m_camera.getPosition()));
    
    renderObjects();
    
    glUseProgram(0);
}

void SceneRenderer::renderGrid() {
    glUseProgram(m_gridShaderProgram);
    
    float aspectRatio = 16.0f / 9.0f;  // Default, will be updated
    glm::mat4 view = m_camera.getViewMatrix();
    glm::mat4 projection = m_camera.getProjectionMatrix(aspectRatio);
    
    glUniformMatrix4fv(glGetUniformLocation(m_gridShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(m_gridShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glBindVertexArray(m_gridMesh.VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_gridMesh.vertices.size() / 6));
    glBindVertexArray(0);
    
    glUseProgram(0);
}

void SceneRenderer::renderObjects() {
    for (size_t i = 0; i < m_objects.size(); i++) {
        const auto& obj = m_objects[i];
        if (!obj.visible) continue;
        
        glm::mat4 model = obj.transform.getMatrix();
        glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(m_shaderProgram, "objectColor"), 1, glm::value_ptr(obj.color));
        glUniform1i(glGetUniformLocation(m_shaderProgram, "selected"), obj.selected ? 1 : 0);
        
        if (m_showWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        
        if (obj.meshIndex >= 0 && obj.meshIndex < (int)m_meshes.size()) {
            m_meshes[obj.meshIndex].draw();
        }
        
        if (m_showWireframe) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
    }
}

void SceneRenderer::renderGizmo(const SceneObject& /*obj*/) {
    // TODO: Implement transform gizmo rendering
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

