#include "TransformGizmo.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Scene/Components/TransformComponent.hpp"
#include "../../Engine/Utilities/Debug/DebugDraw.hpp"
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace GameEngine {

    TransformGizmo::TransformGizmo() {
    }

    bool TransformGizmo::Update(
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix,
        const glm::vec2& viewportSize,
        const glm::vec2& mousePos,
        bool mouseDown) {
        
        if (!m_Target.IsValid() || !m_Target.HasComponent<TransformComponent>()) {
            m_IsUsing = false;
            return false;
        }
        
        auto& transform = m_Target.GetComponent<TransformComponent>();
        glm::vec3 position = transform.GetWorldPosition();
        
        // Get ray from mouse position
        glm::vec3 rayOrigin, rayDir;
        ScreenToWorldRay(mousePos, viewMatrix, projMatrix, viewportSize, rayOrigin, rayDir);
        
        if (!mouseDown) {
            // End manipulation
            if (m_IsUsing) {
                m_IsUsing = false;
                m_ActiveAxis = None;
            }
            
            // Hit test for hover
            m_HoveredAxis = HitTest(rayOrigin, rayDir);
            return false;
        }
        
        // Start manipulation
        if (!m_IsUsing && m_HoveredAxis != None) {
            m_IsUsing = true;
            m_ActiveAxis = m_HoveredAxis;
            m_InitialPosition = transform.Position;
            m_InitialRotation = transform.Rotation;
            m_InitialScale = transform.Scale;
            m_InitialMousePos = mousePos;
            
            // Calculate drag start point on the manipulation plane
            float t;
            glm::vec3 planeNormal;
            
            switch (m_ActiveAxis) {
                case X: planeNormal = glm::vec3(0, 1, 0); break;
                case Y: planeNormal = glm::vec3(1, 0, 0); break;
                case Z: planeNormal = glm::vec3(0, 1, 0); break;
                default: planeNormal = -glm::normalize(glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2])); break;
            }
            
            if (HitTestPlane(rayOrigin, rayDir, planeNormal, position, t)) {
                m_DragStartPoint = rayOrigin + rayDir * t;
            } else {
                m_DragStartPoint = position;
            }
        }
        
        // Apply manipulation
        if (m_IsUsing && m_ActiveAxis != None) {
            float t;
            glm::vec3 planeNormal;
            glm::vec3 cameraForward = -glm::normalize(glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]));
            
            switch (m_ActiveAxis) {
                case X: planeNormal = glm::vec3(0, 1, 0); break;
                case Y: planeNormal = glm::vec3(1, 0, 0); break;
                case Z: planeNormal = glm::vec3(0, 1, 0); break;
                case XY: planeNormal = glm::vec3(0, 0, 1); break;
                case XZ: planeNormal = glm::vec3(0, 1, 0); break;
                case YZ: planeNormal = glm::vec3(1, 0, 0); break;
                default: planeNormal = cameraForward; break;
            }
            
            if (HitTestPlane(rayOrigin, rayDir, planeNormal, m_InitialPosition, t)) {
                glm::vec3 currentPoint = rayOrigin + rayDir * t;
                glm::vec3 delta = currentPoint - m_DragStartPoint;
                
                // Constrain to axis for single-axis operations
                switch (m_ActiveAxis) {
                    case X: delta = glm::vec3(delta.x, 0, 0); break;
                    case Y: delta = glm::vec3(0, delta.y, 0); break;
                    case Z: delta = glm::vec3(0, 0, delta.z); break;
                    case XY: delta.z = 0; break;
                    case XZ: delta.y = 0; break;
                    case YZ: delta.x = 0; break;
                    default: break;
                }
                
                switch (m_Type) {
                    case GizmoType::Translate:
                        transform.Position = m_InitialPosition + delta;
                        transform.SetDirty();  // Mark transform cache as dirty
                        break;
                    case GizmoType::Rotate: {
                        // Simplified rotation - rotate around the active axis
                        float angle = glm::length(delta) * 2.0f; // Scale factor
                        glm::vec3 axis;
                        switch (m_ActiveAxis) {
                            case X: axis = glm::vec3(1, 0, 0); angle = delta.z; break;
                            case Y: axis = glm::vec3(0, 1, 0); angle = delta.x; break;
                            case Z: axis = glm::vec3(0, 0, 1); angle = -delta.x; break;
                            default: axis = glm::vec3(0, 1, 0); break;
                        }
                        transform.Rotation = glm::angleAxis(angle, axis) * m_InitialRotation;
                        transform.SetDirty();  // Mark transform cache as dirty
                        break;
                    }
                    case GizmoType::Scale: {
                        // Use screen-space mouse delta for more intuitive scaling
                        glm::vec2 mouseDelta = mousePos - m_InitialMousePos;
                        float screenDelta = (mouseDelta.x + mouseDelta.y) * 0.01f; // Combine X and Y movement
                        
                        glm::vec3 scaleMultiplier(1.0f);
                        switch (m_ActiveAxis) {
                            case X: scaleMultiplier.x = 1.0f + mouseDelta.x * 0.01f; break;
                            case Y: scaleMultiplier.y = 1.0f - mouseDelta.y * 0.01f; break; // Invert Y for natural feel
                            case Z: scaleMultiplier.z = 1.0f + mouseDelta.x * 0.01f; break;
                            case XYZ: scaleMultiplier = glm::vec3(1.0f + screenDelta); break;
                            default: {
                                // For plane scaling (XY, XZ, YZ)
                                float factor = 1.0f + screenDelta;
                                if (m_ActiveAxis == XY) { scaleMultiplier.x = factor; scaleMultiplier.y = factor; }
                                else if (m_ActiveAxis == XZ) { scaleMultiplier.x = factor; scaleMultiplier.z = factor; }
                                else if (m_ActiveAxis == YZ) { scaleMultiplier.y = factor; scaleMultiplier.z = factor; }
                                break;
                            }
                        }
                        transform.Scale = m_InitialScale * scaleMultiplier;
                        transform.Scale = glm::max(transform.Scale, glm::vec3(0.001f)); // Prevent zero/negative scale
                        transform.SetDirty();  // Mark transform cache as dirty
                        break;
                    }
                }
            }
            
            return true;
        }
        
        return false;
    }

    void TransformGizmo::Draw(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
        if (!m_Target.IsValid() || !m_Target.HasComponent<TransformComponent>()) {
            return;
        }
        
        auto& transform = m_Target.GetComponent<TransformComponent>();
        glm::vec3 position = transform.GetWorldPosition();
        glm::quat rotation = (m_Mode == GizmoMode::Local) ? transform.GetWorldRotation() : glm::quat(1, 0, 0, 0);
        
        switch (m_Type) {
            case GizmoType::Translate:
                DrawTranslateGizmo(position, rotation);
                break;
            case GizmoType::Rotate:
                DrawRotateGizmo(position, rotation);
                break;
            case GizmoType::Scale:
                DrawScaleGizmo(position, rotation);
                break;
        }
    }

    TransformGizmo::Axis TransformGizmo::HitTest(const glm::vec3& rayOrigin, const glm::vec3& rayDir) {
        if (!m_Target.IsValid() || !m_Target.HasComponent<TransformComponent>()) {
            return None;
        }
        
        auto& transform = m_Target.GetComponent<TransformComponent>();
        glm::vec3 position = transform.GetWorldPosition();
        
        float hitThreshold = 0.2f * m_Size;
        float closestT = std::numeric_limits<float>::max();
        Axis closestAxis = None;
        
        float t;
        
        // Test center cube for uniform scale (check first as it's smaller and more precise)
        if (m_Type == GizmoType::Scale) {
            float cubeHalfSize = 0.04f * m_Size;
            glm::vec3 toCenter = position - rayOrigin;
            float tCenter = glm::dot(toCenter, rayDir);
            if (tCenter > 0) {
                glm::vec3 closestPoint = rayOrigin + rayDir * tCenter;
                float distToCenter = glm::length(closestPoint - position);
                if (distToCenter < cubeHalfSize * 2.0f && tCenter < closestT) {
                    closestT = tCenter;
                    closestAxis = XYZ;
                }
            }
        }
        
        // Test X axis
        if (HitTestAxis(rayOrigin, rayDir, glm::vec3(1, 0, 0) * m_Size + position, t)) {
            if (t < closestT) {
                closestT = t;
                closestAxis = X;
            }
        }
        
        // Test Y axis
        if (HitTestAxis(rayOrigin, rayDir, glm::vec3(0, 1, 0) * m_Size + position, t)) {
            if (t < closestT) {
                closestT = t;
                closestAxis = Y;
            }
        }
        
        // Test Z axis
        if (HitTestAxis(rayOrigin, rayDir, glm::vec3(0, 0, 1) * m_Size + position, t)) {
            if (t < closestT) {
                closestT = t;
                closestAxis = Z;
            }
        }
        
        return closestAxis;
    }

    bool TransformGizmo::HitTestAxis(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                     const glm::vec3& axisEnd, float& t) {
        // Simple distance check from ray to axis line
        if (!m_Target.IsValid() || !m_Target.HasComponent<TransformComponent>()) {
            return false;
        }
        
        auto& transform = m_Target.GetComponent<TransformComponent>();
        glm::vec3 position = transform.GetWorldPosition();
        
        glm::vec3 axisDir = glm::normalize(axisEnd - position);
        
        // Calculate closest points on both lines
        glm::vec3 w0 = rayOrigin - position;
        float a = glm::dot(rayDir, rayDir);
        float b = glm::dot(rayDir, axisDir);
        float c = glm::dot(axisDir, axisDir);
        float d = glm::dot(rayDir, w0);
        float e = glm::dot(axisDir, w0);
        
        float denom = a * c - b * b;
        if (std::abs(denom) < 0.0001f) return false;
        
        float s = (b * e - c * d) / denom;
        float tt = (a * e - b * d) / denom;
        
        // Check if intersection is within axis length (with small tolerance past tip)
        if (tt < -0.1f * m_Size || tt > m_Size * 1.15f) return false;
        if (s < 0) return false;
        
        // Calculate distance between closest points
        glm::vec3 p1 = rayOrigin + rayDir * s;
        glm::vec3 p2 = position + axisDir * tt;
        float dist = glm::length(p1 - p2);
        
        if (dist < 0.2f * m_Size) {
            t = s;
            return true;
        }
        
        return false;
    }

    bool TransformGizmo::HitTestPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                                      const glm::vec3& planeNormal, const glm::vec3& planePoint, float& t) {
        float denom = glm::dot(planeNormal, rayDir);
        if (std::abs(denom) < 0.0001f) return false;
        
        t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
        return t >= 0;
    }

    void TransformGizmo::ScreenToWorldRay(const glm::vec2& screenPos, const glm::mat4& viewMatrix,
                                          const glm::mat4& projMatrix, const glm::vec2& viewportSize,
                                          glm::vec3& outOrigin, glm::vec3& outDirection) {
        // Convert screen coordinates to NDC
        float x = (2.0f * screenPos.x) / viewportSize.x - 1.0f;
        float y = 1.0f - (2.0f * screenPos.y) / viewportSize.y;
        
        glm::mat4 invViewProj = glm::inverse(projMatrix * viewMatrix);
        
        glm::vec4 nearPoint = invViewProj * glm::vec4(x, y, -1.0f, 1.0f);
        glm::vec4 farPoint = invViewProj * glm::vec4(x, y, 1.0f, 1.0f);
        
        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;
        
        outOrigin = glm::vec3(nearPoint);
        outDirection = glm::normalize(glm::vec3(farPoint - nearPoint));
    }

    void TransformGizmo::DrawTranslateGizmo(const glm::vec3& position, const glm::quat& rotation) {
        // Draw axes as arrows
        glm::vec3 xColor = (m_HoveredAxis == X || m_ActiveAxis == X) ? HOVER_COLOR : X_AXIS_COLOR;
        glm::vec3 yColor = (m_HoveredAxis == Y || m_ActiveAxis == Y) ? HOVER_COLOR : Y_AXIS_COLOR;
        glm::vec3 zColor = (m_HoveredAxis == Z || m_ActiveAxis == Z) ? HOVER_COLOR : Z_AXIS_COLOR;
        
        glm::vec3 xAxis = rotation * glm::vec3(1, 0, 0);
        glm::vec3 yAxis = rotation * glm::vec3(0, 1, 0);
        glm::vec3 zAxis = rotation * glm::vec3(0, 0, 1);
        
        DebugDraw::DrawLine(position, position + xAxis * m_Size, xColor, 0.0f);
        DebugDraw::DrawLine(position, position + yAxis * m_Size, yColor, 0.0f);
        DebugDraw::DrawLine(position, position + zAxis * m_Size, zColor, 0.0f);
        
        // Draw arrow heads (cones would be better, but use lines for simplicity)
        float arrowSize = 0.1f * m_Size;
        glm::vec3 xEnd = position + xAxis * m_Size;
        glm::vec3 yEnd = position + yAxis * m_Size;
        glm::vec3 zEnd = position + zAxis * m_Size;
        
        // X arrow
        DebugDraw::DrawLine(xEnd, xEnd - xAxis * arrowSize + yAxis * arrowSize * 0.3f, xColor, 0.0f);
        DebugDraw::DrawLine(xEnd, xEnd - xAxis * arrowSize - yAxis * arrowSize * 0.3f, xColor, 0.0f);
        
        // Y arrow
        DebugDraw::DrawLine(yEnd, yEnd - yAxis * arrowSize + xAxis * arrowSize * 0.3f, yColor, 0.0f);
        DebugDraw::DrawLine(yEnd, yEnd - yAxis * arrowSize - xAxis * arrowSize * 0.3f, yColor, 0.0f);
        
        // Z arrow
        DebugDraw::DrawLine(zEnd, zEnd - zAxis * arrowSize + yAxis * arrowSize * 0.3f, zColor, 0.0f);
        DebugDraw::DrawLine(zEnd, zEnd - zAxis * arrowSize - yAxis * arrowSize * 0.3f, zColor, 0.0f);
    }

    void TransformGizmo::DrawRotateGizmo(const glm::vec3& position, const glm::quat& rotation) {
        glm::vec3 xColor = (m_HoveredAxis == X || m_ActiveAxis == X) ? HOVER_COLOR : X_AXIS_COLOR;
        glm::vec3 yColor = (m_HoveredAxis == Y || m_ActiveAxis == Y) ? HOVER_COLOR : Y_AXIS_COLOR;
        glm::vec3 zColor = (m_HoveredAxis == Z || m_ActiveAxis == Z) ? HOVER_COLOR : Z_AXIS_COLOR;
        
        // Draw circles for rotation
        int segments = 32;
        float radius = m_Size * 0.8f;
        
        // X axis circle (YZ plane)
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * glm::two_pi<float>();
            float a2 = (float)(i + 1) / segments * glm::two_pi<float>();
            
            glm::vec3 p1 = position + rotation * glm::vec3(0, cos(a1), sin(a1)) * radius;
            glm::vec3 p2 = position + rotation * glm::vec3(0, cos(a2), sin(a2)) * radius;
            DebugDraw::DrawLine(p1, p2, xColor, 0.0f);
        }
        
        // Y axis circle (XZ plane)
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * glm::two_pi<float>();
            float a2 = (float)(i + 1) / segments * glm::two_pi<float>();
            
            glm::vec3 p1 = position + rotation * glm::vec3(cos(a1), 0, sin(a1)) * radius;
            glm::vec3 p2 = position + rotation * glm::vec3(cos(a2), 0, sin(a2)) * radius;
            DebugDraw::DrawLine(p1, p2, yColor, 0.0f);
        }
        
        // Z axis circle (XY plane)
        for (int i = 0; i < segments; i++) {
            float a1 = (float)i / segments * glm::two_pi<float>();
            float a2 = (float)(i + 1) / segments * glm::two_pi<float>();
            
            glm::vec3 p1 = position + rotation * glm::vec3(cos(a1), sin(a1), 0) * radius;
            glm::vec3 p2 = position + rotation * glm::vec3(cos(a2), sin(a2), 0) * radius;
            DebugDraw::DrawLine(p1, p2, zColor, 0.0f);
        }
    }

    void TransformGizmo::DrawScaleGizmo(const glm::vec3& position, const glm::quat& rotation) {
        glm::vec3 xColor = (m_HoveredAxis == X || m_ActiveAxis == X) ? HOVER_COLOR : X_AXIS_COLOR;
        glm::vec3 yColor = (m_HoveredAxis == Y || m_ActiveAxis == Y) ? HOVER_COLOR : Y_AXIS_COLOR;
        glm::vec3 zColor = (m_HoveredAxis == Z || m_ActiveAxis == Z) ? HOVER_COLOR : Z_AXIS_COLOR;
        
        glm::vec3 xAxis = rotation * glm::vec3(1, 0, 0);
        glm::vec3 yAxis = rotation * glm::vec3(0, 1, 0);
        glm::vec3 zAxis = rotation * glm::vec3(0, 0, 1);
        
        // Draw axes
        DebugDraw::DrawLine(position, position + xAxis * m_Size, xColor, 0.0f);
        DebugDraw::DrawLine(position, position + yAxis * m_Size, yColor, 0.0f);
        DebugDraw::DrawLine(position, position + zAxis * m_Size, zColor, 0.0f);
        
        // Draw cubes at the end
        float cubeSize = 0.1f * m_Size;
        glm::vec3 xEnd = position + xAxis * m_Size;
        glm::vec3 yEnd = position + yAxis * m_Size;
        glm::vec3 zEnd = position + zAxis * m_Size;
        
        // Simple cube representation with lines
        auto drawCube = [](const glm::vec3& center, float size, const glm::vec3& color) {
            float h = size * 0.5f;
            glm::vec3 corners[8] = {
                center + glm::vec3(-h, -h, -h),
                center + glm::vec3( h, -h, -h),
                center + glm::vec3( h,  h, -h),
                center + glm::vec3(-h,  h, -h),
                center + glm::vec3(-h, -h,  h),
                center + glm::vec3( h, -h,  h),
                center + glm::vec3( h,  h,  h),
                center + glm::vec3(-h,  h,  h)
            };
            
            // Bottom face
            DebugDraw::DrawLine(corners[0], corners[1], color, 0.0f);
            DebugDraw::DrawLine(corners[1], corners[2], color, 0.0f);
            DebugDraw::DrawLine(corners[2], corners[3], color, 0.0f);
            DebugDraw::DrawLine(corners[3], corners[0], color, 0.0f);
            // Top face
            DebugDraw::DrawLine(corners[4], corners[5], color, 0.0f);
            DebugDraw::DrawLine(corners[5], corners[6], color, 0.0f);
            DebugDraw::DrawLine(corners[6], corners[7], color, 0.0f);
            DebugDraw::DrawLine(corners[7], corners[4], color, 0.0f);
            // Vertical edges
            DebugDraw::DrawLine(corners[0], corners[4], color, 0.0f);
            DebugDraw::DrawLine(corners[1], corners[5], color, 0.0f);
            DebugDraw::DrawLine(corners[2], corners[6], color, 0.0f);
            DebugDraw::DrawLine(corners[3], corners[7], color, 0.0f);
        };
        
        drawCube(xEnd, cubeSize, xColor);
        drawCube(yEnd, cubeSize, yColor);
        drawCube(zEnd, cubeSize, zColor);
        
        // Center cube for uniform scale
        glm::vec3 centerColor = (m_HoveredAxis == XYZ || m_ActiveAxis == XYZ) ? HOVER_COLOR : glm::vec3(0.8f);
        drawCube(position, cubeSize * 0.8f, centerColor);
    }

}
