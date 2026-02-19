#include "DebugDraw.hpp"
#include "../../Core/Logger.hpp"
#include "../../Core/Application.hpp"
#include "../../Scene/SceneManager.hpp"
#include "../../Graphics/RenderCommand.hpp"
#include "../../Physics/PhysicsWorld.hpp"
#include "../../Physics/Shapes/SphereShape.hpp"
#include "../../Physics/Shapes/BoxShape.hpp"
#include "../../Physics/Shapes/CapsuleShape.hpp"
#include "../../Physics/Shapes/PlaneShape.hpp"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace GameEngine {

    std::vector<DebugDraw::DebugLine> DebugDraw::s_Lines;
    std::mutex DebugDraw::s_LinesMutex;
    Ref<Shader> DebugDraw::s_Shader;
    uint32_t DebugDraw::s_VAO = 0;
    uint32_t DebugDraw::s_VBO = 0;
    bool DebugDraw::s_DepthTest = true;

    void DebugDraw::Init() {
        // Create shader
        const char* vertexSrc = R"(
            #version 420 core
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec3 a_Color;
            
            uniform mat4 u_ViewProjection;
            
            out vec3 v_Color;
            
            void main() {
                v_Color = a_Color;
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";
        
        const char* fragmentSrc = R"(
            #version 420 core
            layout(location = 0) out vec4 FragColor;
            
            in vec3 v_Color;
            
            void main() {
                FragColor = vec4(v_Color, 1.0);
            }
        )";
        
        s_Shader = CreateRef<Shader>("DebugDraw", vertexSrc, fragmentSrc);
        
        // Create VAO/VBO
        glGenVertexArrays(1, &s_VAO);
        glGenBuffers(1, &s_VBO);
        
        glBindVertexArray(s_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
        
        // Position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        
        // Color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glBindVertexArray(0);
        
        GE_CORE_INFO("DebugDraw initialized");
    }

    void DebugDraw::Shutdown() {
        if (s_VAO) {
            glDeleteVertexArrays(1, &s_VAO);
            s_VAO = 0;
        }
        
        if (s_VBO) {
            glDeleteBuffers(1, &s_VBO);
            s_VBO = 0;
        }
        
        s_Shader.reset();
        {
            std::lock_guard<std::mutex> lock(s_LinesMutex);
            s_Lines.clear();
        }
    }

    void DebugDraw::Update(float deltaTime) {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        // Remove expired lines (duration > 0 that have elapsed)
        s_Lines.erase(
            std::remove_if(s_Lines.begin(), s_Lines.end(), [deltaTime](DebugLine& line) {
                if (line.RemainingTime > 0) {
                    line.RemainingTime -= deltaTime;
                    return line.RemainingTime <= 0;
                }
                return false;  // Keep zero-duration (single-frame) and persistent lines
            }),
            s_Lines.end()
        );
        // Remove zero-duration (single-frame) lines after they would have been rendered this frame.
        // Caller is responsible for calling Render() once per frame; Update() only expires lines.
        s_Lines.erase(
            std::remove_if(s_Lines.begin(), s_Lines.end(), [](const DebugLine& line) {
                return line.RemainingTime == 0.0f;
            }),
            s_Lines.end()
        );
    }

    void DebugDraw::Render(const Camera3D& camera) {
        std::vector<DebugLine> linesCopy;
        {
            std::lock_guard<std::mutex> lock(s_LinesMutex);
            if (s_Lines.empty()) return;
            linesCopy = s_Lines;
        }
        
        // Prepare vertex data
        std::vector<float> vertices;
        vertices.reserve(linesCopy.size() * 12);  // 2 vertices * 6 floats per line
        
        for (const auto& line : linesCopy) {
            // Start vertex
            vertices.push_back(line.Start.x);
            vertices.push_back(line.Start.y);
            vertices.push_back(line.Start.z);
            vertices.push_back(line.Color.r);
            vertices.push_back(line.Color.g);
            vertices.push_back(line.Color.b);
            
            // End vertex
            vertices.push_back(line.End.x);
            vertices.push_back(line.End.y);
            vertices.push_back(line.End.z);
            vertices.push_back(line.Color.r);
            vertices.push_back(line.Color.g);
            vertices.push_back(line.Color.b);
        }
        
        // Upload data
        glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        
        // Render
        s_Shader->Bind();
        s_Shader->SetUniformMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
        
        // Set rendering state
        bool prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
        if (!s_DepthTest && prevDepthTest) {
            glDisable(GL_DEPTH_TEST);
        }
        
        glBindVertexArray(s_VAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(linesCopy.size() * 2));
        glBindVertexArray(0);
        
        // Restore state
        if (!s_DepthTest && prevDepthTest) {
            glEnable(GL_DEPTH_TEST);
        }
        
        s_Shader->Unbind();
    }

    void DebugDraw::Render(const glm::mat4& viewProjectionMatrix) {
        if (!s_Shader || s_VAO == 0) return;

        std::vector<DebugLine> linesCopy;
        {
            std::lock_guard<std::mutex> lock(s_LinesMutex);
            if (s_Lines.empty()) return;
            linesCopy = s_Lines;
        }

        std::vector<float> vertices;
        vertices.reserve(linesCopy.size() * 12);

        for (const auto& line : linesCopy) {
            vertices.push_back(line.Start.x);
            vertices.push_back(line.Start.y);
            vertices.push_back(line.Start.z);
            vertices.push_back(line.Color.r);
            vertices.push_back(line.Color.g);
            vertices.push_back(line.Color.b);

            vertices.push_back(line.End.x);
            vertices.push_back(line.End.y);
            vertices.push_back(line.End.z);
            vertices.push_back(line.Color.r);
            vertices.push_back(line.Color.g);
            vertices.push_back(line.Color.b);
        }

        glBindBuffer(GL_ARRAY_BUFFER, s_VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

        s_Shader->Bind();
        s_Shader->SetUniformMat4("u_ViewProjection", viewProjectionMatrix);

        bool prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
        if (!s_DepthTest && prevDepthTest) {
            glDisable(GL_DEPTH_TEST);
        }

        glBindVertexArray(s_VAO);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(linesCopy.size() * 2));
        glBindVertexArray(0);

        if (!s_DepthTest && prevDepthTest) {
            glEnable(GL_DEPTH_TEST);
        }

        s_Shader->Unbind();
    }

    void DebugDraw::FlushSingleFrame() {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.erase(
            std::remove_if(s_Lines.begin(), s_Lines.end(), [](const DebugLine& line) {
                return line.RemainingTime == 0.0f;
            }),
            s_Lines.end()
        );
    }

    void DebugDraw::Clear() {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.clear();
    }

    void DebugDraw::SetDepthTest(bool enabled) {
        s_DepthTest = enabled;
    }

    void DebugDraw::AddLine(const glm::vec3& start, const glm::vec3& end, 
                           const glm::vec3& color, float duration) {
        std::lock_guard<std::mutex> lock(s_LinesMutex);
        s_Lines.push_back({ start, end, color, duration });
    }

    void DebugDraw::DrawLine(const glm::vec3& start, const glm::vec3& end, 
                            const glm::vec3& color, float duration) {
        AddLine(start, end, color, duration);
    }

    void DebugDraw::DrawRay(const glm::vec3& origin, const glm::vec3& direction, 
                           const glm::vec3& color, float duration) {
        DrawLine(origin, origin + direction, color, duration);
    }

    void DebugDraw::DrawBox(const glm::vec3& center, const glm::vec3& size, 
                           const glm::quat& rotation, const glm::vec3& color, float duration) {
        glm::vec3 halfSize = size * 0.5f;
        
        // Define 8 corners
        glm::vec3 corners[8] = {
            glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z),
            glm::vec3( halfSize.x, -halfSize.y, -halfSize.z),
            glm::vec3( halfSize.x,  halfSize.y, -halfSize.z),
            glm::vec3(-halfSize.x,  halfSize.y, -halfSize.z),
            glm::vec3(-halfSize.x, -halfSize.y,  halfSize.z),
            glm::vec3( halfSize.x, -halfSize.y,  halfSize.z),
            glm::vec3( halfSize.x,  halfSize.y,  halfSize.z),
            glm::vec3(-halfSize.x,  halfSize.y,  halfSize.z)
        };
        
        // Transform corners
        for (int i = 0; i < 8; i++) {
            corners[i] = center + rotation * corners[i];
        }
        
        // Draw 12 edges
        DrawLine(corners[0], corners[1], color, duration);
        DrawLine(corners[1], corners[2], color, duration);
        DrawLine(corners[2], corners[3], color, duration);
        DrawLine(corners[3], corners[0], color, duration);
        
        DrawLine(corners[4], corners[5], color, duration);
        DrawLine(corners[5], corners[6], color, duration);
        DrawLine(corners[6], corners[7], color, duration);
        DrawLine(corners[7], corners[4], color, duration);
        
        DrawLine(corners[0], corners[4], color, duration);
        DrawLine(corners[1], corners[5], color, duration);
        DrawLine(corners[2], corners[6], color, duration);
        DrawLine(corners[3], corners[7], color, duration);
    }

    void DebugDraw::DrawSphere(const glm::vec3& center, float radius, 
                               const glm::vec3& color, float duration, int segments) {
        float angleStep = glm::two_pi<float>() / segments;
        
        // Draw three perpendicular circles
        for (int axis = 0; axis < 3; axis++) {
            for (int i = 0; i < segments; i++) {
                float angle1 = i * angleStep;
                float angle2 = (i + 1) * angleStep;
                
                glm::vec3 p1, p2;
                
                if (axis == 0) {  // YZ plane
                    p1 = glm::vec3(0, cos(angle1), sin(angle1)) * radius;
                    p2 = glm::vec3(0, cos(angle2), sin(angle2)) * radius;
                } else if (axis == 1) {  // XZ plane
                    p1 = glm::vec3(cos(angle1), 0, sin(angle1)) * radius;
                    p2 = glm::vec3(cos(angle2), 0, sin(angle2)) * radius;
                } else {  // XY plane
                    p1 = glm::vec3(cos(angle1), sin(angle1), 0) * radius;
                    p2 = glm::vec3(cos(angle2), sin(angle2), 0) * radius;
                }
                
                DrawLine(center + p1, center + p2, color, duration);
            }
        }
    }

    void DebugDraw::DrawCapsule(const glm::vec3& center, float radius, float height,
                                const glm::quat& rotation, const glm::vec3& color, float duration) {
        glm::vec3 up = rotation * glm::vec3(0, 1, 0);
        float halfCylinder = (height - 2.0f * radius) * 0.5f;
        
        glm::vec3 topCenter = center + up * halfCylinder;
        glm::vec3 bottomCenter = center - up * halfCylinder;
        
        // Draw top hemisphere
        DrawSphere(topCenter, radius, color, duration, 16);
        
        // Draw bottom hemisphere
        DrawSphere(bottomCenter, radius, color, duration, 16);
        
        // Draw connecting lines
        glm::vec3 right = rotation * glm::vec3(1, 0, 0);
        glm::vec3 forward = rotation * glm::vec3(0, 0, 1);
        
        DrawLine(topCenter + right * radius, bottomCenter + right * radius, color, duration);
        DrawLine(topCenter - right * radius, bottomCenter - right * radius, color, duration);
        DrawLine(topCenter + forward * radius, bottomCenter + forward * radius, color, duration);
        DrawLine(topCenter - forward * radius, bottomCenter - forward * radius, color, duration);
    }

    void DebugDraw::DrawGrid(float size, int divisions, const glm::vec3& color) {
        float step = size / divisions;
        float halfSize = size * 0.5f;
        
        for (int i = 0; i <= divisions; i++) {
            float pos = -halfSize + i * step;
            
            // X-axis lines
            DrawLine(glm::vec3(pos, 0, -halfSize), glm::vec3(pos, 0, halfSize), color);
            
            // Z-axis lines
            DrawLine(glm::vec3(-halfSize, 0, pos), glm::vec3(halfSize, 0, pos), color);
        }
    }

    void DebugDraw::DrawAxes(const glm::vec3& origin, float length, float duration) {
        DrawLine(origin, origin + glm::vec3(length, 0, 0), glm::vec3(1, 0, 0), duration);  // X = Red
        DrawLine(origin, origin + glm::vec3(0, length, 0), glm::vec3(0, 1, 0), duration);  // Y = Green
        DrawLine(origin, origin + glm::vec3(0, 0, length), glm::vec3(0, 0, 1), duration);  // Z = Blue
    }

    void DebugDraw::DrawCross(const glm::vec3& position, float size, 
                             const glm::vec3& color, float duration) {
        float halfSize = size * 0.5f;
        
        DrawLine(position - glm::vec3(halfSize, 0, 0), position + glm::vec3(halfSize, 0, 0), color, duration);
        DrawLine(position - glm::vec3(0, halfSize, 0), position + glm::vec3(0, halfSize, 0), color, duration);
        DrawLine(position - glm::vec3(0, 0, halfSize), position + glm::vec3(0, 0, halfSize), color, duration);
    }

    void DebugDraw::DrawFrustum(const glm::mat4& viewProjection, 
                                const glm::vec3& color, float duration) {
        // Extract frustum corners from inverse view-projection matrix
        glm::mat4 invVP = glm::inverse(viewProjection);
        
        glm::vec3 corners[8];
        int index = 0;
        
        for (int z = 0; z < 2; z++) {
            for (int y = 0; y < 2; y++) {
                for (int x = 0; x < 2; x++) {
                    glm::vec4 corner = invVP * glm::vec4(
                        x * 2.0f - 1.0f,
                        y * 2.0f - 1.0f,
                        z * 2.0f - 1.0f,
                        1.0f
                    );
                    corners[index++] = glm::vec3(corner) / corner.w;
                }
            }
        }
        
        // Draw near plane
        DrawLine(corners[0], corners[1], color, duration);
        DrawLine(corners[1], corners[3], color, duration);
        DrawLine(corners[3], corners[2], color, duration);
        DrawLine(corners[2], corners[0], color, duration);
        
        // Draw far plane
        DrawLine(corners[4], corners[5], color, duration);
        DrawLine(corners[5], corners[7], color, duration);
        DrawLine(corners[7], corners[6], color, duration);
        DrawLine(corners[6], corners[4], color, duration);
        
        // Draw connecting lines
        DrawLine(corners[0], corners[4], color, duration);
        DrawLine(corners[1], corners[5], color, duration);
        DrawLine(corners[2], corners[6], color, duration);
        DrawLine(corners[3], corners[7], color, duration);
    }

    // ==================== Physics Debug Visualization ====================

    void DebugDraw::DrawCollider(const Ref<Physics::CollisionShape>& shape,
                                const glm::vec3& position,
                                const glm::quat& rotation,
                                const glm::vec3& color,
                                float duration) {
        if (!shape) return;
        
        switch (shape->GetType()) {
            case Physics::CollisionShape::ShapeType::Sphere: {
                auto sphere = std::static_pointer_cast<Physics::SphereShape>(shape);
                DrawSphere(position, sphere->GetRadius(), color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Box: {
                auto box = std::static_pointer_cast<Physics::BoxShape>(shape);
                DrawBox(position, box->GetHalfExtents() * 2.0f, rotation, color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Capsule: {
                auto capsule = std::static_pointer_cast<Physics::CapsuleShape>(shape);
                DrawCapsule(position, capsule->GetRadius(), capsule->GetHeight(), rotation, color, duration);
                break;
            }
            case Physics::CollisionShape::ShapeType::Plane: {
                auto plane = std::static_pointer_cast<Physics::PlaneShape>(shape);
                DrawPlane(rotation * plane->GetNormal(), plane->GetDistance(), 5.0f, color, duration);
                break;
            }
            default:
                break;
        }
    }

    void DebugDraw::DrawContact(const glm::vec3& position,
                               const glm::vec3& normal,
                               float penetration,
                               const glm::vec3& color,
                               float duration) {
        // Draw contact point marker
        DrawCross(position, 0.1f, color, duration);
        
        // Draw normal arrow
        DrawRay(position, normal * 0.5f, glm::vec3(1, 0, 0), duration);
        
        // Draw penetration indicator
        DrawLine(position, position - normal * penetration, glm::vec3(1, 1, 0), duration);
    }

    void DebugDraw::DrawContactManifold(const Physics::ContactManifold& manifold, float duration) {
        const auto& contacts = manifold.GetContacts();
        
        for (const auto& contact : contacts) {
            // Draw contact position
            glm::vec3 contactPos = (contact.PositionA + contact.PositionB) * 0.5f;
            DrawContact(contactPos, contact.Normal, contact.Penetration, glm::vec3(1, 0, 0), duration);
            
            // Draw position on body A (green)
            DrawCross(contact.PositionA, 0.05f, glm::vec3(0, 1, 0), duration);
            
            // Draw position on body B (blue)
            DrawCross(contact.PositionB, 0.05f, glm::vec3(0, 0, 1), duration);
        }
    }

    void DebugDraw::DrawConstraint(const Physics::Constraint* constraint,
                                  const glm::vec3& color,
                                  float duration) {
        if (!constraint) return;
        
        Physics::RigidBody* bodyA = constraint->GetBodyA();
        Physics::RigidBody* bodyB = constraint->GetBodyB();
        
        if (bodyA && bodyB) {
            // Draw line between connected bodies
            DrawLine(bodyA->GetPosition(), bodyB->GetPosition(), color, duration);
            
            // Draw markers at connection points
            DrawCross(bodyA->GetPosition(), 0.15f, color, duration);
            DrawCross(bodyB->GetPosition(), 0.15f, color, duration);
        }
    }

    void DebugDraw::DrawPhysicsWorld(const Physics::PhysicsWorld* world,
                                    bool drawColliders,
                                    bool drawContacts,
                                    bool drawConstraints,
                                    float duration) {
        if (!world) return;
        
        // Draw colliders
        if (drawColliders) {
            const auto& bodies = world->GetRigidBodies();
            for (const auto& body : bodies) {
                if (body->HasCollisionShape()) {
                    glm::vec3 color;
                    switch (body->GetBodyType()) {
                        case Physics::RigidBody::BodyType::Static:
                            color = glm::vec3(0.5f, 0.5f, 0.5f);  // Gray for static
                            break;
                        case Physics::RigidBody::BodyType::Kinematic:
                            color = glm::vec3(0.0f, 0.8f, 0.8f);  // Cyan for kinematic
                            break;
                        case Physics::RigidBody::BodyType::Dynamic:
                            color = body->IsAwake() ? 
                                glm::vec3(0.0f, 1.0f, 0.0f) :  // Green for awake
                                glm::vec3(0.3f, 0.3f, 0.6f);   // Blue for sleeping
                            break;
                    }
                    
                    DrawCollider(body->GetCollisionShape(), body->GetPosition(), 
                                body->GetRotation(), color, duration);
                }
            }
        }
        
        // Draw contacts
        if (drawContacts) {
            const auto& manifolds = world->GetContactManifolds();
            for (const auto& manifold : manifolds) {
                DrawContactManifold(manifold, duration);
            }
        }
        
        // Draw constraints
        if (drawConstraints) {
            const auto& constraints = world->GetConstraints();
            for (const auto& constraint : constraints) {
                if (constraint->Enabled) {
                    DrawConstraint(constraint.get(), glm::vec3(1, 0.5f, 0), duration);
                } else {
                    DrawConstraint(constraint.get(), glm::vec3(0.5f, 0.5f, 0.5f), duration);
                }
            }
        }
    }

    void DebugDraw::DrawAABB(const glm::vec3& min, const glm::vec3& max,
                            const glm::vec3& color, float duration) {
        glm::vec3 size = max - min;
        glm::vec3 center = (min + max) * 0.5f;
        DrawBox(center, size, glm::quat(1, 0, 0, 0), color, duration);
    }

    void DebugDraw::DrawPlane(const glm::vec3& normal, float distance,
                             float size, const glm::vec3& color, float duration) {
        // Calculate a point on the plane
        glm::vec3 planePoint = normal * distance;
        
        // Calculate tangent vectors
        glm::vec3 tangent1;
        if (std::abs(normal.y) < 0.9f) {
            tangent1 = glm::normalize(glm::cross(normal, glm::vec3(0, 1, 0)));
        } else {
            tangent1 = glm::normalize(glm::cross(normal, glm::vec3(1, 0, 0)));
        }
        glm::vec3 tangent2 = glm::normalize(glm::cross(normal, tangent1));
        
        float halfSize = size * 0.5f;
        
        // Draw a grid on the plane
        int gridDiv = 5;
        float step = size / gridDiv;
        
        for (int i = 0; i <= gridDiv; i++) {
            float offset = -halfSize + i * step;
            
            // Lines along tangent1
            glm::vec3 start1 = planePoint + tangent1 * offset - tangent2 * halfSize;
            glm::vec3 end1 = planePoint + tangent1 * offset + tangent2 * halfSize;
            DrawLine(start1, end1, color, duration);
            
            // Lines along tangent2
            glm::vec3 start2 = planePoint - tangent1 * halfSize + tangent2 * offset;
            glm::vec3 end2 = planePoint + tangent1 * halfSize + tangent2 * offset;
            DrawLine(start2, end2, color, duration);
        }
        
        // Draw normal arrow
        DrawRay(planePoint, normal * 0.5f, glm::vec3(0, 1, 0), duration);
    }

}