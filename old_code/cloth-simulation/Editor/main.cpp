
// /**
//  * ONE-FILE C++ GAME ENGINE + FAKE GODOT EDITOR (PROFESSIONAL LEVEL)
//  * 
//  * A complete game engine and editor in a single C++ file
//  * Features:
//  * - Godot-like Editor with full GUI layout
//  * - Entity system with balls, cloths, planes
//  * - Physics simulation
//  * - Scene hierarchy and inspector panels
//  * - Save/Load system (JSON)
//  * - Play mode for testing
//  * - AI scene generation placeholder
//  */

// // --- Includes ---
// #include <iostream>
// #include <vector>
// #include <string>
// #include <memory>
// #include <functional>
// #include <random>
// #include <algorithm>
// #include <fstream>
// #include <unordered_map>

// // OpenGL / Window handling
// #include <GL/glew.h>
// #include <GLFW/glfw3.h>

// #include "extern/imgui/imgui.h"
// #include "extern/imgui/backends/imgui_impl_glfw.h"
// #include "extern/imgui/backends/imgui_impl_opengl3.h"
// #include "extern/json/json.hpp"
// using json = nlohmann::json;

// // GLM Math Library
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
// #include <glm/gtx/quaternion.hpp>

// // --- Forward Declarations ---
// class Entity;
// class Scene;
// class PhysicsSystem;
// class EditorGUI;
// class Renderer;

// // --- Global Variables ---
// const int WINDOW_WIDTH = 1280;
// const int WINDOW_HEIGHT = 720;
// const char* WINDOW_TITLE = "Mini-Godot Editor";

// GLFWwindow* g_Window = nullptr;
// bool g_IsPlaying = false;
// bool g_IsDragging = false;
// bool g_IsMouseDown = false;
// double g_MouseX = 0, g_MouseY = 0;
// double g_MouseDeltaX = 0, g_MouseDeltaY = 0;
// glm::vec3 g_CameraPosition = glm::vec3(0.0f, 5.0f, 10.0f);
// glm::vec3 g_CameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
// float g_CameraZoom = 1.0f;
// float g_DeltaTime = 0.016f; // Default to 60fps
// std::string g_ConsoleOutput = "Mini-Godot Editor initialized.\nReady to create.\n";

// // --- Basic Shader Programs ---
// GLuint g_BasicShader = 0;
// const char* g_VertexShaderSrc = R"(
//     #version 330 core
//     layout (location = 0) in vec3 aPos;
//     layout (location = 1) in vec3 aNormal;
    
//     out vec3 FragPos;
//     out vec3 Normal;
    
//     uniform mat4 model;
//     uniform mat4 view;
//     uniform mat4 projection;
//     uniform vec3 objectColor;
    
//     out vec3 Color;
    
//     void main() {
//         FragPos = vec3(model * vec4(aPos, 1.0));
//         Normal = mat3(transpose(inverse(model))) * aNormal;
//         Color = objectColor;
//         gl_Position = projection * view * model * vec4(aPos, 1.0);
//     }
// )";

// const char* g_FragmentShaderSrc = R"(
//     #version 330 core
//     in vec3 FragPos;
//     in vec3 Normal;
//     in vec3 Color;
    
//     out vec4 FragColor;
    
//     uniform vec3 lightPos;
//     uniform vec3 lightColor;
//     uniform vec3 objectColor;
//     uniform vec3 viewPos;
    
//     void main() {
//         // Ambient
//         float ambientStrength = 0.3;
//         vec3 ambient = ambientStrength * lightColor;
        
//         // Diffuse
//         vec3 norm = normalize(Normal);
//         vec3 lightDir = normalize(lightPos - FragPos);
//         float diff = max(dot(norm, lightDir), 0.0);
//         vec3 diffuse = diff * lightColor;
        
//         // Specular
//         float specularStrength = 0.5;
//         vec3 viewDir = normalize(viewPos - FragPos);
//         vec3 reflectDir = reflect(-lightDir, norm);
//         float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
//         vec3 specular = specularStrength * spec * lightColor;
        
//         vec3 result = (ambient + diffuse + specular) * Color;
//         FragColor = vec4(result, 1.0);
//     }
// )";

// // --- Entity Structs and Classes ---

// // Entity Type Enum
// enum class EntityType {
//     BALL,
//     CLOTH,
//     PLANE
// };

// // Physics Properties struct
// struct PhysicsProperties {
//     bool isStatic = false;
//     float mass = 1.0f;
//     float restitution = 0.7f; // Bounciness
//     float friction = 0.3f;
//     bool useGravity = true;
//     glm::vec3 velocity = glm::vec3(0.0f);
//     glm::vec3 acceleration = glm::vec3(0.0f);
// };

// // Base Entity Class
// class Entity {
// public:
//     int id;
//     std::string name;
//     EntityType type;
//     glm::vec3 position;
//     glm::vec3 rotation; // Euler angles in degrees
//     glm::vec3 scale;
//     glm::vec4 color;
//     PhysicsProperties physics;
//     bool isSelected = false;
    
//     // Specific data for different entity types
//     // For cloth
//     int clothResolution = 10; // Number of particles per side
//     float clothSize = 5.0f;
//     std::vector<glm::vec3> clothVertices;
//     std::vector<glm::vec3> clothVelocities;
//     std::vector<glm::vec3> clothForces;
//     std::vector<std::pair<int, int>> clothSprings;
//     float clothStiffness = 100.0f;
//     float clothDamping = 0.01f;
    
//     // For rendering
//     GLuint VAO = 0, VBO = 0, EBO = 0;
//     int vertexCount = 0;
//     int indexCount = 0;
    
//     Entity(int _id, const std::string& _name, EntityType _type) 
//         : id(_id), name(_name), type(_type), 
//           position(0.0f), rotation(0.0f), scale(1.0f),
//           color(1.0f, 1.0f, 1.0f, 1.0f) {
//         // Initialize based on type
//         switch (type) {
//             case EntityType::BALL:
//                 scale = glm::vec3(1.0f);
//                 color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
//                 physics.mass = 1.0f;
//                 physics.useGravity = true;
//                 break;
                
//             case EntityType::CLOTH:
//                 scale = glm::vec3(5.0f, 0.1f, 5.0f);
//                 color = glm::vec4(0.2f, 0.6f, 0.8f, 1.0f);
//                 physics.mass = 0.1f;
//                 physics.useGravity = true;
//                 initializeCloth();
//                 break;
                
//             case EntityType::PLANE:
//                 scale = glm::vec3(10.0f, 0.1f, 10.0f);
//                 color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
//                 physics.isStatic = true;
//                 physics.useGravity = false;
//                 break;
//         }
//     }
    
//     ~Entity() {
//         cleanupGL();
//     }
    
//     void cleanupGL() {
//         if (VAO) glDeleteVertexArrays(1, &VAO);
//         if (VBO) glDeleteBuffers(1, &VBO);
//         if (EBO) glDeleteBuffers(1, &EBO);
//         VAO = VBO = EBO = 0;
//     }
    
//     void initializeCloth() {
//         // Create cloth mesh
//         clothVertices.clear();
//         clothVelocities.clear();
//         clothForces.clear();
//         clothSprings.clear();
        
//         float step = clothSize / (clothResolution - 1);
        
//         // Create vertices
//         for (int y = 0; y < clothResolution; y++) {
//             for (int x = 0; x < clothResolution; x++) {
//                 float xPos = x * step - clothSize / 2.0f;
//                 float yPos = 0.0f;
//                 float zPos = y * step - clothSize / 2.0f;
                
//                 clothVertices.push_back(glm::vec3(xPos, yPos, zPos));
//                 clothVelocities.push_back(glm::vec3(0.0f));
//                 clothForces.push_back(glm::vec3(0.0f));
//             }
//         }
        
//         // Create springs
//         for (int y = 0; y < clothResolution; y++) {
//             for (int x = 0; x < clothResolution; x++) {
//                 int i = y * clothResolution + x;
                
//                 // Structural springs (horizontal and vertical)
//                 if (x < clothResolution - 1)
//                     clothSprings.push_back(std::make_pair(i, i + 1));
//                 if (y < clothResolution - 1)
//                     clothSprings.push_back(std::make_pair(i, i + clothResolution));
                
//                 // Shear springs (diagonal)
//                 if (x < clothResolution - 1 && y < clothResolution - 1) {
//                     clothSprings.push_back(std::make_pair(i, i + clothResolution + 1));
//                     clothSprings.push_back(std::make_pair(i + 1, i + clothResolution));
//                 }
//             }
//         }
        
//         // Pin the top corners
//         if (clothVertices.size() >= clothResolution) {
//             int topLeft = 0;
//             int topRight = clothResolution - 1;
//             clothVelocities[topLeft] = glm::vec3(0.0f);
//             clothVelocities[topRight] = glm::vec3(0.0f);
//         }
//     }
    
//     void updateTransform() {
//         // Update any internal data that depends on transform
//         if (type == EntityType::CLOTH) {
//             // We don't update cloth vertices here because that's done in the physics system
//         }
//     }
    
//     void generateRenderingData(Renderer* renderer);
    
//     json serialize() const {
//         json j;
//         j["id"] = id;
//         j["name"] = name;
//         j["type"] = static_cast<int>(type);
        
//         // Transform
//         j["position"] = {position.x, position.y, position.z};
//         j["rotation"] = {rotation.x, rotation.y, rotation.z};
//         j["scale"] = {scale.x, scale.y, scale.z};
//         j["color"] = {color.r, color.g, color.b, color.a};
        
//         // Physics
//         j["physics"]["isStatic"] = physics.isStatic;
//         j["physics"]["mass"] = physics.mass;
//         j["physics"]["restitution"] = physics.restitution;
//         j["physics"]["friction"] = physics.friction;
//         j["physics"]["useGravity"] = physics.useGravity;
        
//         // Type-specific data
//         if (type == EntityType::CLOTH) {
//             j["clothResolution"] = clothResolution;
//             j["clothSize"] = clothSize;
//             j["clothStiffness"] = clothStiffness;
//             j["clothDamping"] = clothDamping;
//         }
        
//         return j;
//     }
    
//     static std::shared_ptr<Entity> deserialize(const json& j) {
//         int id = j["id"];
//         std::string name = j["name"];
//         EntityType type = static_cast<EntityType>(j["type"].get<int>());
        
//         auto entity = std::make_shared<Entity>(id, name, type);
        
//         // Transform
//         auto pos = j["position"];
//         entity->position = glm::vec3(pos[0], pos[1], pos[2]);
        
//         auto rot = j["rotation"];
//         entity->rotation = glm::vec3(rot[0], rot[1], rot[2]);
        
//         auto scl = j["scale"];
//         entity->scale = glm::vec3(scl[0], scl[1], scl[2]);
        
//         auto col = j["color"];
//         entity->color = glm::vec4(col[0], col[1], col[2], col[3]);
        
//         // Physics
//         entity->physics.isStatic = j["physics"]["isStatic"];
//         entity->physics.mass = j["physics"]["mass"];
//         entity->physics.restitution = j["physics"]["restitution"];
//         entity->physics.friction = j["physics"]["friction"];
//         entity->physics.useGravity = j["physics"]["useGravity"];
        
//         // Type-specific data
//         if (type == EntityType::CLOTH) {
//             entity->clothResolution = j["clothResolution"];
//             entity->clothSize = j["clothSize"];
//             entity->clothStiffness = j["clothStiffness"];
//             entity->clothDamping = j["clothDamping"];
//             entity->initializeCloth();
//         }
        
//         return entity;
//     }
// };

// // --- Scene Class ---
// class Scene {
// private:
//     std::vector<std::shared_ptr<Entity>> entities;
//     int nextEntityId = 1;
//     std::shared_ptr<Entity> selectedEntity = nullptr;
    
// public:
//     Scene() {}
    
//     std::shared_ptr<Entity> addEntity(const std::string& name, EntityType type) {
//         auto entity = std::make_shared<Entity>(nextEntityId++, name, type);
//         entities.push_back(entity);
//         g_ConsoleOutput += "Added new " + name + " to scene.\n";
//         return entity;
//     }
    
//     void removeEntity(int id) {
//         auto it = std::find_if(entities.begin(), entities.end(), 
//                              [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
//         if (it != entities.end()) {
//             if (selectedEntity && selectedEntity->id == id) {
//                 selectedEntity = nullptr;
//             }
//             g_ConsoleOutput += "Removed " + (*it)->name + " from scene.\n";
//             entities.erase(it);
//         }
//     }
    
//     std::shared_ptr<Entity> getEntity(int id) {
//         auto it = std::find_if(entities.begin(), entities.end(), 
//                              [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
//         if (it != entities.end()) {
//             return *it;
//         }
//         return nullptr;
//     }
    
//     std::vector<std::shared_ptr<Entity>>& getEntities() {
//         return entities;
//     }
    
//     void clear() {
//         entities.clear();
//         selectedEntity = nullptr;
//         nextEntityId = 1;
//         g_ConsoleOutput += "Scene cleared.\n";
//     }
    
//     std::shared_ptr<Entity> getSelectedEntity() const {
//         return selectedEntity;
//     }
    
//     void setSelectedEntity(std::shared_ptr<Entity> entity) {
//         // Deselect previous entity
//         if (selectedEntity) {
//             selectedEntity->isSelected = false;
//         }
        
//         selectedEntity = entity;
        
//         // Select new entity
//         if (selectedEntity) {
//             selectedEntity->isSelected = true;
//             g_ConsoleOutput += "Selected: " + selectedEntity->name + "\n";
//         }
//     }
    
//     bool selectEntityAtScreenPos(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection);
    
//     json serialize() const {
//         json j;
//         j["nextEntityId"] = nextEntityId;
        
//         json entitiesJson = json::array();
//         for (const auto& entity : entities) {
//             entitiesJson.push_back(entity->serialize());
//         }
//         j["entities"] = entitiesJson;
        
//         return j;
//     }
    
//     void deserialize(const json& j) {
//         clear();
        
//         nextEntityId = j["nextEntityId"];
        
//         for (const auto& entityJson : j["entities"]) {
//             auto entity = Entity::deserialize(entityJson);
//             entities.push_back(entity);
//         }
        
//         g_ConsoleOutput += "Scene loaded with " + std::to_string(entities.size()) + " entities.\n";
//     }
    
//     void saveToFile(const std::string& filename) {
//         json j = serialize();
//         std::ofstream file(filename);
//         if (file.is_open()) {
//             file << j.dump(4); // Pretty print with 4-space indent
//             file.close();
//             g_ConsoleOutput += "Scene saved to " + filename + "\n";
//         } else {
//             g_ConsoleOutput += "ERROR: Failed to save scene to " + filename + "\n";
//         }
//     }
    
//     bool loadFromFile(const std::string& filename) {
//         std::ifstream file(filename);
//         if (file.is_open()) {
//             try {
//                 json j;
//                 file >> j;
//                 deserialize(j);
//                 g_ConsoleOutput += "Scene loaded from " + filename + "\n";
//                 return true;
//             } catch (const std::exception& e) {
//                 g_ConsoleOutput += "ERROR: Failed to parse scene file: " + std::string(e.what()) + "\n";
//                 return false;
//             }
//         } else {
//             g_ConsoleOutput += "ERROR: Failed to open scene file: " + filename + "\n";
//             return false;
//         }
//     }
    
//     void generatedRandomScene() {
//         clear();
        
//         std::random_device rd;
//         std::mt19937 gen(rd());
//         std::uniform_real_distribution<> posDist(-5.0, 5.0);
//         std::uniform_real_distribution<> heightDist(2.0, 8.0);
//         std::uniform_real_distribution<> colorDist(0.1, 0.9);
//         std::uniform_real_distribution<> scaleDist(0.5, 2.0);
        
//         // Add ground plane
//         auto plane = addEntity("Ground", EntityType::PLANE);
//         plane->position = glm::vec3(0.0f, -0.05f, 0.0f);
//         plane->scale = glm::vec3(15.0f, 0.1f, 15.0f);
//         plane->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        
//         // Add some balls
//         int numBalls = 5 + (gen() % 5); // 5-9 balls
//         for (int i = 0; i < numBalls; i++) {
//             auto ball = addEntity("Ball_" + std::to_string(i+1), EntityType::BALL);
//             ball->position = glm::vec3(posDist(gen), heightDist(gen), posDist(gen));
//             ball->scale = glm::vec3(scaleDist(gen));
//             ball->color = glm::vec4(colorDist(gen), colorDist(gen), colorDist(gen), 1.0f);
//         }
        
//         // Add a cloth
//         auto cloth = addEntity("Cloth", EntityType::CLOTH);
//         cloth->position = glm::vec3(0.0f, 5.0f, 0.0f);
//         cloth->scale = glm::vec3(5.0f, 0.1f, 5.0f);
//         cloth->color = glm::vec4(0.2f, 0.6f, 0.8f, 1.0f);
//         cloth->clothResolution = 15; // Higher resolution for nicer cloth
//         cloth->initializeCloth();
        
//         g_ConsoleOutput += "AI Agent: Generated random scene with " + 
//                           std::to_string(numBalls) + " balls and 1 cloth.\n";
//     }
// };

// // --- Physics System ---
// class PhysicsSystem {
// private:
//     const float GRAVITY = -9.81f;
//     const int MAX_SUBSTEPS = 5;
//     const float FIXED_TIMESTEP = 0.016f; // 60Hz physics
    
// public:
//     PhysicsSystem() {}
    
//     void update(Scene& scene, float deltaTime) {
//         if (!g_IsPlaying) return; // Only simulate in play mode
        
//         // Apply substeps for more stable physics
//         float remainingTime = deltaTime;
//         int numSubsteps = std::min(MAX_SUBSTEPS, static_cast<int>(std::ceil(deltaTime / FIXED_TIMESTEP)));
//         float subDelta = deltaTime / numSubsteps;
        
//         for (int step = 0; step < numSubsteps; step++) {
//             updateEntities(scene, subDelta);
//             resolveCollisions(scene);
//         }
//     }
    
//     void updateEntities(Scene& scene, float deltaTime) {
//         for (auto& entity : scene.getEntities()) {
//             if (entity->physics.isStatic) continue;
            
//             // Apply gravity
//             if (entity->physics.useGravity) {
//                 entity->physics.acceleration.y = GRAVITY;
//             }
            
//             if (entity->type == EntityType::BALL) {
//                 // Simple euler integration for balls
//                 entity->physics.velocity += entity->physics.acceleration * deltaTime;
//                 entity->position += entity->physics.velocity * deltaTime;
//             }
//             else if (entity->type == EntityType::CLOTH) {
//                 updateCloth(entity, deltaTime);
//             }
//         }
//     }
    
//     void updateCloth(std::shared_ptr<Entity> cloth, float deltaTime) {
//         // Reset forces
//         for (auto& force : cloth->clothForces) {
//             force = glm::vec3(0.0f);
//         }
        
//         // Apply gravity to each particle
//         for (int i = 0; i < cloth->clothVertices.size(); i++) {
//             cloth->clothForces[i].y += GRAVITY * cloth->physics.mass;
//         }
        
//         // Top row particles are fixed (pinned)
//         for (int i = 0; i < cloth->clothResolution; i++) {
//             cloth->clothForces[i] = glm::vec3(0.0f);
//         }
        
//         // Apply spring forces
//         for (const auto& spring : cloth->clothSprings) {
//             int i1 = spring.first;
//             int i2 = spring.second;
            
//             glm::vec3 pos1 = cloth->clothVertices[i1] + cloth->position;
//             glm::vec3 pos2 = cloth->clothVertices[i2] + cloth->position;
            
//             glm::vec3 vel1 = cloth->clothVelocities[i1];
//             glm::vec3 vel2 = cloth->clothVelocities[i2];
            
//             glm::vec3 deltaPos = pos2 - pos1;
//             float distance = glm::length(deltaPos);
            
//             // Get original rest length (approximate as grid spacing)
//             float restLength = cloth->clothSize / (cloth->clothResolution - 1);
            
//             // Spring force (Hooke's law)
//             glm::vec3 direction = distance > 0.0001f ? deltaPos / distance : glm::vec3(1, 0, 0);
//             float springForce = cloth->clothStiffness * (distance - restLength);
            
//             // Damping force (reduces oscillation)
//             glm::vec3 deltaVel = vel2 - vel1;
//             float dampingForce = cloth->clothDamping * glm::dot(deltaVel, direction);
            
//             glm::vec3 force = direction * (springForce + dampingForce);
            
//             // Apply forces (equal and opposite)
//             if (i1 >= cloth->clothResolution) { // Don't move pinned particles
//                 cloth->clothForces[i1] += force;
//             }
//             if (i2 >= cloth->clothResolution) { // Don't move pinned particles
//                 cloth->clothForces[i2] -= force;
//             }
//         }
        
//         // Integrate forces
//         for (int i = 0; i < cloth->clothVertices.size(); i++) {
//             // Don't move pinned particles
//             if (i < cloth->clothResolution) {
//                 cloth->clothVelocities[i] = glm::vec3(0.0f);
//                 continue;
//             }
            
//             // Semi-implicit Euler integration
//             cloth->clothVelocities[i] += cloth->clothForces[i] * deltaTime / cloth->physics.mass;
//             cloth->clothVertices[i] += cloth->clothVelocities[i] * deltaTime;
            
//             // Add some damping to prevent explosion
//             cloth->clothVelocities[i] *= 0.99f;
//         }
//     }
    
//     void resolveCollisions(Scene& scene) {
//         auto& entities = scene.getEntities();
        
//         // Iterate through all entities
//         for (auto& entity : entities) {
//             if (entity->physics.isStatic) continue;
            
//             // Check for ball collisions with planes
//             if (entity->type == EntityType::BALL) {
//                 for (auto& other : entities) {
//                     if (other->type == EntityType::PLANE) {
//                         resolveCollisionBallPlane(entity, other);
//                     } else if (other->type == EntityType::BALL && other->id != entity->id) {
//                         resolveCollisionBallBall(entity, other);
//                     }
//                 }
//             }
//             // Check for cloth collisions with other objects
//             else if (entity->type == EntityType::CLOTH) {
//                 for (auto& other : entities) {
//                     if (other->type == EntityType::PLANE) {
//                         resolveCollisionClothPlane(entity, other);
//                     } else if (other->type == EntityType::BALL) {
//                         resolveCollisionClothBall(entity, other);
//                     }
//                 }
//             }
//         }
//     }
    
//     void resolveCollisionBallPlane(std::shared_ptr<Entity> ball, std::shared_ptr<Entity> plane) {
//         float ballRadius = ball->scale.x; // Assuming uniform scale
        
//         // Plane properties
//         glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Assuming plane is always facing up
//         float planeY = plane->position.y;
        
//         // Check distance to plane
//         float distance = ball->position.y - planeY - ballRadius;
        
//         // If penetrating, resolve collision
//         if (distance < 0) {
//             // Move ball out of plane
//             ball->position.y = planeY + ballRadius;
            
//             // Reflect velocity with damping (restitution)
//             float restitution = ball->physics.restitution;
//             ball->physics.velocity.y = -ball->physics.velocity.y * restitution;
            
//             // Apply friction to horizontal velocity
//             float friction = ball->physics.friction;
//             ball->physics.velocity.x *= (1.0f - friction);
//             ball->physics.velocity.z *= (1.0f - friction);
//         }
//     }
    
//     void resolveCollisionBallBall(std::shared_ptr<Entity> ball1, std::shared_ptr<Entity> ball2) {
//         float radius1 = ball1->scale.x; // Assuming uniform scale
//         float radius2 = ball2->scale.x;
        
//         glm::vec3 delta = ball2->position - ball1->position;
//         float distance = glm::length(delta);
//         float minDist = radius1 + radius2;
        
//         // Check for collision
//         if (distance < minDist) {
//             // Calculate penetration depth
//             float penetration = minDist - distance;
            
//             // Normalize delta
//             glm::vec3 normal = distance > 0.0001f ? delta / distance : glm::vec3(1, 0, 0);
            
//             // Calculate relative velocity
//             glm::vec3 relVel = ball2->physics.velocity - ball1->physics.velocity;
            
//             // Calculate impulse
//             float restitution = std::min(ball1->physics.restitution, ball2->physics.restitution);
//             float impulseMag = -(1.0f + restitution) * glm::dot(relVel, normal);
//             impulseMag /= 1.0f/ball1->physics.mass + 1.0f/ball2->physics.mass;
            
//             // Apply impulse
//             if (!ball1->physics.isStatic) {
//                 ball1->physics.velocity -= impulseMag * normal / ball1->physics.mass;
//             }
//             if (!ball2->physics.isStatic) {
//                 ball2->physics.velocity += impulseMag * normal / ball2->physics.mass;
//             }
            
//             // Separate the balls
//             if (!ball1->physics.isStatic && !ball2->physics.isStatic) {
//                 ball1->position -= normal * penetration * 0.5f;
//                 ball2->position += normal * penetration * 0.5f;
//             } else if (!ball1->physics.isStatic) {
//                 ball1->position -= normal * penetration;
//             } else if (!ball2->physics.isStatic) {
//                 ball2->position += normal * penetration;
//             }
//         }
//     }
    
//     void resolveCollisionClothPlane(std::shared_ptr<Entity> cloth, std::shared_ptr<Entity> plane) {
//         float planeY = plane->position.y;
        
//         // For each cloth vertex, check for collision with plane
//         for (int i = 0; i < cloth->clothVertices.size(); i++) {
//             glm::vec3 particlePos = cloth->clothVertices[i] + cloth->position;
            
//             // If particle is below plane
//             if (particlePos.y < planeY) {
//                 // Move particle to plane surface
//                 cloth->clothVertices[i].y = planeY - cloth->position.y;
                
//                 // Reflect velocity with damping
//                 cloth->clothVelocities[i].y = -cloth->clothVelocities[i].y * cloth->physics.restitution;
                
//                 // Apply friction to horizontal velocity
//                 float friction = cloth->physics.friction;
//                 cloth->clothVelocities[i].x *= (1.0f - friction);
//                 cloth->clothVelocities[i].z *= (1.0f - friction);
//             }
//         }
//     }
    
//     void resolveCollisionClothBall(std::shared_ptr<Entity> cloth, std::shared_ptr<Entity> ball) {
//         float ballRadius = ball->scale.x; // Assuming uniform scale
        
//         // For each cloth vertex, check for collision with ball
//         for (int i = 0; i < cloth->clothVertices.size(); i++) {
//             glm::vec3 particlePos = cloth->clothVertices[i] + cloth->position;
//             glm::vec3 delta = particlePos - ball->position;
//             float distance = glm::length(delta);
            
//             // If particle is inside ball
//             if (distance < ballRadius) {
//                 // Normalize direction
//                 glm::vec3 normal = distance > 0.0001f ? delta / distance : glm::vec3(0, 1, 0);
                
//                 // Move particle to ball surface
//                 cloth->clothVertices[i] = ball->position + normal * ballRadius - cloth->position;
                
//                 // Reflect velocity with damping
//                 float restitution = std::min(cloth->physics.restitution, ball->physics.restitution);
                
//                 // Calculate velocity component along normal
//                 float velAlongNormal = glm::dot(cloth->clothVelocities[i], normal);
                
//                 // Only reflect if moving towards each other
//                 if (velAlongNormal < 0) {
//                     cloth->clothVelocities[i] -= (1.0f + restitution) * velAlongNormal * normal;
//                 }
                
//                 // Also affect ball slightly (much less if ball is heavier)
//                 if (!ball->physics.isStatic) {
//                     float massRatio = cloth->physics.mass / (ball->physics.mass * cloth->clothVertices.size());
//                     ball->physics.velocity -= normal * velAlongNormal * massRatio;
//                 }
//             }
//         }
//     }
// };

// // --- Renderer Class ---
// class Renderer {
// private:
//     // Mesh data for primitive shapes
//     std::vector<float> sphereVertices;
//     std::vector<unsigned int> sphereIndices;
//     std::vector<float> planeVertices;
//     std::vector<unsigned int> planeIndices;
    
//     // OpenGL shader program
//     GLuint shaderProgram;
    
//     // Uniform locations
//     GLint modelLoc, viewLoc, projLoc;
//     GLint objectColorLoc, lightPosLoc, lightColorLoc, viewPosLoc;
    
//     void createPrimitiveMeshes() {
//         // Create Sphere mesh data
//         createSphere(1.0f, 16, 16);
        
//         // Create Plane mesh data
//         createPlane(1.0f);
//     }
    
//     void createSphere(float radius, int sectors, int stacks) {
//         sphereVertices.clear();
//         sphereIndices.clear();
        
//         float sectorStep = 2.0f * M_PI / sectors;
//         float stackStep = M_PI / stacks;
        
//         for (int i = 0; i <= stacks; ++i) {
//             float stackAngle = M_PI / 2.0f - i * stackStep;
//             float xy = radius * cos(stackAngle);
//             float z = radius * sin(stackAngle);
            
//             for (int j = 0; j <= sectors; ++j) {
//                 float sectorAngle = j * sectorStep;
                
//                 // Vertex position
//                 float x = xy * cos(sectorAngle);
//                 float y = xy * sin(sectorAngle);
                
//                 // Normal vector
//                 float nx = x / radius;
//                 float ny = y / radius;
//                 float nz = z / radius;
                
//                 // Add vertex data
//                 sphereVertices.push_back(x);
//                 sphereVertices.push_back(y);
//                 sphereVertices.push_back(z);
//                 sphereVertices.push_back(nx);
//                 sphereVertices.push_back(ny);
//                 sphereVertices.push_back(nz);
//             }
//         }
        
//         // Generate indices
//         for (int i = 0; i < stacks; ++i) {
//             int k1 = i * (sectors + 1);
//             int k2 = k1 + sectors + 1;
            
//             for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
//                 if (i != 0) {
//                     sphereIndices.push_back(k1);
//                     sphereIndices.push_back(k2);
//                     sphereIndices.push_back(k1 + 1);
//                 }
                
//                 if (i != (stacks - 1)) {
//                     sphereIndices.push_back(k1 + 1);
//                     sphereIndices.push_back(k2);
//                     sphereIndices.push_back(k2 + 1);
//                 }
//             }
//         }
//     }
    
//     void createPlane(float size) {
//         planeVertices = {
//             // Positions           // Normals
//             -size, 0.0f, -size,    0.0f, 1.0f, 0.0f,
//              size, 0.0f, -size,    0.0f, 1.0f, 0.0f,
//              size, 0.0f,  size,    0.0f, 1.0f, 0.0f,
//             -size, 0.0f,  size,    0.0f, 1.0f, 0.0f
//         };
        
//         planeIndices = {
//             0, 1, 2,
//             0, 2, 3
//         };
//     }
    
//     GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
//         // Vertex shader
//         GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
//         glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
//         glCompileShader(vertexShader);
        
//         // Check for shader compile errors
//         int success;
//         char infoLog[512];
//         glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
//         if (!success) {
//             glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
//             std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
//         }
        
//         // Fragment shader
//         GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
//         glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
//         glCompileShader(fragmentShader);
        
//         // Check for shader compile errors
//         glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
//         if (!success) {
//             glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
//             std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
//         }
        
//         // Link shaders
//         GLuint program = glCreateProgram();
//         glAttachShader(program, vertexShader);
//         glAttachShader(program, fragmentShader);
//         glLinkProgram(program);
        
//         // Check for linking errors
//         glGetProgramiv(program, GL_LINK_STATUS, &success);
//         if (!success) {
//             glGetProgramInfoLog(program, 512, NULL, infoLog);
//             std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
//         }
        
//         // Delete shaders after linking
//         glDeleteShader(vertexShader);
//         glDeleteShader(fragmentShader);
        
//         return program;
//     }
    
//     void configureBuffers(std::shared_ptr<Entity> entity) {
//         if (entity->VAO != 0) {
//             // Already configured
//             return;
//         }
        
//         glGenVertexArrays(1, &entity->VAO);
//         glGenBuffers(1, &entity->VBO);
//         glGenBuffers(1, &entity->EBO);
        
//         glBindVertexArray(entity->VAO);
        
//         if (entity->type == EntityType::BALL) {
//             // Use sphere mesh data
//             glBindBuffer(GL_ARRAY_BUFFER, entity->VBO);
//             glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
            
//             glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entity->EBO);
//             glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
            
//             entity->indexCount = sphereIndices.size();
//         }
//         else if (entity->type == EntityType::PLANE) {
//             // Use plane mesh data
//             glBindBuffer(GL_ARRAY_BUFFER, entity->VBO);
//             glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);
            
//             glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entity->EBO);
//             glBufferData(GL_ELEMENT_ARRAY_BUFFER, planeIndices.size() * sizeof(unsigned int), planeIndices.data(), GL_STATIC_DRAW);
            
//             entity->indexCount = planeIndices.size();
//         }
//         else if (entity->type == EntityType::CLOTH) {
//             // Create dynamic buffers for cloth - will update each frame
//             updateClothMesh(entity);
//         }
        
//         // Set vertex attribute pointers
//         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
//         glEnableVertexAttribArray(0);
        
//         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
//         glEnableVertexAttribArray(1);
        
//         glBindVertexArray(0);
//     }
    
//     void updateClothMesh(std::shared_ptr<Entity> cloth) {
//         if (cloth->type != EntityType::CLOTH) return;
        
//         // Generate mesh data from cloth particles
//         std::vector<float> vertices;
//         std::vector<unsigned int> indices;
        
//         // Create vertices with positions and normals
//         for (int y = 0; y < cloth->clothResolution; y++) {
//             for (int x = 0; x < cloth->clothResolution; x++) {
//                 int i = y * cloth->clothResolution + x;
                
//                 // Position
//                 glm::vec3 pos = cloth->clothVertices[i];
//                 vertices.push_back(pos.x);
//                 vertices.push_back(pos.y);
//                 vertices.push_back(pos.z);
                
//                 // Calculate normal using neighboring particles (simplified)
//                 glm::vec3 normal(0.0f, 1.0f, 0.0f); // Default normal
                
//                 if (x < cloth->clothResolution - 1 && y < cloth->clothResolution - 1) {
//                     int i1 = i;
//                     int i2 = i + 1;
//                     int i3 = i + cloth->clothResolution;
                    
//                     glm::vec3 p1 = cloth->clothVertices[i1];
//                     glm::vec3 p2 = cloth->clothVertices[i2];
//                     glm::vec3 p3 = cloth->clothVertices[i3];
                    
//                     glm::vec3 v1 = p2 - p1;
//                     glm::vec3 v2 = p3 - p1;
                    
//                     normal = glm::normalize(glm::cross(v1, v2));
//                 }
                
//                 vertices.push_back(normal.x);
//                 vertices.push_back(normal.y);
//                 vertices.push_back(normal.z);
//             }
//         }
        
//         // Create indices for triangles
//         for (int y = 0; y < cloth->clothResolution - 1; y++) {
//             for (int x = 0; x < cloth->clothResolution - 1; x++) {
//                 int i = y * cloth->clothResolution + x;
                
//                 // First triangle
//                 indices.push_back(i);
//                 indices.push_back(i + 1);
//                 indices.push_back(i + cloth->clothResolution);
                
//                 // Second triangle
//                 indices.push_back(i + 1);
//                 indices.push_back(i + cloth->clothResolution + 1);
//                 indices.push_back(i + cloth->clothResolution);
//             }
//         }
        
//         // Update buffers
//         glBindVertexArray(cloth->VAO);
        
//         glBindBuffer(GL_ARRAY_BUFFER, cloth->VBO);
//         glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        
//         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloth->EBO);
//         glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
        
//         cloth->indexCount = indices.size();
        
//         glBindVertexArray(0);
//     }
    
// public:
//     Renderer() : shaderProgram(0) {}
    
//     void initialize() {
//         // Create primitive meshes
//         createPrimitiveMeshes();
        
//         // Create shader program
//         shaderProgram = createShaderProgram(g_VertexShaderSrc, g_FragmentShaderSrc);
        
//         // Get uniform locations
//         modelLoc = glGetUniformLocation(shaderProgram, "model");
//         viewLoc = glGetUniformLocation(shaderProgram, "view");
//         projLoc = glGetUniformLocation(shaderProgram, "projection");
//         objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
//         lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
//         lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
//         viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
//     }
    
//     void render(Scene& scene) {
//         // Clear the screen
//         glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
//         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
//         // Set up view and projection matrices
//         glm::mat4 view = glm::lookAt(
//             g_CameraPosition,
//             g_CameraTarget,
//             glm::vec3(0.0f, 1.0f, 0.0f)
//         );
        
//         glm::mat4 projection = glm::perspective(
//             glm::radians(45.0f),
//             (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
//             0.1f,
//             100.0f
//         );
        
//         // Use our shader
//         glUseProgram(shaderProgram);
        
//         // Set light properties
//         glm::vec3 lightPos = g_CameraPosition; // Light follows camera
//         glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        
//         glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
//         glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
//         glUniform3fv(viewPosLoc, 1, glm::value_ptr(g_CameraPosition));
        
//         // Set view and projection matrices
//         glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
//         glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
//         // Render each entity
//         for (auto& entity : scene.getEntities()) {
//             // Configure buffers if not already done
//             configureBuffers(entity);
            
//             // For cloth, update the mesh data each frame
//             if (entity->type == EntityType::CLOTH) {
//                 updateClothMesh(entity);
//             }
            
//             // Set model matrix
//             glm::mat4 model = glm::mat4(1.0f);
//             model = glm::translate(model, entity->position);
//             model = glm::rotate(model, glm::radians(entity->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
//             model = glm::rotate(model, glm::radians(entity->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
//             model = glm::rotate(model, glm::radians(entity->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
//             model = glm::scale(model, entity->scale);
            
//             glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
//             // Set color (with highlight if selected)
//             glm::vec3 color = entity->isSelected ? 
//                               glm::vec3(entity->color) * 1.3f : 
//                               glm::vec3(entity->color);
//             glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
            
//             // Draw the entity
//             glBindVertexArray(entity->VAO);
//             glDrawElements(GL_TRIANGLES, entity->indexCount, GL_UNSIGNED_INT, 0);
//             glBindVertexArray(0);
//         }
//     }
    
//     glm::vec3 screenToWorldRay(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection) {
//         // Convert screen coordinates to normalized device coordinates
//         float x = (2.0f * screenX) / WINDOW_WIDTH - 1.0f;
//         float y = 1.0f - (2.0f * screenY) / WINDOW_HEIGHT;
//         float z = 1.0f; // Forward in NDC
        
//         // Homogeneous clip space
//         glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        
//         // Eye space
//         glm::vec4 rayEye = glm::inverse(projection) * rayClip;
//         rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        
//         // World space
//         glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
        
//         return rayWorld;
//     }
    
//     void cleanup() {
//         glDeleteProgram(shaderProgram);
//     }
// };

// // Helper function to implement in Entity class
// void Entity::generateRenderingData(Renderer* renderer) {
//     // This can be used to create mesh data if needed
// }

// // Implementation of ray casting for scene selection
// bool Scene::selectEntityAtScreenPos(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection) {
//     glm::vec3 rayDir = Renderer().screenToWorldRay(screenX, screenY, view, projection);
//     glm::vec3 rayOrigin = g_CameraPosition;
    
//     float closestT = std::numeric_limits<float>::max();
//     std::shared_ptr<Entity> closestEntity = nullptr;
    
//     for (auto& entity : entities) {
//         // Simple sphere intersection test for all entities (simplified)
//         float radius = glm::length(entity->scale) * 0.5f; // Approximation
        
//         glm::vec3 oc = rayOrigin - entity->position;
//         float a = glm::dot(rayDir, rayDir);
//         float b = 2.0f * glm::dot(oc, rayDir);
//         float c = glm::dot(oc, oc) - radius * radius;
//         float discriminant = b*b - 4*a*c;
        
//         if (discriminant > 0) {
//             float t = (-b - sqrt(discriminant)) / (2.0f * a);
//             if (t > 0 && t < closestT) {
//                 closestT = t;
//                 closestEntity = entity;
//             }
//         }
//     }
    
//     if (closestEntity) {
//         setSelectedEntity(closestEntity);
//         return true;
//     }
    
//     return false;
// }

// // --- Editor GUI Class ---
// class EditorGUI {
// private:
//     Scene* scene;
//     Renderer* renderer;
//     PhysicsSystem* physics;
//     bool showMetrics = false;
    
// public:
//     EditorGUI(Scene* _scene, Renderer* _renderer, PhysicsSystem* _physics)
//         : scene(_scene), renderer(_renderer), physics(_physics) {}
    
//     void initialize() {
//         // Setup ImGui context
//         IMGUI_CHECKVERSION();
//         ImGui::CreateContext();
//         ImGuiIO& io = ImGui::GetIO();
//         io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
//         io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
//         // Setup ImGui style
//         ImGui::StyleColorsDark();
//         ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
//         ImGui_ImplOpenGL3_Init("#version 330 core");
//     }
    
//     void render() {
//         // Start ImGui frame
//         ImGui_ImplOpenGL3_NewFrame();
//         ImGui_ImplGlfw_NewFrame();
//         ImGui::NewFrame();
        
//         // Set up docking
//         setupDockSpace();
        
//         // Render editor panels
//         renderToolbar();
//         renderSceneHierarchy();
//         renderInspector();
//         renderConsole();
        
//         // Render ImGui
//         ImGui::Render();
//         ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//     }
    
//     void setupDockSpace() {
//         static bool opt_fullscreen = true;
//         static bool opt_padding = false;
//         static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        
//         ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
//         if (opt_fullscreen) {
//             const ImGuiViewport* viewport = ImGui::GetMainViewport();
//             ImGui::SetNextWindowPos(viewport->WorkPos);
//             ImGui::SetNextWindowSize(viewport->WorkSize);
//             ImGui::SetNextWindowViewport(viewport->ID);
//             ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
//             ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
//             window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
//             window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
//         }
        
//         if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
//             window_flags |= ImGuiWindowFlags_NoBackground;
            
//         ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
//         ImGui::Begin("DockSpace", nullptr, window_flags);
//         ImGui::PopStyleVar();
        
//         if (opt_fullscreen)
//             ImGui::PopStyleVar(2);
            
//         // Submit the DockSpace
//         ImGuiIO& io = ImGui::GetIO();
//         if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
//             ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
//             ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
//         }
        
//         ImGui::End();
//     }
    
//     void renderToolbar() {
//         if (ImGui::BeginMainMenuBar()) {
//             if (ImGui::BeginMenu("File")) {
//                 if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
//                     scene->clear();
//                 }
//                 if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
//                     scene->loadFromFile("scene.json");
//                 }
//                 if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
//                     scene->saveToFile("scene.json");
//                 }
//                 ImGui::Separator();
//                 if (ImGui::MenuItem("Exit", "Alt+F4")) {
//                     glfwSetWindowShouldClose(g_Window, true);
//                 }
//                 ImGui::EndMenu();
//             }
            
//             if (ImGui::BeginMenu("Edit")) {
//                 if (ImGui::MenuItem("Delete Selected", "Del")) {
//                     if (auto entity = scene->getSelectedEntity()) {
//                         scene->removeEntity(entity->id);
//                     }
//                 }
//                 ImGui::EndMenu();
//             }
            
//             if (ImGui::BeginMenu("Create")) {
//                 if (ImGui::MenuItem("Ball", "Ctrl+1")) {
//                     scene->addEntity("Ball", EntityType::BALL);
//                 }
//                 if (ImGui::MenuItem("Cloth", "Ctrl+2")) {
//                     scene->addEntity("Cloth", EntityType::CLOTH);
//                 }
//                 if (ImGui::MenuItem("Plane", "Ctrl+3")) {
//                     scene->addEntity("Plane", EntityType::PLANE);
//                 }
//                 ImGui::EndMenu();
//             }
            
//             if (ImGui::BeginMenu("View")) {
//                 if (ImGui::MenuItem("Metrics", NULL, &showMetrics)) {}
//                 ImGui::EndMenu();
//             }
            
//             ImGui::EndMainMenuBar();
//         }
        
//         // Toolbar buttons
//         ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        
//         if (ImGui::Button(g_IsPlaying ? "Stop" : "Play", ImVec2(100, 30))) {
//             g_IsPlaying = !g_IsPlaying;
//             if (g_IsPlaying) {
//                 g_ConsoleOutput += "Started simulation.\n";
//             } else {
//                 g_ConsoleOutput += "Stopped simulation.\n";
//             }
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("Add Ball", ImVec2(100, 30))) {
//             scene->addEntity("Ball", EntityType::BALL);
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("Add Cloth", ImVec2(100, 30))) {
//             scene->addEntity("Cloth", EntityType::CLOTH);
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("Add Plane", ImVec2(100, 30))) {
//             scene->addEntity("Plane", EntityType::PLANE);
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("Save Scene", ImVec2(100, 30))) {
//             scene->saveToFile("scene.json");
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("Load Scene", ImVec2(100, 30))) {
//             scene->loadFromFile("scene.json");
//         }
        
//         ImGui::SameLine();
//         if (ImGui::Button("AI Generate", ImVec2(100, 30))) {
//             scene->generatedRandomScene();
//         }
        
//         ImGui::End();
        
//         // Display metrics window if enabled
//         if (showMetrics) {
//             ImGui::ShowMetricsWindow(&showMetrics);
//         }
//     }
    
//     void renderSceneHierarchy() {
//         ImGui::Begin("Scene Hierarchy");
        
//         for (auto& entity : scene->getEntities()) {
//             ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
//             if (entity->isSelected) {
//                 flags |= ImGuiTreeNodeFlags_Selected;
//             }
            
//             std::string label = entity->name + " (" + std::to_string(entity->id) + ")";
            
//             if (ImGui::TreeNodeEx((void*)(intptr_t)entity->id, flags, "%s", label.c_str())) {
//                 if (ImGui::IsItemClicked()) {
//                     scene->setSelectedEntity(entity);
//                 }
//             }
//         }
        
//         // Right-click context menu
//         if (ImGui::BeginPopupContextWindow()) {
//             if (ImGui::MenuItem("Add Ball")) {
//                 scene->addEntity("Ball", EntityType::BALL);
//             }
//             if (ImGui::MenuItem("Add Cloth")) {
//                 scene->addEntity("Cloth", EntityType::CLOTH);
//             }
//             if (ImGui::MenuItem("Add Plane")) {
//                 scene->addEntity("Plane", EntityType::PLANE);
//             }
//             ImGui::EndPopup();
//         }
        
//         ImGui::End();
//     }
    
//     void renderInspector() {
//         ImGui::Begin("Inspector");
        
//         auto selectedEntity = scene->getSelectedEntity();
//         if (selectedEntity) {
//             // Entity name and type
//             std::string typeName;
//             switch (selectedEntity->type) {
//                 case EntityType::BALL: typeName = "Ball"; break;
//                 case EntityType::CLOTH: typeName = "Cloth"; break;
//                 case EntityType::PLANE: typeName = "Plane"; break;
//                 default: typeName = "Unknown"; break;
//             }
            
//             char nameBuffer[256];
//             strcpy(nameBuffer, selectedEntity->name.c_str());
//             if (ImGui::InputText("Name", nameBuffer, 256)) {
//                 selectedEntity->name = nameBuffer;
//             }
            
//             ImGui::Text("Type: %s", typeName.c_str());
//             ImGui::Text("ID: %d", selectedEntity->id);
//             ImGui::Separator();
            
//             // Transform
//             ImGui::Text("Transform");
            
//             glm::vec3 position = selectedEntity->position;
//             if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
//                 selectedEntity->position = position;
//                 selectedEntity->updateTransform();
//             }
            
//             glm::vec3 rotation = selectedEntity->rotation;
//             if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 1.0f)) {
//                 selectedEntity->rotation = rotation;
//                 selectedEntity->updateTransform();
//             }
            
//             glm::vec3 scale = selectedEntity->scale;
//             if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f, 0.1f, 100.0f)) {
//                 selectedEntity->scale = scale;
//                 selectedEntity->updateTransform();
//             }
            
//             ImGui::Separator();
            
//             // Material/Color
//             ImGui::Text("Material");
//             glm::vec4 color = selectedEntity->color;
//             if (ImGui::ColorEdit4("Color", glm::value_ptr(color))) {
//                 selectedEntity->color = color;
//             }
            
//             ImGui::Separator();
            
//             // Physics properties
//             ImGui::Text("Physics");
            
//             bool isStatic = selectedEntity->physics.isStatic;
//             if (ImGui::Checkbox("Static", &isStatic)) {
//                 selectedEntity->physics.isStatic = isStatic;
//             }
            
//             float mass = selectedEntity->physics.mass;
//             if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f)) {
//                 selectedEntity->physics.mass = mass;
//             }
            
//             float restitution = selectedEntity->physics.restitution;
//             if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f)) {
//                 selectedEntity->physics.restitution = restitution;
//             }
            
//             float friction = selectedEntity->physics.friction;
//             if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
//                 selectedEntity->physics.friction = friction;
//             }
            
//             bool useGravity = selectedEntity->physics.useGravity;
//             if (ImGui::Checkbox("Use Gravity", &useGravity)) {
//                 selectedEntity->physics.useGravity = useGravity;
//             }
            
//             // Entity specific properties
//             if (selectedEntity->type == EntityType::CLOTH) {
//                 ImGui::Separator();
//                 ImGui::Text("Cloth Properties");
                
//                 int resolution = selectedEntity->clothResolution;
//                 if (ImGui::SliderInt("Resolution", &resolution, 5, 30)) {
//                     selectedEntity->clothResolution = resolution;
//                     selectedEntity->initializeCloth();
//                 }
                
//                 float size = selectedEntity->clothSize;
//                 if (ImGui::SliderFloat("Size", &size, 1.0f, 20.0f)) {
//                     selectedEntity->clothSize = size;
//                     selectedEntity->initializeCloth();
//                 }
                
//                 float stiffness = selectedEntity->clothStiffness;
//                 if (ImGui::SliderFloat("Stiffness", &stiffness, 10.0f, 500.0f)) {
//                     selectedEntity->clothStiffness = stiffness;
//                 }
                
//                 float damping = selectedEntity->clothDamping;
//                 if (ImGui::SliderFloat("Damping", &damping, 0.001f, 0.1f)) {
//                     selectedEntity->clothDamping = damping;
//                 }
//             }
//         }
//         else {
//             ImGui::Text("No entity selected");
//         }
        
//         ImGui::End();
//     }
    
//     void renderConsole() {
//         ImGui::Begin("Console");
        
//         ImGui::TextWrapped("%s", g_ConsoleOutput.c_str());
        
//         // Auto-scroll to bottom
//         if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
//             ImGui::SetScrollHereY(1.0f);
        
//         ImGui::End();
//     }
    
//     void cleanup() {
//         // Cleanup ImGui
//         ImGui_ImplOpenGL3_Shutdown();
//         ImGui_ImplGlfw_Shutdown();
//         ImGui::DestroyContext();
//     }
// };

// // --- Input Handling ---
// void handleInput(GLFWwindow* window, Scene& scene, float deltaTime) {
//     // Camera controls
//     if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
//         g_CameraPosition += glm::normalize(g_CameraTarget - g_CameraPosition) * 5.0f * deltaTime;
//     }
//     if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
//         g_CameraPosition -= glm::normalize(g_CameraTarget - g_CameraPosition) * 5.0f * deltaTime;
//     }
//     if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
//         glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
//         g_CameraPosition -= right * 5.0f * deltaTime;
//         g_CameraTarget -= right * 5.0f * deltaTime;
//     }
//     if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
//         glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
//         g_CameraPosition += right * 5.0f * deltaTime;
//         g_CameraTarget += right * 5.0f * deltaTime;
//     }
//     if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
//         g_CameraPosition.y += 5.0f * deltaTime;
//         g_CameraTarget.y += 5.0f * deltaTime;
//     }
//     if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
//         g_CameraPosition.y -= 5.0f * deltaTime;
//         g_CameraTarget.y -= 5.0f * deltaTime;
//     }
    
//     // Shortcut keys
//     static bool ctrlDown = false;
//     if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
//         glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
//         ctrlDown = true;
//     } else {
//         ctrlDown = false;
//     }
    
//     // Ctrl+S: Save
//     static bool savePressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
//         if (!savePressed) {
//             scene.saveToFile("scene.json");
//             savePressed = true;
//         }
//     } else {
//         savePressed = false;
//     }
    
//     // Ctrl+O: Open
//     static bool openPressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
//         if (!openPressed) {
//             scene.loadFromFile("scene.json");
//             openPressed = true;
//         }
//     } else {
//         openPressed = false;
//     }
    
//     // Ctrl+N: New Scene
//     static bool newPressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
//         if (!newPressed) {
//             scene.clear();
//             newPressed = true;
//         }
//     } else {
//         newPressed = false;
//     }
    
//     // Delete: Remove selected entity
//     static bool deletePressed = false;
//     if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
//         if (!deletePressed) {
//             if (auto selectedEntity = scene.getSelectedEntity()) {
//                 scene.removeEntity(selectedEntity->id);
//             }
//             deletePressed = true;
//         }
//     } else {
//         deletePressed = false;
//     }
    
//     // 1,2,3: Add entities
//     static bool key1Pressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
//         if (!key1Pressed) {
//             scene.addEntity("Ball", EntityType::BALL);
//             key1Pressed = true;
//         }
//     } else {
//         key1Pressed = false;
//     }
    
//     static bool key2Pressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
//         if (!key2Pressed) {
//             scene.addEntity("Cloth", EntityType::CLOTH);
//             key2Pressed = true;
//         }
//     } else {
//         key2Pressed = false;
//     }
    
//     static bool key3Pressed = false;
//     if (ctrlDown && glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
//         if (!key3Pressed) {
//             scene.addEntity("Plane", EntityType::PLANE);
//             key3Pressed = true;
//         }
//     } else {
//         key3Pressed = false;
//     }
    
//     // Space: Play/Pause
//     static bool spacePressed = false;
//     if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
//         if (!spacePressed) {
//             g_IsPlaying = !g_IsPlaying;
//             spacePressed = true;
//         }
//     } else {
//         spacePressed = false;
//     }
// }

// // --- Mouse Callbacks ---
// void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
//     ImGuiIO& io = ImGui::GetIO();
//     if (io.WantCaptureMouse) return; // Don't handle mouse if ImGui is using it
    
//     if (button == GLFW_MOUSE_BUTTON_LEFT) {
//         if (action == GLFW_PRESS) {
//             g_IsMouseDown = true;
//             g_IsDragging = false;
//         } else if (action == GLFW_RELEASE) {
//             g_IsMouseDown = false;
//             g_IsDragging = false;
//         }
//     }
// }

// void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
//     ImGuiIO& io = ImGui::GetIO();
//     if (io.WantCaptureMouse) return; // Don't handle mouse if ImGui is using it
    
//     static double lastX = xpos;
//     static double lastY = ypos;
    
//     g_MouseDeltaX = xpos - lastX;
//     g_MouseDeltaY = ypos - lastY;
    
//     // Store current position for next frame
//     lastX = xpos;
//     lastY = ypos;
    
//     // Update global mouse position
//     g_MouseX = xpos;
//     g_MouseY = ypos;
// }

// void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
//     ImGuiIO& io = ImGui::GetIO();
//     if (io.WantCaptureMouse) return; // Don't handle scroll if ImGui is using it
    
//     // Zoom camera in/out
//     float zoomSpeed = 0.1f;
//     g_CameraZoom += yoffset * zoomSpeed;
//     g_CameraZoom = glm::clamp(g_CameraZoom, 0.1f, 10.0f);
    
//     // Update camera position based on zoom
//     glm::vec3 direction = glm::normalize(g_CameraTarget - g_CameraPosition);
//     float distance = glm::length(g_CameraTarget - g_CameraPosition);
//     float newDistance = distance * (1.0f - yoffset * zoomSpeed);
    
//     g_CameraPosition = g_CameraTarget - direction * newDistance;
// }

// // --- GLFW Error Callback ---
// void errorCallback(int error, const char* description) {
//     std::cerr << "GLFW Error " << error << ": " << description << std::endl;
// }

// // --- OpenGL Debug Callback ---
// void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
//                            GLsizei length, const GLchar *message, const void *userParam) {
//     // Ignore non-significant error/warning codes
//     if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    
//     std::cout << "---------------" << std::endl;
//     std::cout << "Debug message (" << id << "): " << message << std::endl;
    
//     switch (source) {
//         case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
//         case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
//         case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
//         case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
//         case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
//         case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
//     }
//     std::cout << std::endl;
    
//     switch (type) {
//         case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
//         case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
//         case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
//         case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
//         case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
//         case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
//         case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
//         case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
//         case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
//     }
//     std::cout << std::endl;
    
//     switch (severity) {
//         case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
//         case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
//         case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
//         case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
//     }
//     std::cout << std::endl;
//     std::cout << std::endl;
// }

// // --- Main Function ---
// int main() {
//     // Initialize GLFW
//     if (!glfwInit()) {
//         std::cerr << "Failed to initialize GLFW" << std::endl;
//         return -1;
//     }
    
//     // Set GLFW error callback
//     glfwSetErrorCallback(errorCallback);
    
//     // Set OpenGL version hints
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
//     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//     glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
// #ifdef __APPLE__
//     glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
// #endif
    
//     // Enable debug output
//     glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    
//     // Create window
//     g_Window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
//     if (!g_Window) {
//         std::cerr << "Failed to create GLFW window" << std::endl;
//         glfwTerminate();
//         return -1;
//     }
    
//     // Make OpenGL context current
//     glfwMakeContextCurrent(g_Window);
    
//     // Set vsync
//     glfwSwapInterval(1);
    
//     // Initialize GLEW
//     glewExperimental = GL_TRUE;
//     if (glewInit() != GLEW_OK) {
//         std::cerr << "Failed to initialize GLEW" << std::endl;
//         glfwTerminate();
//         return -1;
//     }
    
//     // Set up OpenGL debug callback
//     if (GLEW_ARB_debug_output) {
//         glEnable(GL_DEBUG_OUTPUT);
//         glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
//         glDebugMessageCallback(glDebugOutput, nullptr);
//         glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
//     }
    
//     // Set up viewport
//     int width, height;
//     glfwGetFramebufferSize(g_Window, &width, &height);
//     glViewport(0, 0, width, height);
    
//     // Enable depth testing
//     glEnable(GL_DEPTH_TEST);
    
//     // Register callbacks
//     glfwSetMouseButtonCallback(g_Window, mouseButtonCallback);
//     glfwSetCursorPosCallback(g_Window, cursorPosCallback);
//     glfwSetScrollCallback(g_Window, scrollCallback);
    
//     // Create systems
//     Scene scene;
//     Renderer renderer;
//     PhysicsSystem physics;
//     EditorGUI editor(&scene, &renderer, &physics);
    
//     // Initialize systems
//     renderer.initialize();
//     editor.initialize();
    
//     // Add a default ground plane
//     auto plane = scene.addEntity("Ground", EntityType::PLANE);
//     plane->position = glm::vec3(0.0f, -0.05f, 0.0f);
//     plane->scale = glm::vec3(15.0f, 0.1f, 15.0f);
//     plane->physics.isStatic = true;
    
//     // Main loop
//     double lastTime = glfwGetTime();
//     while (!glfwWindowShouldClose(g_Window)) {
//         // Calculate delta time
//         double currentTime = glfwGetTime();
//         g_DeltaTime = float(currentTime - lastTime);
//         lastTime = currentTime;
        
//         // Poll events
//         glfwPollEvents();
        
//         // Handle input
//         handleInput(g_Window, scene, g_DeltaTime);
        
//         // Update physics
//         physics.update(scene, g_DeltaTime);
        
//         // Handle mouse picking
//         static bool wasMouseDown = false;
//         if (g_IsMouseDown && !wasMouseDown) {
//             // Mouse just pressed
//             ImGuiIO& io = ImGui::GetIO();
//             if (!io.WantCaptureMouse) {
//                 // Get view and projection matrices for picking
//                 glm::mat4 view = glm::lookAt(
//                     g_CameraPosition,
//                     g_CameraTarget,
//                     glm::vec3(0.0f, 1.0f, 0.0f)
//                 );
                
//                 glm::mat4 projection = glm::perspective(
//                     glm::radians(45.0f),
//                     (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
//                     0.1f,
//                     100.0f
//                 );
                
//                 // Try to select entity
//                 if (!scene.selectEntityAtScreenPos(g_MouseX, g_MouseY, view, projection)) {
//                     // If no entity was selected, deselect current
//                     scene.setSelectedEntity(nullptr);
//                 }
//             }
//         }
        
//         // Handle dragging
//         if (g_IsMouseDown && !g_IsDragging && (abs(g_MouseDeltaX) > 1.0 || abs(g_MouseDeltaY) > 1.0)) {
//             g_IsDragging = true;
//         }
        
//         if (g_IsDragging) {
//             ImGuiIO& io = ImGui::GetIO();
//             if (!io.WantCaptureMouse) {
//                 auto selectedEntity = scene.getSelectedEntity();
//                 if (selectedEntity && !g_IsPlaying && !selectedEntity->physics.isStatic) {
//                     // Calculate move in world space
//                     glm::mat4 view = glm::lookAt(
//                         g_CameraPosition,
//                         g_CameraTarget,
//                         glm::vec3(0.0f, 1.0f, 0.0f)
//                     );
                    
//                     glm::mat4 projection = glm::perspective(
//                         glm::radians(45.0f),
//                         (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
//                         0.1f,
//                         100.0f
//                     );
                    
//                     // Get screen point depths
//                     glm::vec3 rayDir = renderer.screenToWorldRay(g_MouseX, g_MouseY, view, projection);
//                     float distance = glm::length(selectedEntity->position - g_CameraPosition) / glm::dot(rayDir, glm::normalize(g_CameraTarget - g_CameraPosition));
                    
//                     // Calculate world movement
//                     glm::vec3 screenPoint = g_CameraPosition + rayDir * distance;
                    
//                     // Move entity
//                     selectedEntity->position = screenPoint;
//                     selectedEntity->updateTransform();
//                 }
//                 else if (!selectedEntity) {
//                     // Rotate camera around target
//                     float rotationSpeed = 0.01f;
//                     float horizontalAngle = g_MouseDeltaX * rotationSpeed;
//                     float verticalAngle = g_MouseDeltaY * rotationSpeed;
                    
//                     // Calculate distance from camera to target
//                     float distance = glm::length(g_CameraPosition - g_CameraTarget);
                    
//                     // Create rotation quaternions
//                     glm::quat horizontalRot = glm::angleAxis(horizontalAngle, glm::vec3(0, 1, 0));
                    
//                     // Get right vector for vertical rotation
//                     glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
//                     glm::quat verticalRot = glm::angleAxis(verticalAngle, right);
                    
//                     // Rotate camera position around target
//                     glm::vec3 dir = glm::normalize(g_CameraPosition - g_CameraTarget);
//                     dir = glm::rotate(horizontalRot * verticalRot, dir);
                    
//                     // Set new camera position
//                     g_CameraPosition = g_CameraTarget + dir * distance;
//                 }
//             }
//         }
        
//         wasMouseDown = g_IsMouseDown;
        
//         // Render scene
//         renderer.render(scene);
        
//         // Render GUI
//         editor.render();
        
//         // Swap buffers
//         glfwSwapBuffers(g_Window);
//     }
    
//     // Cleanup
//     editor.cleanup();
//     renderer.cleanup();
    
//     // Terminate GLFW
//     glfwDestroyWindow(g_Window);
//     glfwTerminate();
    
//     return 0;
// }




/**
 * ONE-FILE C++ GAME ENGINE + FAKE GODOT EDITOR (PROFESSIONAL LEVEL)
 * 
 * A complete game engine and editor in a single C++ file
 * Features:
 * - Godot-like Editor with full GUI layout
 * - Entity system with balls, cloths, planes
 * - Physics simulation
 * - Scene hierarchy and inspector panels
 * - Save/Load system (JSON)
 * - Play mode for testing
 * - AI scene generation placeholder
 */

// --- Includes ---
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <random>
#include <algorithm>
#include <fstream>
#include <unordered_map>

// OpenGL / Window handling
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "extern/imgui/imgui.h"
#include "extern/imgui/backends/imgui_impl_glfw.h"
#include "extern/imgui/backends/imgui_impl_opengl3.h"
#include "extern/json/json.hpp"
using json = nlohmann::json;

// GLM Math Library
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
// --- Forward Declarations ---
class Entity;
class Scene;
class PhysicsSystem;
class EditorGUI;
class Renderer;

// --- Global Variables ---
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char* WINDOW_TITLE = "Mini-Godot Editor";

GLFWwindow* g_Window = nullptr;
bool g_IsPlaying = false;
bool g_IsDragging = false;
bool g_IsMouseDown = false;
double g_MouseX = 0, g_MouseY = 0;
double g_MouseDeltaX = 0, g_MouseDeltaY = 0;
glm::vec3 g_CameraPosition = glm::vec3(0.0f, 5.0f, 10.0f);
glm::vec3 g_CameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
float g_CameraZoom = 1.0f;
float g_DeltaTime = 0.016f; // Default to 60fps
std::string g_ConsoleOutput = "Mini-Godot Editor initialized.\nReady to create.\n";

// --- Basic Shader Programs ---
GLuint g_BasicShader = 0;
const char* g_VertexShaderSrc = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    
    out vec3 FragPos;
    out vec3 Normal;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform vec3 objectColor;
    
    out vec3 Color;
    
    void main() {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;
        Color = objectColor;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* g_FragmentShaderSrc = R"(
    #version 330 core
    in vec3 FragPos;
    in vec3 Normal;
    in vec3 Color;
    
    out vec4 FragColor;
    
    uniform vec3 lightPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform vec3 viewPos;
    
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
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;
        
        vec3 result = (ambient + diffuse + specular) * Color;
        FragColor = vec4(result, 1.0);
    }
)";

// --- Entity Structs and Classes ---

// Entity Type Enum
enum class EntityType {
    BALL,
    CLOTH,
    PLANE
};

// Physics Properties struct
struct PhysicsProperties {
    bool isStatic = false;
    float mass = 1.0f;
    float restitution = 0.7f; // Bounciness
    float friction = 0.3f;
    bool useGravity = true;
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f);
};

// Base Entity Class
class Entity {
public:
    int id;
    std::string name;
    EntityType type;
    glm::vec3 position;
    glm::vec3 rotation; // Euler angles in degrees
    glm::vec3 scale;
    glm::vec4 color;
    PhysicsProperties physics;
    bool isSelected = false;
    
    // Specific data for different entity types
    // For cloth
    int clothResolution = 10; // Number of particles per side
    float clothSize = 5.0f;
    std::vector<glm::vec3> clothVertices;
    std::vector<glm::vec3> clothVelocities;
    std::vector<glm::vec3> clothForces;
    std::vector<std::pair<int, int>> clothSprings;
    float clothStiffness = 100.0f;
    float clothDamping = 0.01f;
    
    // For rendering
    GLuint VAO = 0, VBO = 0, EBO = 0;
    int vertexCount = 0;
    int indexCount = 0;
    
    Entity(int _id, const std::string& _name, EntityType _type) 
        : id(_id), name(_name), type(_type), 
          position(0.0f), rotation(0.0f), scale(1.0f),
          color(1.0f, 1.0f, 1.0f, 1.0f) {
        // Initialize based on type
        switch (type) {
            case EntityType::BALL:
                scale = glm::vec3(1.0f);
                color = glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);
                physics.mass = 1.0f;
                physics.useGravity = true;
                break;
                
            case EntityType::CLOTH:
                scale = glm::vec3(5.0f, 0.1f, 5.0f);
                color = glm::vec4(0.2f, 0.6f, 0.8f, 1.0f);
                physics.mass = 0.1f;
                physics.useGravity = true;
                initializeCloth();
                break;
                
            case EntityType::PLANE:
                scale = glm::vec3(10.0f, 0.1f, 10.0f);
                color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
                physics.isStatic = true;
                physics.useGravity = false;
                break;
        }
    }
    
    ~Entity() {
        cleanupGL();
    }
    
    void cleanupGL() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
        VAO = VBO = EBO = 0;
    }
    
    void initializeCloth() {
        // Create cloth mesh
        clothVertices.clear();
        clothVelocities.clear();
        clothForces.clear();
        clothSprings.clear();
        
        float step = clothSize / (clothResolution - 1);
        
        // Create vertices
        for (int y = 0; y < clothResolution; y++) {
            for (int x = 0; x < clothResolution; x++) {
                float xPos = x * step - clothSize / 2.0f;
                float yPos = 0.0f;
                float zPos = y * step - clothSize / 2.0f;
                
                clothVertices.push_back(glm::vec3(xPos, yPos, zPos));
                clothVelocities.push_back(glm::vec3(0.0f));
                clothForces.push_back(glm::vec3(0.0f));
            }
        }
        
        // Create springs
        for (int y = 0; y < clothResolution; y++) {
            for (int x = 0; x < clothResolution; x++) {
                int i = y * clothResolution + x;
                
                // Structural springs (horizontal and vertical)
                if (x < clothResolution - 1)
                    clothSprings.push_back(std::make_pair(i, i + 1));
                if (y < clothResolution - 1)
                    clothSprings.push_back(std::make_pair(i, i + clothResolution));
                
                // Shear springs (diagonal)
                if (x < clothResolution - 1 && y < clothResolution - 1) {
                    clothSprings.push_back(std::make_pair(i, i + clothResolution + 1));
                    clothSprings.push_back(std::make_pair(i + 1, i + clothResolution));
                }
            }
        }
        
        // Pin the top corners
        if (clothVertices.size() >= clothResolution) {
            int topLeft = 0;
            int topRight = clothResolution - 1;
            clothVelocities[topLeft] = glm::vec3(0.0f);
            clothVelocities[topRight] = glm::vec3(0.0f);
        }
    }
    
    void updateTransform() {
        // Update any internal data that depends on transform
        if (type == EntityType::CLOTH) {
            // We don't update cloth vertices here because that's done in the physics system
        }
    }
    
    void generateRenderingData(Renderer* renderer);
    
    json serialize() const {
        json j;
        j["id"] = id;
        j["name"] = name;
        j["type"] = static_cast<int>(type);
        
        // Transform
        j["position"] = {position.x, position.y, position.z};
        j["rotation"] = {rotation.x, rotation.y, rotation.z};
        j["scale"] = {scale.x, scale.y, scale.z};
        j["color"] = {color.r, color.g, color.b, color.a};
        
        // Physics
        j["physics"]["isStatic"] = physics.isStatic;
        j["physics"]["mass"] = physics.mass;
        j["physics"]["restitution"] = physics.restitution;
        j["physics"]["friction"] = physics.friction;
        j["physics"]["useGravity"] = physics.useGravity;
        
        // Type-specific data
        if (type == EntityType::CLOTH) {
            j["clothResolution"] = clothResolution;
            j["clothSize"] = clothSize;
            j["clothStiffness"] = clothStiffness;
            j["clothDamping"] = clothDamping;
        }
        
        return j;
    }
    
    static std::shared_ptr<Entity> deserialize(const json& j) {
        int id = j["id"];
        std::string name = j["name"];
        EntityType type = static_cast<EntityType>(j["type"].get<int>());
        
        auto entity = std::make_shared<Entity>(id, name, type);
        
        // Transform
        auto pos = j["position"];
        entity->position = glm::vec3(pos[0], pos[1], pos[2]);
        
        auto rot = j["rotation"];
        entity->rotation = glm::vec3(rot[0], rot[1], rot[2]);
        
        auto scl = j["scale"];
        entity->scale = glm::vec3(scl[0], scl[1], scl[2]);
        
        auto col = j["color"];
        entity->color = glm::vec4(col[0], col[1], col[2], col[3]);
        
        // Physics
        entity->physics.isStatic = j["physics"]["isStatic"];
        entity->physics.mass = j["physics"]["mass"];
        entity->physics.restitution = j["physics"]["restitution"];
        entity->physics.friction = j["physics"]["friction"];
        entity->physics.useGravity = j["physics"]["useGravity"];
        
        // Type-specific data
        if (type == EntityType::CLOTH) {
            entity->clothResolution = j["clothResolution"];
            entity->clothSize = j["clothSize"];
            entity->clothStiffness = j["clothStiffness"];
            entity->clothDamping = j["clothDamping"];
            entity->initializeCloth();
        }
        
        return entity;
    }
};

// --- Scene Class ---
class Scene {
private:
    std::vector<std::shared_ptr<Entity>> entities;
    int nextEntityId = 1;
    std::shared_ptr<Entity> selectedEntity = nullptr;
    
public:
    Scene() {}
    
    std::shared_ptr<Entity> addEntity(const std::string& name, EntityType type) {
        auto entity = std::make_shared<Entity>(nextEntityId++, name, type);
        entities.push_back(entity);
        g_ConsoleOutput += "Added new " + name + " to scene.\n";
        return entity;
    }
    
    void removeEntity(int id) {
        auto it = std::find_if(entities.begin(), entities.end(), 
                             [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
        if (it != entities.end()) {
            if (selectedEntity && selectedEntity->id == id) {
                selectedEntity = nullptr;
            }
            g_ConsoleOutput += "Removed " + (*it)->name + " from scene.\n";
            entities.erase(it);
        }
    }
    
    std::shared_ptr<Entity> getEntity(int id) {
        auto it = std::find_if(entities.begin(), entities.end(), 
                             [id](const std::shared_ptr<Entity>& e) { return e->id == id; });
        if (it != entities.end()) {
            return *it;
        }
        return nullptr;
    }
    
    std::vector<std::shared_ptr<Entity>>& getEntities() {
        return entities;
    }
    
    void clear() {
        entities.clear();
        selectedEntity = nullptr;
        nextEntityId = 1;
        g_ConsoleOutput += "Scene cleared.\n";
    }
    
    std::shared_ptr<Entity> getSelectedEntity() const {
        return selectedEntity;
    }
    
    void setSelectedEntity(std::shared_ptr<Entity> entity) {
        // Deselect previous entity
        if (selectedEntity) {
            selectedEntity->isSelected = false;
        }
        
        selectedEntity = entity;
        
        // Select new entity
        if (selectedEntity) {
            selectedEntity->isSelected = true;
            g_ConsoleOutput += "Selected: " + selectedEntity->name + "\n";
        }
    }
    
    bool selectEntityAtScreenPos(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection);
    
    json serialize() const {
        json j;
        j["nextEntityId"] = nextEntityId;
        
        json entitiesJson = json::array();
        for (const auto& entity : entities) {
            entitiesJson.push_back(entity->serialize());
        }
        j["entities"] = entitiesJson;
        
        return j;
    }
    
    void deserialize(const json& j) {
        clear();
        
        nextEntityId = j["nextEntityId"];
        
        for (const auto& entityJson : j["entities"]) {
            auto entity = Entity::deserialize(entityJson);
            entities.push_back(entity);
        }
        
        g_ConsoleOutput += "Scene loaded with " + std::to_string(entities.size()) + " entities.\n";
    }
    
    void saveToFile(const std::string& filename) {
        json j = serialize();
        std::ofstream file(filename);
        if (file.is_open()) {
            file << j.dump(4); // Pretty print with 4-space indent
            file.close();
            g_ConsoleOutput += "Scene saved to " + filename + "\n";
        } else {
            g_ConsoleOutput += "ERROR: Failed to save scene to " + filename + "\n";
        }
    }
    
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (file.is_open()) {
            try {
                json j;
                file >> j;
                deserialize(j);
                g_ConsoleOutput += "Scene loaded from " + filename + "\n";
                return true;
            } catch (const std::exception& e) {
                g_ConsoleOutput += "ERROR: Failed to parse scene file: " + std::string(e.what()) + "\n";
                return false;
            }
        } else {
            g_ConsoleOutput += "ERROR: Failed to open scene file: " + filename + "\n";
            return false;
        }
    }
    
    void generatedRandomScene() {
        clear();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> posDist(-5.0, 5.0);
        std::uniform_real_distribution<> heightDist(2.0, 8.0);
        std::uniform_real_distribution<> colorDist(0.1, 0.9);
        std::uniform_real_distribution<> scaleDist(0.5, 2.0);
        
        // Add ground plane
        auto plane = addEntity("Ground", EntityType::PLANE);
        plane->position = glm::vec3(0.0f, -0.05f, 0.0f);
        plane->scale = glm::vec3(15.0f, 0.1f, 15.0f);
        plane->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        
        // Add some balls
        int numBalls = 5 + (gen() % 5); // 5-9 balls
        for (int i = 0; i < numBalls; i++) {
            auto ball = addEntity("Ball_" + std::to_string(i+1), EntityType::BALL);
            ball->position = glm::vec3(posDist(gen), heightDist(gen), posDist(gen));
            ball->scale = glm::vec3(scaleDist(gen));
            ball->color = glm::vec4(colorDist(gen), colorDist(gen), colorDist(gen), 1.0f);
        }
        
        // Add a cloth
        auto cloth = addEntity("Cloth", EntityType::CLOTH);
        cloth->position = glm::vec3(0.0f, 5.0f, 0.0f);
        cloth->scale = glm::vec3(5.0f, 0.1f, 5.0f);
        cloth->color = glm::vec4(0.2f, 0.6f, 0.8f, 1.0f);
        cloth->clothResolution = 15; // Higher resolution for nicer cloth
        cloth->initializeCloth();
        
        g_ConsoleOutput += "AI Agent: Generated random scene with " + 
                          std::to_string(numBalls) + " balls and 1 cloth.\n";
    }
};

// --- Physics System ---
class PhysicsSystem {
private:
    const float GRAVITY = -9.81f;
    const int MAX_SUBSTEPS = 5;
    const float FIXED_TIMESTEP = 0.016f; // 60Hz physics
    
public:
    PhysicsSystem() {}
    
    void update(Scene& scene, float deltaTime) {
        if (!g_IsPlaying) return; // Only simulate in play mode
        
        // Apply substeps for more stable physics
        float remainingTime = deltaTime;
        int numSubsteps = std::min(MAX_SUBSTEPS, static_cast<int>(std::ceil(deltaTime / FIXED_TIMESTEP)));
        float subDelta = deltaTime / numSubsteps;
        
        for (int step = 0; step < numSubsteps; step++) {
            updateEntities(scene, subDelta);
            resolveCollisions(scene);
        }
    }
    
    void updateEntities(Scene& scene, float deltaTime) {
        for (auto& entity : scene.getEntities()) {
            if (entity->physics.isStatic) continue;
            
            // Apply gravity
            if (entity->physics.useGravity) {
                entity->physics.acceleration.y = GRAVITY;
            }
            
            if (entity->type == EntityType::BALL) {
                // Simple euler integration for balls
                entity->physics.velocity += entity->physics.acceleration * deltaTime;
                entity->position += entity->physics.velocity * deltaTime;
            }
            else if (entity->type == EntityType::CLOTH) {
                updateCloth(entity, deltaTime);
            }
        }
    }
    
    void updateCloth(std::shared_ptr<Entity> cloth, float deltaTime) {
        // Reset forces
        for (auto& force : cloth->clothForces) {
            force = glm::vec3(0.0f);
        }
        
        // Apply gravity to each particle
        for (int i = 0; i < cloth->clothVertices.size(); i++) {
            cloth->clothForces[i].y += GRAVITY * cloth->physics.mass;
        }
        
        // Top row particles are fixed (pinned)
        for (int i = 0; i < cloth->clothResolution; i++) {
            cloth->clothForces[i] = glm::vec3(0.0f);
        }
        
        // Apply spring forces
        for (const auto& spring : cloth->clothSprings) {
            int i1 = spring.first;
            int i2 = spring.second;
            
            glm::vec3 pos1 = cloth->clothVertices[i1] + cloth->position;
            glm::vec3 pos2 = cloth->clothVertices[i2] + cloth->position;
            
            glm::vec3 vel1 = cloth->clothVelocities[i1];
            glm::vec3 vel2 = cloth->clothVelocities[i2];
            
            glm::vec3 deltaPos = pos2 - pos1;
            float distance = glm::length(deltaPos);
            
            // Get original rest length (approximate as grid spacing)
            float restLength = cloth->clothSize / (cloth->clothResolution - 1);
            
            // Spring force (Hooke's law)
            glm::vec3 direction = distance > 0.0001f ? deltaPos / distance : glm::vec3(1, 0, 0);
            float springForce = cloth->clothStiffness * (distance - restLength);
            
            // Damping force (reduces oscillation)
            glm::vec3 deltaVel = vel2 - vel1;
            float dampingForce = cloth->clothDamping * glm::dot(deltaVel, direction);
            
            glm::vec3 force = direction * (springForce + dampingForce);
            
            // Apply forces (equal and opposite)
            if (i1 >= cloth->clothResolution) { // Don't move pinned particles
                cloth->clothForces[i1] += force;
            }
            if (i2 >= cloth->clothResolution) { // Don't move pinned particles
                cloth->clothForces[i2] -= force;
            }
        }
        
        // Integrate forces
        for (int i = 0; i < cloth->clothVertices.size(); i++) {
            // Don't move pinned particles
            if (i < cloth->clothResolution) {
                cloth->clothVelocities[i] = glm::vec3(0.0f);
                continue;
            }
            
            // Semi-implicit Euler integration
            cloth->clothVelocities[i] += cloth->clothForces[i] * deltaTime / cloth->physics.mass;
            cloth->clothVertices[i] += cloth->clothVelocities[i] * deltaTime;
            
            // Add some damping to prevent explosion
            cloth->clothVelocities[i] *= 0.99f;
        }
    }
    
    void resolveCollisions(Scene& scene) {
        auto& entities = scene.getEntities();
        
        // Iterate through all entities
        for (auto& entity : entities) {
            if (entity->physics.isStatic) continue;
            
            // Check for ball collisions with planes
            if (entity->type == EntityType::BALL) {
                for (auto& other : entities) {
                    if (other->type == EntityType::PLANE) {
                        resolveCollisionBallPlane(entity, other);
                    } else if (other->type == EntityType::BALL && other->id != entity->id) {
                        resolveCollisionBallBall(entity, other);
                    }
                }
            }
            // Check for cloth collisions with other objects
            else if (entity->type == EntityType::CLOTH) {
                for (auto& other : entities) {
                    if (other->type == EntityType::PLANE) {
                        resolveCollisionClothPlane(entity, other);
                    } else if (other->type == EntityType::BALL) {
                        resolveCollisionClothBall(entity, other);
                    }
                }
            }
        }
    }
    
    void resolveCollisionBallPlane(std::shared_ptr<Entity> ball, std::shared_ptr<Entity> plane) {
        float ballRadius = ball->scale.x; // Assuming uniform scale
        
        // Plane properties
        glm::vec3 planeNormal = glm::vec3(0.0f, 1.0f, 0.0f); // Assuming plane is always facing up
        float planeY = plane->position.y;
        
        // Check distance to plane
        float distance = ball->position.y - planeY - ballRadius;
        
        // If penetrating, resolve collision
        if (distance < 0) {
            // Move ball out of plane
            ball->position.y = planeY + ballRadius;
            
            // Reflect velocity with damping (restitution)
            float restitution = ball->physics.restitution;
            ball->physics.velocity.y = -ball->physics.velocity.y * restitution;
            
            // Apply friction to horizontal velocity
            float friction = ball->physics.friction;
            ball->physics.velocity.x *= (1.0f - friction);
            ball->physics.velocity.z *= (1.0f - friction);
        }
    }
    
    void resolveCollisionBallBall(std::shared_ptr<Entity> ball1, std::shared_ptr<Entity> ball2) {
        float radius1 = ball1->scale.x; // Assuming uniform scale
        float radius2 = ball2->scale.x;
        
        glm::vec3 delta = ball2->position - ball1->position;
        float distance = glm::length(delta);
        float minDist = radius1 + radius2;
        
        // Check for collision
        if (distance < minDist) {
            // Calculate penetration depth
            float penetration = minDist - distance;
            
            // Normalize delta
            glm::vec3 normal = distance > 0.0001f ? delta / distance : glm::vec3(1, 0, 0);
            
            // Calculate relative velocity
            glm::vec3 relVel = ball2->physics.velocity - ball1->physics.velocity;
            
            // Calculate impulse
            float restitution = std::min(ball1->physics.restitution, ball2->physics.restitution);
            float impulseMag = -(1.0f + restitution) * glm::dot(relVel, normal);
            impulseMag /= 1.0f/ball1->physics.mass + 1.0f/ball2->physics.mass;
            
            // Apply impulse
            if (!ball1->physics.isStatic) {
                ball1->physics.velocity -= impulseMag * normal / ball1->physics.mass;
            }
            if (!ball2->physics.isStatic) {
                ball2->physics.velocity += impulseMag * normal / ball2->physics.mass;
            }
            
            // Separate the balls
            if (!ball1->physics.isStatic && !ball2->physics.isStatic) {
                ball1->position -= normal * penetration * 0.5f;
                ball2->position += normal * penetration * 0.5f;
            } else if (!ball1->physics.isStatic) {
                ball1->position -= normal * penetration;
            } else if (!ball2->physics.isStatic) {
                ball2->position += normal * penetration;
            }
        }
    }
    
    void resolveCollisionClothPlane(std::shared_ptr<Entity> cloth, std::shared_ptr<Entity> plane) {
        float planeY = plane->position.y;
        
        // For each cloth vertex, check for collision with plane
        for (int i = 0; i < cloth->clothVertices.size(); i++) {
            glm::vec3 particlePos = cloth->clothVertices[i] + cloth->position;
            
            // If particle is below plane
            if (particlePos.y < planeY) {
                // Move particle to plane surface
                cloth->clothVertices[i].y = planeY - cloth->position.y;
                
                // Reflect velocity with damping
                cloth->clothVelocities[i].y = -cloth->clothVelocities[i].y * cloth->physics.restitution;
                
                // Apply friction to horizontal velocity
                float friction = cloth->physics.friction;
                cloth->clothVelocities[i].x *= (1.0f - friction);
                cloth->clothVelocities[i].z *= (1.0f - friction);
            }
        }
    }
    
    void resolveCollisionClothBall(std::shared_ptr<Entity> cloth, std::shared_ptr<Entity> ball) {
        float ballRadius = ball->scale.x; // Assuming uniform scale
        
        // For each cloth vertex, check for collision with ball
        for (int i = 0; i < cloth->clothVertices.size(); i++) {
            glm::vec3 particlePos = cloth->clothVertices[i] + cloth->position;
            glm::vec3 delta = particlePos - ball->position;
            float distance = glm::length(delta);
            
            // If particle is inside ball
            if (distance < ballRadius) {
                // Normalize direction
                glm::vec3 normal = distance > 0.0001f ? delta / distance : glm::vec3(0, 1, 0);
                
                // Move particle to ball surface
                cloth->clothVertices[i] = ball->position + normal * ballRadius - cloth->position;
                
                // Reflect velocity with damping
                float restitution = std::min(cloth->physics.restitution, ball->physics.restitution);
                
                // Calculate velocity component along normal
                float velAlongNormal = glm::dot(cloth->clothVelocities[i], normal);
                
                // Only reflect if moving towards each other
                if (velAlongNormal < 0) {
                    cloth->clothVelocities[i] -= (1.0f + restitution) * velAlongNormal * normal;
                }
                
                // Also affect ball slightly (much less if ball is heavier)
                if (!ball->physics.isStatic) {
                    float massRatio = cloth->physics.mass / (ball->physics.mass * cloth->clothVertices.size());
                    ball->physics.velocity -= normal * velAlongNormal * massRatio;
                }
            }
        }
    }
};

// --- Renderer Class ---
class Renderer {
private:
    // Mesh data for primitive shapes
    std::vector<float> sphereVertices;
    std::vector<unsigned int> sphereIndices;
    std::vector<float> planeVertices;
    std::vector<unsigned int> planeIndices;
    
    // OpenGL shader program
    GLuint shaderProgram;
    
    // Uniform locations
    GLint modelLoc, viewLoc, projLoc;
    GLint objectColorLoc, lightPosLoc, lightColorLoc, viewPosLoc;
    
    void createPrimitiveMeshes() {
        // Create Sphere mesh data
        createSphere(1.0f, 16, 16);
        
        // Create Plane mesh data
        createPlane(1.0f);
    }
    
    void createSphere(float radius, int sectors, int stacks) {
        sphereVertices.clear();
        sphereIndices.clear();
        
        float sectorStep = 2.0f * M_PI / sectors;
        float stackStep = M_PI / stacks;
        
        for (int i = 0; i <= stacks; ++i) {
            float stackAngle = M_PI / 2.0f - i * stackStep;
            float xy = radius * cos(stackAngle);
            float z = radius * sin(stackAngle);
            
            for (int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * sectorStep;
                
                // Vertex position
                float x = xy * cos(sectorAngle);
                float y = xy * sin(sectorAngle);
                
                // Normal vector
                float nx = x / radius;
                float ny = y / radius;
                float nz = z / radius;
                
                // Add vertex data
                sphereVertices.push_back(x);
                sphereVertices.push_back(y);
                sphereVertices.push_back(z);
                sphereVertices.push_back(nx);
                sphereVertices.push_back(ny);
                sphereVertices.push_back(nz);
            }
        }
        
        // Generate indices
        for (int i = 0; i < stacks; ++i) {
            int k1 = i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            
            for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if (i != 0) {
                    sphereIndices.push_back(k1);
                    sphereIndices.push_back(k2);
                    sphereIndices.push_back(k1 + 1);
                }
                
                if (i != (stacks - 1)) {
                    sphereIndices.push_back(k1 + 1);
                    sphereIndices.push_back(k2);
                    sphereIndices.push_back(k2 + 1);
                }
            }
        }
    }
    
    void createPlane(float size) {
        planeVertices = {
            // Positions           // Normals
            -size, 0.0f, -size,    0.0f, 1.0f, 0.0f,
             size, 0.0f, -size,    0.0f, 1.0f, 0.0f,
             size, 0.0f,  size,    0.0f, 1.0f, 0.0f,
            -size, 0.0f,  size,    0.0f, 1.0f, 0.0f
        };
        
        planeIndices = {
            0, 1, 2,
            0, 2, 3
        };
    }
    
    GLuint createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource) {
        // Vertex shader
        GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        // Check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // Fragment shader
        GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        // Check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        }
        
        // Link shaders
        GLuint program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);
        
        // Check for linking errors
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        }
        
        // Delete shaders after linking
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return program;
    }
    
    void configureBuffers(std::shared_ptr<Entity> entity) {
        if (entity->VAO != 0) {
            // Already configured
            return;
        }
        
        glGenVertexArrays(1, &entity->VAO);
        glGenBuffers(1, &entity->VBO);
        glGenBuffers(1, &entity->EBO);
        
        glBindVertexArray(entity->VAO);
        
        if (entity->type == EntityType::BALL) {
            // Use sphere mesh data
            glBindBuffer(GL_ARRAY_BUFFER, entity->VBO);
            glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entity->EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
            
            entity->indexCount = sphereIndices.size();
        }
        else if (entity->type == EntityType::PLANE) {
            // Use plane mesh data
            glBindBuffer(GL_ARRAY_BUFFER, entity->VBO);
            glBufferData(GL_ARRAY_BUFFER, planeVertices.size() * sizeof(float), planeVertices.data(), GL_STATIC_DRAW);
            
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entity->EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, planeIndices.size() * sizeof(unsigned int), planeIndices.data(), GL_STATIC_DRAW);
            
            entity->indexCount = planeIndices.size();
        }
        else if (entity->type == EntityType::CLOTH) {
            // Create dynamic buffers for cloth - will update each frame
            updateClothMesh(entity);
        }
        
        // Set vertex attribute pointers
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        glBindVertexArray(0);
    }
    
    void updateClothMesh(std::shared_ptr<Entity> cloth) {
        if (cloth->type != EntityType::CLOTH) return;
        
        // Generate mesh data from cloth particles
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        
        // Create vertices with positions and normals
        for (int y = 0; y < cloth->clothResolution; y++) {
            for (int x = 0; x < cloth->clothResolution; x++) {
                int i = y * cloth->clothResolution + x;
                
                // Position
                glm::vec3 pos = cloth->clothVertices[i];
                vertices.push_back(pos.x);
                vertices.push_back(pos.y);
                vertices.push_back(pos.z);
                
                // Calculate normal using neighboring particles (simplified)
                glm::vec3 normal(0.0f, 1.0f, 0.0f); // Default normal
                
                if (x < cloth->clothResolution - 1 && y < cloth->clothResolution - 1) {
                    int i1 = i;
                    int i2 = i + 1;
                    int i3 = i + cloth->clothResolution;
                    
                    glm::vec3 p1 = cloth->clothVertices[i1];
                    glm::vec3 p2 = cloth->clothVertices[i2];
                    glm::vec3 p3 = cloth->clothVertices[i3];
                    
                    glm::vec3 v1 = p2 - p1;
                    glm::vec3 v2 = p3 - p1;
                    
                    normal = glm::normalize(glm::cross(v1, v2));
                }
                
                vertices.push_back(normal.x);
                vertices.push_back(normal.y);
                vertices.push_back(normal.z);
            }
        }
        
        // Create indices for triangles
        for (int y = 0; y < cloth->clothResolution - 1; y++) {
            for (int x = 0; x < cloth->clothResolution - 1; x++) {
                int i = y * cloth->clothResolution + x;
                
                // First triangle
                indices.push_back(i);
                indices.push_back(i + 1);
                indices.push_back(i + cloth->clothResolution);
                
                // Second triangle
                indices.push_back(i + 1);
                indices.push_back(i + cloth->clothResolution + 1);
                indices.push_back(i + cloth->clothResolution);
            }
        }
        
        // Update buffers
        glBindVertexArray(cloth->VAO);
        
        glBindBuffer(GL_ARRAY_BUFFER, cloth->VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloth->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);
        
        cloth->indexCount = indices.size();
        
        glBindVertexArray(0);
    }
    
public:
    Renderer() : shaderProgram(0) {}
    
    void initialize() {
        // Create primitive meshes
        createPrimitiveMeshes();
        
        // Create shader program
        shaderProgram = createShaderProgram(g_VertexShaderSrc, g_FragmentShaderSrc);
        
        // Get uniform locations
        modelLoc = glGetUniformLocation(shaderProgram, "model");
        viewLoc = glGetUniformLocation(shaderProgram, "view");
        projLoc = glGetUniformLocation(shaderProgram, "projection");
        objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
        lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
        viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
    }
    
    void render(Scene& scene) {
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Set up view and projection matrices
        glm::mat4 view = glm::lookAt(
            g_CameraPosition,
            g_CameraTarget,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        
        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f,
            100.0f
        );
        
        // Use our shader
        glUseProgram(shaderProgram);
        
        // Set light properties
        glm::vec3 lightPos = g_CameraPosition; // Light follows camera
        glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
        
        glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
        glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
        glUniform3fv(viewPosLoc, 1, glm::value_ptr(g_CameraPosition));
        
        // Set view and projection matrices
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
        
        // Render each entity
        for (auto& entity : scene.getEntities()) {
            // Configure buffers if not already done
            configureBuffers(entity);
            
            // For cloth, update the mesh data each frame
            if (entity->type == EntityType::CLOTH) {
                updateClothMesh(entity);
            }
            
            // Set model matrix
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, entity->position);
            model = glm::rotate(model, glm::radians(entity->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(entity->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(entity->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, entity->scale);
            
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            // Set color (with highlight if selected)
            glm::vec3 color = entity->isSelected ? 
                              glm::vec3(entity->color) * 1.3f : 
                              glm::vec3(entity->color);
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
            
            // Draw the entity
            glBindVertexArray(entity->VAO);
            glDrawElements(GL_TRIANGLES, entity->indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
    
    glm::vec3 screenToWorldRay(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection) {
        // Convert screen coordinates to normalized device coordinates
        float x = (2.0f * screenX) / WINDOW_WIDTH - 1.0f;
        float y = 1.0f - (2.0f * screenY) / WINDOW_HEIGHT;
        float z = 1.0f; // Forward in NDC
        
        // Homogeneous clip space
        glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);
        
        // Eye space
        glm::vec4 rayEye = glm::inverse(projection) * rayClip;
        rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
        
        // World space
        glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
        
        return rayWorld;
    }
    
    void cleanup() {
        glDeleteProgram(shaderProgram);
    }
};

// Helper function to implement in Entity class
void Entity::generateRenderingData(Renderer* renderer) {
    // This can be used to create mesh data if needed
}

// Implementation of ray casting for scene selection
bool Scene::selectEntityAtScreenPos(double screenX, double screenY, const glm::mat4& view, const glm::mat4& projection) {
    glm::vec3 rayDir = Renderer().screenToWorldRay(screenX, screenY, view, projection);
    glm::vec3 rayOrigin = g_CameraPosition;
    
    float closestT = std::numeric_limits<float>::max();
    std::shared_ptr<Entity> closestEntity = nullptr;
    
    for (auto& entity : entities) {
        // Simple sphere intersection test for all entities (simplified)
        float radius = glm::length(entity->scale) * 0.5f; // Approximation
        
        glm::vec3 oc = rayOrigin - entity->position;
        float a = glm::dot(rayDir, rayDir);
        float b = 2.0f * glm::dot(oc, rayDir);
        float c = glm::dot(oc, oc) - radius * radius;
        float discriminant = b*b - 4*a*c;
        
        if (discriminant > 0) {
            float t = (-b - sqrt(discriminant)) / (2.0f * a);
            if (t > 0 && t < closestT) {
                closestT = t;
                closestEntity = entity;
            }
        }
    }
    
    if (closestEntity) {
        setSelectedEntity(closestEntity);
        return true;
    }
    
    return false;
}

// --- Editor GUI Class ---
class EditorGUI {
private:
    Scene* scene;
    Renderer* renderer;
    PhysicsSystem* physics;
    bool showMetrics = false;
    
public:
    EditorGUI(Scene* _scene, Renderer* _renderer, PhysicsSystem* _physics)
        : scene(_scene), renderer(_renderer), physics(_physics) {}
    
    void initialize() {
        // Setup ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        
        // Setup ImGui style
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(g_Window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");
    }
    
    void render() {
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Set up docking
        setupDockSpace();
        
        // Render editor panels
        renderToolbar();
        renderSceneHierarchy();
        renderInspector();
        renderConsole();
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
    
    void setupDockSpace() {
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;
            
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, window_flags);
        ImGui::PopStyleVar();
        
        if (opt_fullscreen)
            ImGui::PopStyleVar(2);
            
        // Submit the DockSpace
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        
        ImGui::End();
    }
    
    void renderToolbar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                    scene->clear();
                }
                if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {
                    scene->loadFromFile("scene.json");
                }
                if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
                    scene->saveToFile("scene.json");
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(g_Window, true);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Delete Selected", "Del")) {
                    if (auto entity = scene->getSelectedEntity()) {
                        scene->removeEntity(entity->id);
                    }
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Ball", "Ctrl+1")) {
                    scene->addEntity("Ball", EntityType::BALL);
                }
                if (ImGui::MenuItem("Cloth", "Ctrl+2")) {
                    scene->addEntity("Cloth", EntityType::CLOTH);
                }
                if (ImGui::MenuItem("Plane", "Ctrl+3")) {
                    scene->addEntity("Plane", EntityType::PLANE);
                }
                ImGui::EndMenu();
            }
            
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Metrics", NULL, &showMetrics)) {}
                ImGui::EndMenu();
            }
            
            ImGui::EndMainMenuBar();
        }
        
        // Toolbar buttons
        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
        
        if (ImGui::Button(g_IsPlaying ? "Stop" : "Play", ImVec2(100, 30))) {
            g_IsPlaying = !g_IsPlaying;
            if (g_IsPlaying) {
                g_ConsoleOutput += "Started simulation.\n";
            } else {
                g_ConsoleOutput += "Stopped simulation.\n";
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Ball", ImVec2(100, 30))) {
            scene->addEntity("Ball", EntityType::BALL);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Cloth", ImVec2(100, 30))) {
            scene->addEntity("Cloth", EntityType::CLOTH);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Add Plane", ImVec2(100, 30))) {
            scene->addEntity("Plane", EntityType::PLANE);
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Save Scene", ImVec2(100, 30))) {
            scene->saveToFile("scene.json");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Load Scene", ImVec2(100, 30))) {
            scene->loadFromFile("scene.json");
        }
        
        ImGui::SameLine();
        if (ImGui::Button("AI Generate", ImVec2(100, 30))) {
            scene->generatedRandomScene();
        }
        
        ImGui::End();
        
        // Display metrics window if enabled
        if (showMetrics) {
            ImGui::ShowMetricsWindow(&showMetrics);
        }
    }
    
    void renderSceneHierarchy() {
        ImGui::Begin("Scene Hierarchy");
        
        for (auto& entity : scene->getEntities()) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            if (entity->isSelected) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            
            std::string label = entity->name + " (" + std::to_string(entity->id) + ")";
            
            if (ImGui::TreeNodeEx((void*)(intptr_t)entity->id, flags, "%s", label.c_str())) {
                if (ImGui::IsItemClicked()) {
                    scene->setSelectedEntity(entity);
                }
            }
        }
        
        // Right-click context menu
        if (ImGui::BeginPopupContextWindow()) {
            if (ImGui::MenuItem("Add Ball")) {
                scene->addEntity("Ball", EntityType::BALL);
            }
            if (ImGui::MenuItem("Add Cloth")) {
                scene->addEntity("Cloth", EntityType::CLOTH);
            }
            if (ImGui::MenuItem("Add Plane")) {
                scene->addEntity("Plane", EntityType::PLANE);
            }
            ImGui::EndPopup();
        }
        
        ImGui::End();
    }
    
    void renderInspector() {
        ImGui::Begin("Inspector");
        
        auto selectedEntity = scene->getSelectedEntity();
        if (selectedEntity) {
            // Entity name and type
            std::string typeName;
            switch (selectedEntity->type) {
                case EntityType::BALL: typeName = "Ball"; break;
                case EntityType::CLOTH: typeName = "Cloth"; break;
                case EntityType::PLANE: typeName = "Plane"; break;
                default: typeName = "Unknown"; break;
            }
            
            char nameBuffer[256];
            strcpy(nameBuffer, selectedEntity->name.c_str());
            if (ImGui::InputText("Name", nameBuffer, 256)) {
                selectedEntity->name = nameBuffer;
            }
            
            ImGui::Text("Type: %s", typeName.c_str());
            ImGui::Text("ID: %d", selectedEntity->id);
            ImGui::Separator();
            
            // Transform
            ImGui::Text("Transform");
            
            glm::vec3 position = selectedEntity->position;
            if (ImGui::DragFloat3("Position", glm::value_ptr(position), 0.1f)) {
                selectedEntity->position = position;
                selectedEntity->updateTransform();
            }
            
            glm::vec3 rotation = selectedEntity->rotation;
            if (ImGui::DragFloat3("Rotation", glm::value_ptr(rotation), 1.0f)) {
                selectedEntity->rotation = rotation;
                selectedEntity->updateTransform();
            }
            
            glm::vec3 scale = selectedEntity->scale;
            if (ImGui::DragFloat3("Scale", glm::value_ptr(scale), 0.1f, 0.1f, 100.0f)) {
                selectedEntity->scale = scale;
                selectedEntity->updateTransform();
            }
            
            ImGui::Separator();
            
            // Material/Color
            ImGui::Text("Material");
            glm::vec4 color = selectedEntity->color;
            if (ImGui::ColorEdit4("Color", glm::value_ptr(color))) {
                selectedEntity->color = color;
            }
            
            ImGui::Separator();
            
            // Physics properties
            ImGui::Text("Physics");
            
            bool isStatic = selectedEntity->physics.isStatic;
            if (ImGui::Checkbox("Static", &isStatic)) {
                selectedEntity->physics.isStatic = isStatic;
            }
            
            float mass = selectedEntity->physics.mass;
            if (ImGui::DragFloat("Mass", &mass, 0.1f, 0.1f, 100.0f)) {
                selectedEntity->physics.mass = mass;
            }
            
            float restitution = selectedEntity->physics.restitution;
            if (ImGui::SliderFloat("Restitution", &restitution, 0.0f, 1.0f)) {
                selectedEntity->physics.restitution = restitution;
            }
            
            float friction = selectedEntity->physics.friction;
            if (ImGui::SliderFloat("Friction", &friction, 0.0f, 1.0f)) {
                selectedEntity->physics.friction = friction;
            }
            
            bool useGravity = selectedEntity->physics.useGravity;
            if (ImGui::Checkbox("Use Gravity", &useGravity)) {
                selectedEntity->physics.useGravity = useGravity;
            }
            
            // Entity specific properties
            if (selectedEntity->type == EntityType::CLOTH) {
                ImGui::Separator();
                ImGui::Text("Cloth Properties");
                
                int resolution = selectedEntity->clothResolution;
                if (ImGui::SliderInt("Resolution", &resolution, 5, 30)) {
                    selectedEntity->clothResolution = resolution;
                    selectedEntity->initializeCloth();
                }
                
                float size = selectedEntity->clothSize;
                if (ImGui::SliderFloat("Size", &size, 1.0f, 20.0f)) {
                    selectedEntity->clothSize = size;
                    selectedEntity->initializeCloth();
                }
                
                float stiffness = selectedEntity->clothStiffness;
                if (ImGui::SliderFloat("Stiffness", &stiffness, 10.0f, 500.0f)) {
                    selectedEntity->clothStiffness = stiffness;
                }
                
                float damping = selectedEntity->clothDamping;
                if (ImGui::SliderFloat("Damping", &damping, 0.001f, 0.1f)) {
                    selectedEntity->clothDamping = damping;
                }
            }
        }
        else {
            ImGui::Text("No entity selected");
        }
        
        ImGui::End();
    }
    
    void renderConsole() {
        ImGui::Begin("Console");
        
        ImGui::TextWrapped("%s", g_ConsoleOutput.c_str());
        
        // Auto-scroll to bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);
        
        ImGui::End();
    }
    
    void cleanup() {
        // Cleanup ImGui
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
};

// --- Input Handling ---
void handleInput(GLFWwindow* window, Scene& scene, float deltaTime) {
    // Camera controls
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        g_CameraPosition += glm::normalize(g_CameraTarget - g_CameraPosition) * 5.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        g_CameraPosition -= glm::normalize(g_CameraTarget - g_CameraPosition) * 5.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
        g_CameraPosition -= right * 5.0f * deltaTime;
        g_CameraTarget -= right * 5.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
        g_CameraPosition += right * 5.0f * deltaTime;
        g_CameraTarget += right * 5.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        g_CameraPosition.y += 5.0f * deltaTime;
        g_CameraTarget.y += 5.0f * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        g_CameraPosition.y -= 5.0f * deltaTime;
        g_CameraTarget.y -= 5.0f * deltaTime;
    }
    
    // Shortcut keys
    static bool ctrlDown = false;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        ctrlDown = true;
    } else {
        ctrlDown = false;
    }
    
    // Ctrl+S: Save
    static bool savePressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (!savePressed) {
            scene.saveToFile("scene.json");
            savePressed = true;
        }
    } else {
        savePressed = false;
    }
    
    // Ctrl+O: Open
    static bool openPressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        if (!openPressed) {
            scene.loadFromFile("scene.json");
            openPressed = true;
        }
    } else {
        openPressed = false;
    }
    
    // Ctrl+N: New Scene
    static bool newPressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (!newPressed) {
            scene.clear();
            newPressed = true;
        }
    } else {
        newPressed = false;
    }
    
    // Delete: Remove selected entity
    static bool deletePressed = false;
    if (glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS) {
        if (!deletePressed) {
            if (auto selectedEntity = scene.getSelectedEntity()) {
                scene.removeEntity(selectedEntity->id);
            }
            deletePressed = true;
        }
    } else {
        deletePressed = false;
    }
    
    // 1,2,3: Add entities
    static bool key1Pressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!key1Pressed) {
            scene.addEntity("Ball", EntityType::BALL);
            key1Pressed = true;
        }
    } else {
        key1Pressed = false;
    }
    
    static bool key2Pressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (!key2Pressed) {
            scene.addEntity("Cloth", EntityType::CLOTH);
            key2Pressed = true;
        }
    } else {
        key2Pressed = false;
    }
    
    static bool key3Pressed = false;
    if (ctrlDown && glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        if (!key3Pressed) {
            scene.addEntity("Plane", EntityType::PLANE);
            key3Pressed = true;
        }
    } else {
        key3Pressed = false;
    }
    
    // Space: Play/Pause
    static bool spacePressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spacePressed) {
            g_IsPlaying = !g_IsPlaying;
            spacePressed = true;
        }
    } else {
        spacePressed = false;
    }
}

// --- Mouse Callbacks ---
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // Don't handle mouse if ImGui is using it
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            g_IsMouseDown = true;
            g_IsDragging = false;
        } else if (action == GLFW_RELEASE) {
            g_IsMouseDown = false;
            g_IsDragging = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // Don't handle mouse if ImGui is using it
    
    static double lastX = xpos;
    static double lastY = ypos;
    
    g_MouseDeltaX = xpos - lastX;
    g_MouseDeltaY = ypos - lastY;
    
    // Store current position for next frame
    lastX = xpos;
    lastY = ypos;
    
    // Update global mouse position
    g_MouseX = xpos;
    g_MouseY = ypos;
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return; // Don't handle scroll if ImGui is using it
    
    // Zoom camera in/out
    float zoomSpeed = 0.1f;
    g_CameraZoom += yoffset * zoomSpeed;
    g_CameraZoom = glm::clamp(g_CameraZoom, 0.1f, 10.0f);
    
    // Update camera position based on zoom
    glm::vec3 direction = glm::normalize(g_CameraTarget - g_CameraPosition);
    float distance = glm::length(g_CameraTarget - g_CameraPosition);
    float newDistance = distance * (1.0f - yoffset * zoomSpeed);
    
    g_CameraPosition = g_CameraTarget - direction * newDistance;
}

// --- GLFW Error Callback ---
void errorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// --- OpenGL Debug Callback ---
void APIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity,
                           GLsizei length, const GLchar *message, const void *userParam) {
    // Ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    
    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
    
    switch (source) {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    }
    std::cout << std::endl;
    
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    }
    std::cout << std::endl;
    
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

// --- Main Function ---
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Set GLFW error callback
    glfwSetErrorCallback(errorCallback);
    
    // Set OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    
    // Enable debug output
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    
    // Create window
    g_Window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (!g_Window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Make OpenGL context current
    glfwMakeContextCurrent(g_Window);
    
    // Set vsync
    glfwSwapInterval(1);
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    // Set up OpenGL debug callback
    if (GLEW_ARB_debug_output) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    
    // Set up viewport
    int width, height;
    glfwGetFramebufferSize(g_Window, &width, &height);
    glViewport(0, 0, width, height);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Register callbacks
    glfwSetMouseButtonCallback(g_Window, mouseButtonCallback);
    glfwSetCursorPosCallback(g_Window, cursorPosCallback);
    glfwSetScrollCallback(g_Window, scrollCallback);
    
    // Create systems
    Scene scene;
    Renderer renderer;
    PhysicsSystem physics;
    EditorGUI editor(&scene, &renderer, &physics);
    
    // Initialize systems
    renderer.initialize();
    editor.initialize();
    
    // Add a default ground plane
    auto plane = scene.addEntity("Ground", EntityType::PLANE);
    plane->position = glm::vec3(0.0f, -0.05f, 0.0f);
    plane->scale = glm::vec3(15.0f, 0.1f, 15.0f);
    plane->physics.isStatic = true;
    
    // Main loop
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(g_Window)) {
        // Calculate delta time
        double currentTime = glfwGetTime();
        g_DeltaTime = float(currentTime - lastTime);
        lastTime = currentTime;
        
        // Poll events
        glfwPollEvents();
        
        // Handle input
        handleInput(g_Window, scene, g_DeltaTime);
        
        // Update physics
        physics.update(scene, g_DeltaTime);
        
        // Handle mouse picking
        static bool wasMouseDown = false;
        if (g_IsMouseDown && !wasMouseDown) {
            // Mouse just pressed
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                // Get view and projection matrices for picking
                glm::mat4 view = glm::lookAt(
                    g_CameraPosition,
                    g_CameraTarget,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
                
                glm::mat4 projection = glm::perspective(
                    glm::radians(45.0f),
                    (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                    0.1f,
                    100.0f
                );
                
                // Try to select entity
                if (!scene.selectEntityAtScreenPos(g_MouseX, g_MouseY, view, projection)) {
                    // If no entity was selected, deselect current
                    scene.setSelectedEntity(nullptr);
                }
            }
        }
        
        // Handle dragging
        if (g_IsMouseDown && !g_IsDragging && (abs(g_MouseDeltaX) > 1.0 || abs(g_MouseDeltaY) > 1.0)) {
            g_IsDragging = true;
        }
        
        if (g_IsDragging) {
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse) {
                auto selectedEntity = scene.getSelectedEntity();
                if (selectedEntity && !g_IsPlaying && !selectedEntity->physics.isStatic) {
                    // Calculate move in world space
                    glm::mat4 view = glm::lookAt(
                        g_CameraPosition,
                        g_CameraTarget,
                        glm::vec3(0.0f, 1.0f, 0.0f)
                    );
                    
                    glm::mat4 projection = glm::perspective(
                        glm::radians(45.0f),
                        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
                        0.1f,
                        100.0f
                    );
                    
                    // Get screen point depths
                    glm::vec3 rayDir = renderer.screenToWorldRay(g_MouseX, g_MouseY, view, projection);
                    float distance = glm::length(selectedEntity->position - g_CameraPosition) / glm::dot(rayDir, glm::normalize(g_CameraTarget - g_CameraPosition));
                    
                    // Calculate world movement
                    glm::vec3 screenPoint = g_CameraPosition + rayDir * distance;
                    
                    // Move entity
                    selectedEntity->position = screenPoint;
                    selectedEntity->updateTransform();
                }
                else if (!selectedEntity) {
                    // Rotate camera around target
                    float rotationSpeed = 0.01f;
                    float horizontalAngle = g_MouseDeltaX * rotationSpeed;
                    float verticalAngle = g_MouseDeltaY * rotationSpeed;
                    
                    // Calculate distance from camera to target
                    float distance = glm::length(g_CameraPosition - g_CameraTarget);
                    
                    // Create rotation quaternions
                    glm::quat horizontalRot = glm::angleAxis(horizontalAngle, glm::vec3(0, 1, 0));
                    
                    // Get right vector for vertical rotation
                    glm::vec3 right = glm::normalize(glm::cross(g_CameraTarget - g_CameraPosition, glm::vec3(0, 1, 0)));
                    glm::quat verticalRot = glm::angleAxis(verticalAngle, right);
                    
                    // Rotate camera position around target
                    glm::vec3 dir = glm::normalize(g_CameraPosition - g_CameraTarget);
                    dir = glm::rotate(horizontalRot * verticalRot, dir);
                    
                    // Set new camera position
                    g_CameraPosition = g_CameraTarget + dir * distance;
                }
            }
        }
        
        wasMouseDown = g_IsMouseDown;
        
        // Render scene
        renderer.render(scene);
        
        // Render GUI
        editor.render();
        
        // Swap buffers
        glfwSwapBuffers(g_Window);
    }
    
    // Cleanup
    editor.cleanup();
    renderer.cleanup();
    
    // Terminate GLFW
    glfwDestroyWindow(g_Window);
    glfwTerminate();
    
    return 0;
}
