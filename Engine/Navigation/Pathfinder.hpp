#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <functional>

namespace GameEngine {

    struct NavNode {
        glm::vec3 Position;
        std::vector<uint32_t> Neighbors;
        float AreaCost = 1.0f;
        bool Walkable = true;
    };

    struct NavPath {
        std::vector<glm::vec3> Waypoints;
        float TotalCost = 0.0f;
        bool IsComplete = false; // false if partial path
    };

    class NavigationMesh {
    public:
        NavigationMesh() = default;

        // Build a grid-based navmesh from scene bounds
        void buildFromGrid(const glm::vec3& min, const glm::vec3& max, float cellSize,
                           std::function<bool(const glm::vec3&)> isWalkable = nullptr);

        // Manual node management
        uint32_t addNode(const glm::vec3& position, float areaCost = 1.0f);
        void connectNodes(uint32_t a, uint32_t b);
        void setNodeWalkable(uint32_t id, bool walkable);

        // Queries
        uint32_t findNearestNode(const glm::vec3& position) const;
        const NavNode* getNode(uint32_t id) const;
        int getNodeCount() const { return static_cast<int>(m_Nodes.size()); }
        const std::unordered_map<uint32_t, NavNode>& getNodes() const { return m_Nodes; }

        // Configuration
        void setAgentRadius(float r) { m_AgentRadius = r; }
        void setAgentHeight(float h) { m_AgentHeight = h; }
        void setMaxSlope(float degrees) { m_MaxSlope = degrees; }
        float getAgentRadius() const { return m_AgentRadius; }

    private:
        std::unordered_map<uint32_t, NavNode> m_Nodes;
        uint32_t m_NextNodeId = 0;
        float m_AgentRadius = 0.5f;
        float m_AgentHeight = 2.0f;
        float m_MaxSlope = 45.0f;
    };

    class Pathfinder {
    public:
        Pathfinder() = default;

        void setNavigationMesh(Ref<NavigationMesh> navMesh) { m_NavMesh = navMesh; }

        // Find path using A*
        NavPath findPath(const glm::vec3& start, const glm::vec3& end) const;

        // Smooth path with Catmull-Rom spline
        static NavPath smoothPath(const NavPath& path, int subdivisions = 4);

        // Max path nodes (to prevent runaway searches)
        void setMaxSearchNodes(int max) { m_MaxSearchNodes = max; }

    private:
        Ref<NavigationMesh> m_NavMesh;
        int m_MaxSearchNodes = 1000;
    };

    class NavigationAgent {
    public:
        enum class Status { Idle, Moving, Reached, Failed };

        NavigationAgent() = default;

        void setPathfinder(Ref<Pathfinder> pathfinder) { m_Pathfinder = pathfinder; }

        // Movement
        void setDestination(const glm::vec3& target);
        void update(float dt);
        void stop();

        // Properties
        void setSpeed(float s) { m_Speed = s; }
        void setAcceleration(float a) { m_Acceleration = a; }
        void setStoppingDistance(float d) { m_StoppingDistance = d; }
        void setTurningSpeed(float s) { m_TurningSpeed = s; }

        // State
        Status getStatus() const { return m_Status; }
        glm::vec3 getPosition() const { return m_Position; }
        void setPosition(const glm::vec3& pos) { m_Position = pos; }
        glm::vec3 getVelocity() const { return m_Velocity; }
        const NavPath& getCurrentPath() const { return m_CurrentPath; }

        // Callbacks
        std::function<void()> OnReachDestination;
        std::function<void()> OnPathFailed;

    private:
        Ref<Pathfinder> m_Pathfinder;
        glm::vec3 m_Position{0.0f};
        glm::vec3 m_Velocity{0.0f};
        glm::vec3 m_Destination{0.0f};
        NavPath m_CurrentPath;
        int m_CurrentWaypointIndex = 0;
        Status m_Status = Status::Idle;

        float m_Speed = 5.0f;
        float m_Acceleration = 10.0f;
        float m_StoppingDistance = 0.5f;
        float m_TurningSpeed = 360.0f;
        float m_CurrentSpeed = 0.0f;
    };

}
