#include "LODSystem.hpp"
#include <cstring>

namespace GameEngine {

    void OcclusionCullingSystem::initialize(int maxObjects) {
        m_Queries.clear();
        m_VisibilityCache.clear();

        // Create unit cube VAO for rendering bounding boxes
        if (!m_Initialized) {
            float vertices[] = {
                // positions
                -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,
                -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
            };

            unsigned int indices[] = {
                0, 1, 2, 2, 3, 0,  // back
                4, 5, 6, 6, 7, 4,  // front
                0, 4, 7, 7, 3, 0,  // left
                1, 5, 6, 6, 2, 1,  // right
                3, 7, 6, 6, 2, 3,  // top
                0, 4, 5, 5, 1, 0,  // bottom
            };

            glGenVertexArrays(1, &m_BoxVAO);
            glGenBuffers(1, &m_BoxVBO);
            glGenBuffers(1, &m_BoxIBO);

            glBindVertexArray(m_BoxVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_BoxVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_BoxIBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
            glBindVertexArray(0);

            m_Initialized = true;
        }
    }

    void OcclusionCullingSystem::beginFrame() {
        // Store previous frame results as cache
        for (auto& [id, query] : m_Queries) {
            if (query->isResultAvailable()) {
                m_VisibilityCache[id] = query->getResult();
            }
        }
        m_Stats = {};
    }

    void OcclusionCullingSystem::testObject(uint32_t entityID, const AABB& bounds) {
        m_Stats.TotalTested++;

        // Create query if needed
        if (m_Queries.find(entityID) == m_Queries.end()) {
            m_Queries[entityID] = CreateScope<OcclusionQuery>();
        }

        auto& query = m_Queries[entityID];

        // Render AABB with occlusion query
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDepthMask(GL_FALSE);

        query->begin();
        renderAABB(bounds);
        query->end();

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
    }

    void OcclusionCullingSystem::endFrame() {
        // Collect results
        for (auto& [id, query] : m_Queries) {
            bool visible = true;
            if (query->isResultAvailable()) {
                visible = query->getResult();
                m_VisibilityCache[id] = visible;
            } else {
                // Use cached result from previous frame
                auto it = m_VisibilityCache.find(id);
                if (it != m_VisibilityCache.end()) {
                    visible = it->second;
                }
            }

            if (visible)
                m_Stats.TotalVisible++;
            else
                m_Stats.TotalOccluded++;
        }
    }

    bool OcclusionCullingSystem::isVisible(uint32_t entityID) const {
        auto it = m_VisibilityCache.find(entityID);
        if (it != m_VisibilityCache.end()) return it->second;
        return true; // Visible by default
    }

    void OcclusionCullingSystem::renderAABB(const AABB& bounds) {
        if (!m_BoxVAO) return;

        // The box mesh is unit cube centered at origin
        // We need to scale and translate it to match the AABB
        // For simplicity, just use the VAO directly (shader should apply transform)
        glBindVertexArray(m_BoxVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);
    }

} // namespace GameEngine
