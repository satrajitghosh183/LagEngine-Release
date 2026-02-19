#include "Frustum.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    void Frustum::Update(const glm::mat4& vp) {
        // Extract frustum planes from view-projection matrix (Gribb/Hartmann method)
        // Each plane: Ax + By + Cz + D >= 0 means inside

        // Left
        m_Planes[Left] = glm::vec4(
            vp[0][3] + vp[0][0],
            vp[1][3] + vp[1][0],
            vp[2][3] + vp[2][0],
            vp[3][3] + vp[3][0]);

        // Right
        m_Planes[Right] = glm::vec4(
            vp[0][3] - vp[0][0],
            vp[1][3] - vp[1][0],
            vp[2][3] - vp[2][0],
            vp[3][3] - vp[3][0]);

        // Bottom
        m_Planes[Bottom] = glm::vec4(
            vp[0][3] + vp[0][1],
            vp[1][3] + vp[1][1],
            vp[2][3] + vp[2][1],
            vp[3][3] + vp[3][1]);

        // Top
        m_Planes[Top] = glm::vec4(
            vp[0][3] - vp[0][1],
            vp[1][3] - vp[1][1],
            vp[2][3] - vp[2][1],
            vp[3][3] - vp[3][1]);

        // Near
        m_Planes[Near] = glm::vec4(
            vp[0][3] + vp[0][2],
            vp[1][3] + vp[1][2],
            vp[2][3] + vp[2][2],
            vp[3][3] + vp[3][2]);

        // Far
        m_Planes[Far] = glm::vec4(
            vp[0][3] - vp[0][2],
            vp[1][3] - vp[1][2],
            vp[2][3] - vp[2][2],
            vp[3][3] - vp[3][2]);

        for (int i = 0; i < Count; i++) {
            NormalizePlane(m_Planes[i]);
        }
    }

    void Frustum::NormalizePlane(glm::vec4& plane) {
        float length = glm::length(glm::vec3(plane));
        if (length > 0.0f) {
            plane /= length;
        }
    }

    bool Frustum::IsPointVisible(const glm::vec3& point) const {
        for (int i = 0; i < Count; i++) {
            if (glm::dot(glm::vec3(m_Planes[i]), point) + m_Planes[i].w < 0.0f) {
                return false;
            }
        }
        return true;
    }

    bool Frustum::IsSphereVisible(const glm::vec3& center, float radius) const {
        for (int i = 0; i < Count; i++) {
            float distance = glm::dot(glm::vec3(m_Planes[i]), center) + m_Planes[i].w;
            if (distance < -radius) {
                return false;
            }
        }
        return true;
    }

    bool Frustum::IsAABBVisible(const Mesh3D::AABB& aabb, const glm::mat4& transform) const {
        // Transform AABB corners to world space and find the world-space AABB
        glm::vec3 corners[8] = {
            glm::vec3(aabb.Min.x, aabb.Min.y, aabb.Min.z),
            glm::vec3(aabb.Max.x, aabb.Min.y, aabb.Min.z),
            glm::vec3(aabb.Min.x, aabb.Max.y, aabb.Min.z),
            glm::vec3(aabb.Max.x, aabb.Max.y, aabb.Min.z),
            glm::vec3(aabb.Min.x, aabb.Min.y, aabb.Max.z),
            glm::vec3(aabb.Max.x, aabb.Min.y, aabb.Max.z),
            glm::vec3(aabb.Min.x, aabb.Max.y, aabb.Max.z),
            glm::vec3(aabb.Max.x, aabb.Max.y, aabb.Max.z)
        };

        // Transform all corners
        for (int i = 0; i < 8; i++) {
            glm::vec4 transformed = transform * glm::vec4(corners[i], 1.0f);
            corners[i] = glm::vec3(transformed);
        }

        // For each frustum plane, check if all 8 corners are outside
        for (int p = 0; p < Count; p++) {
            int outsideCount = 0;
            for (int c = 0; c < 8; c++) {
                if (glm::dot(glm::vec3(m_Planes[p]), corners[c]) + m_Planes[p].w < 0.0f) {
                    outsideCount++;
                }
            }
            // All 8 corners are outside this plane -> AABB is completely outside
            if (outsideCount == 8) {
                return false;
            }
        }

        return true;
    }

}
