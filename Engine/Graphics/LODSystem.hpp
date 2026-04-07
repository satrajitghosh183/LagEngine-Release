#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <glad/glad.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace GameEngine {

    struct LODLevel {
        uint32_t MeshID = 0;
        std::string MeshPath;
        float ScreenSizeThreshold = 1.0f; // Fraction of screen height (0-1)
        float Distance = 0.0f;            // Distance-based threshold
    };

    struct LODStatistics {
        int TotalObjects = 0;
        int ObjectsByLOD[4] = {};
        int CulledObjects = 0;
    };

    class LODGroup {
    public:
        void addLevel(const LODLevel& level) {
            m_Levels.push_back(level);
        }

        int selectLevel(const glm::vec3& cameraPos, const glm::vec3& objectPos,
                         float boundingRadius, float screenHeight) const {
            float dist = glm::length(cameraPos - objectPos);

            for (int i = 0; i < static_cast<int>(m_Levels.size()); i++) {
                float threshold = m_Levels[i].Distance;
                float margin = threshold * m_HysteresisMargin;

                if (i == m_CurrentLevel) {
                    // Hysteresis: require exceeding threshold + margin to switch away
                    if (dist < threshold + margin) return i;
                } else {
                    if (dist < threshold) return i;
                }
            }

            return static_cast<int>(m_Levels.size()) - 1;
        }

        int getCurrentLevel() const { return m_CurrentLevel; }
        void setCurrentLevel(int level) { m_CurrentLevel = level; }
        const std::vector<LODLevel>& getLevels() const { return m_Levels; }
        void setHysteresisMargin(float margin) { m_HysteresisMargin = margin; }

    private:
        std::vector<LODLevel> m_Levels;
        mutable int m_CurrentLevel = 0;
        float m_HysteresisMargin = 0.1f;
    };

    class LODSystem {
    public:
        void registerGroup(uint32_t entityID, const LODGroup& group) {
            m_Groups[entityID] = group;
        }

        void unregisterGroup(uint32_t entityID) {
            m_Groups.erase(entityID);
        }

        void update(const glm::vec3& cameraPos, float screenHeight) {
            m_Stats = {};
            m_Stats.TotalObjects = static_cast<int>(m_Groups.size());

            for (auto& [id, group] : m_Groups) {
                auto it = m_ObjectPositions.find(id);
                if (it == m_ObjectPositions.end()) continue;

                int level = group.selectLevel(cameraPos, it->second, 1.0f, screenHeight);
                group.setCurrentLevel(level);

                if (level >= 0 && level < 4) {
                    m_Stats.ObjectsByLOD[level]++;
                }
            }
        }

        void setObjectPosition(uint32_t entityID, const glm::vec3& pos) {
            m_ObjectPositions[entityID] = pos;
        }

        int getSelectedLevel(uint32_t entityID) const {
            auto it = m_Groups.find(entityID);
            return it != m_Groups.end() ? it->second.getCurrentLevel() : 0;
        }

        const LODStatistics& getStatistics() const { return m_Stats; }

    private:
        std::unordered_map<uint32_t, LODGroup> m_Groups;
        std::unordered_map<uint32_t, glm::vec3> m_ObjectPositions;
        LODStatistics m_Stats;
    };

    // =====================================================================
    // Occlusion Culling (OpenGL occlusion queries)
    // =====================================================================

    class OcclusionQuery {
    public:
        OcclusionQuery() { glGenQueries(1, &m_QueryID); }
        ~OcclusionQuery() { if (m_QueryID) glDeleteQueries(1, &m_QueryID); }

        OcclusionQuery(const OcclusionQuery&) = delete;
        OcclusionQuery& operator=(const OcclusionQuery&) = delete;

        void begin() {
            glBeginQuery(GL_ANY_SAMPLES_PASSED, m_QueryID);
            m_Active = true;
        }

        void end() {
            glEndQuery(GL_ANY_SAMPLES_PASSED);
            m_Active = false;
            m_HasResult = false;
        }

        bool isResultAvailable() const {
            if (m_Active) return false;
            GLuint available = 0;
            glGetQueryObjectuiv(m_QueryID, GL_QUERY_RESULT_AVAILABLE, &available);
            return available == GL_TRUE;
        }

        bool getResult() {
            GLuint result = 0;
            glGetQueryObjectuiv(m_QueryID, GL_QUERY_RESULT, &result);
            m_LastResult = result > 0;
            m_HasResult = true;
            return m_LastResult;
        }

        bool getLastResult() const { return m_LastResult; }
        GLuint getQueryID() const { return m_QueryID; }

    private:
        GLuint m_QueryID = 0;
        bool m_Active = false;
        bool m_HasResult = false;
        bool m_LastResult = true;
    };

    struct AABB {
        glm::vec3 Min = glm::vec3(0.0f);
        glm::vec3 Max = glm::vec3(0.0f);

        glm::vec3 center() const { return (Min + Max) * 0.5f; }
        glm::vec3 extent() const { return (Max - Min) * 0.5f; }
    };

    class OcclusionCullingSystem {
    public:
        OcclusionCullingSystem() = default;
        ~OcclusionCullingSystem() = default;

        void initialize(int maxObjects);
        void beginFrame();
        void testObject(uint32_t entityID, const AABB& bounds);
        void endFrame();

        bool isVisible(uint32_t entityID) const;

        struct Stats {
            int TotalTested = 0;
            int TotalVisible = 0;
            int TotalOccluded = 0;
        };
        const Stats& getStatistics() const { return m_Stats; }

    private:
        void renderAABB(const AABB& bounds);

        std::unordered_map<uint32_t, Scope<OcclusionQuery>> m_Queries;
        std::unordered_map<uint32_t, bool> m_VisibilityCache; // Previous frame results
        Stats m_Stats;
        GLuint m_BoxVAO = 0, m_BoxVBO = 0, m_BoxIBO = 0;
        bool m_Initialized = false;
    };

} // namespace GameEngine
