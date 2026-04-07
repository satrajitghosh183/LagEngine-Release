#include "Pathfinder.hpp"
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <limits>

namespace GameEngine {

    // =====================================================================
    // NavigationMesh
    // =====================================================================

    void NavigationMesh::buildFromGrid(const glm::vec3& min, const glm::vec3& max, float cellSize,
                                       std::function<bool(const glm::vec3&)> isWalkable) {
        m_Nodes.clear();
        m_NextNodeId = 0;

        int countX = static_cast<int>(std::ceil((max.x - min.x) / cellSize));
        int countZ = static_cast<int>(std::ceil((max.z - min.z) / cellSize));

        // Create grid of nodes
        std::vector<std::vector<uint32_t>> nodeGrid(countZ, std::vector<uint32_t>(countX, UINT32_MAX));

        for (int z = 0; z < countZ; z++) {
            for (int x = 0; x < countX; x++) {
                glm::vec3 pos(
                    min.x + (x + 0.5f) * cellSize,
                    min.y,
                    min.z + (z + 0.5f) * cellSize
                );

                bool walkable = true;
                if (isWalkable) {
                    walkable = isWalkable(pos);
                }

                uint32_t id = addNode(pos);
                setNodeWalkable(id, walkable);
                nodeGrid[z][x] = id;
            }
        }

        // Connect neighbors (8-directional)
        for (int z = 0; z < countZ; z++) {
            for (int x = 0; x < countX; x++) {
                uint32_t id = nodeGrid[z][x];
                if (!m_Nodes[id].Walkable) continue;

                int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
                int dz[] = {-1, -1, -1, 0, 0, 1, 1, 1};

                for (int d = 0; d < 8; d++) {
                    int nx = x + dx[d];
                    int nz = z + dz[d];
                    if (nx < 0 || nx >= countX || nz < 0 || nz >= countZ) continue;

                    uint32_t neighborId = nodeGrid[nz][nx];
                    if (m_Nodes[neighborId].Walkable) {
                        connectNodes(id, neighborId);
                    }
                }
            }
        }
    }

    uint32_t NavigationMesh::addNode(const glm::vec3& position, float areaCost) {
        uint32_t id = m_NextNodeId++;
        NavNode node;
        node.Position = position;
        node.AreaCost = areaCost;
        m_Nodes[id] = node;
        return id;
    }

    void NavigationMesh::connectNodes(uint32_t a, uint32_t b) {
        auto itA = m_Nodes.find(a);
        auto itB = m_Nodes.find(b);
        if (itA == m_Nodes.end() || itB == m_Nodes.end()) return;

        // Avoid duplicate connections
        auto& neighborsA = itA->second.Neighbors;
        if (std::find(neighborsA.begin(), neighborsA.end(), b) == neighborsA.end()) {
            neighborsA.push_back(b);
        }
        auto& neighborsB = itB->second.Neighbors;
        if (std::find(neighborsB.begin(), neighborsB.end(), a) == neighborsB.end()) {
            neighborsB.push_back(a);
        }
    }

    void NavigationMesh::setNodeWalkable(uint32_t id, bool walkable) {
        auto it = m_Nodes.find(id);
        if (it != m_Nodes.end()) {
            it->second.Walkable = walkable;
        }
    }

    uint32_t NavigationMesh::findNearestNode(const glm::vec3& position) const {
        float bestDist = std::numeric_limits<float>::max();
        uint32_t bestId = UINT32_MAX;

        for (const auto& [id, node] : m_Nodes) {
            if (!node.Walkable) continue;
            float dist = glm::distance(position, node.Position);
            if (dist < bestDist) {
                bestDist = dist;
                bestId = id;
            }
        }
        return bestId;
    }

    const NavNode* NavigationMesh::getNode(uint32_t id) const {
        auto it = m_Nodes.find(id);
        return (it != m_Nodes.end()) ? &it->second : nullptr;
    }

    // =====================================================================
    // Pathfinder (A*)
    // =====================================================================

    NavPath Pathfinder::findPath(const glm::vec3& start, const glm::vec3& end) const {
        NavPath result;
        if (!m_NavMesh) { return result; }

        uint32_t startNode = m_NavMesh->findNearestNode(start);
        uint32_t endNode = m_NavMesh->findNearestNode(end);

        if (startNode == UINT32_MAX || endNode == UINT32_MAX) {
            return result;
        }

        if (startNode == endNode) {
            result.Waypoints.push_back(start);
            result.Waypoints.push_back(end);
            result.IsComplete = true;
            return result;
        }

        // A* search
        struct AStarNode {
            uint32_t id;
            float gCost; // cost from start
            float fCost; // gCost + heuristic
            bool operator>(const AStarNode& other) const { return fCost > other.fCost; }
        };

        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> openSet;
        std::unordered_map<uint32_t, float> gCosts;
        std::unordered_map<uint32_t, uint32_t> cameFrom;
        std::unordered_set<uint32_t> closedSet;

        const NavNode* endNodeData = m_NavMesh->getNode(endNode);
        if (!endNodeData) return result;

        gCosts[startNode] = 0.0f;
        float heuristic = glm::distance(m_NavMesh->getNode(startNode)->Position, endNodeData->Position);
        openSet.push({startNode, 0.0f, heuristic});

        int nodesSearched = 0;
        uint32_t closestToEnd = startNode;
        float closestDist = heuristic;

        while (!openSet.empty() && nodesSearched < m_MaxSearchNodes) {
            AStarNode current = openSet.top();
            openSet.pop();
            nodesSearched++;

            if (current.id == endNode) {
                // Reconstruct path
                std::vector<glm::vec3> path;
                uint32_t node = endNode;
                while (cameFrom.find(node) != cameFrom.end()) {
                    path.push_back(m_NavMesh->getNode(node)->Position);
                    node = cameFrom[node];
                }
                path.push_back(m_NavMesh->getNode(startNode)->Position);
                std::reverse(path.begin(), path.end());

                // Replace first/last with actual start/end positions
                if (!path.empty()) path.front() = start;
                path.push_back(end);

                result.Waypoints = std::move(path);
                result.TotalCost = gCosts[endNode];
                result.IsComplete = true;
                return result;
            }

            if (closedSet.count(current.id)) continue;
            closedSet.insert(current.id);

            const NavNode* currentNode = m_NavMesh->getNode(current.id);
            if (!currentNode) continue;

            // Track closest node to end for partial paths
            float distToEnd = glm::distance(currentNode->Position, endNodeData->Position);
            if (distToEnd < closestDist) {
                closestDist = distToEnd;
                closestToEnd = current.id;
            }

            for (uint32_t neighborId : currentNode->Neighbors) {
                if (closedSet.count(neighborId)) continue;

                const NavNode* neighbor = m_NavMesh->getNode(neighborId);
                if (!neighbor || !neighbor->Walkable) continue;

                float moveCost = glm::distance(currentNode->Position, neighbor->Position) * neighbor->AreaCost;
                float newGCost = gCosts[current.id] + moveCost;

                if (gCosts.find(neighborId) == gCosts.end() || newGCost < gCosts[neighborId]) {
                    gCosts[neighborId] = newGCost;
                    cameFrom[neighborId] = current.id;
                    float h = glm::distance(neighbor->Position, endNodeData->Position);
                    openSet.push({neighborId, newGCost, newGCost + h});
                }
            }
        }

        // Partial path to closest reachable node
        if (closestToEnd != startNode) {
            std::vector<glm::vec3> path;
            uint32_t node = closestToEnd;
            while (cameFrom.find(node) != cameFrom.end()) {
                path.push_back(m_NavMesh->getNode(node)->Position);
                node = cameFrom[node];
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            result.Waypoints = std::move(path);
            result.TotalCost = gCosts[closestToEnd];
            result.IsComplete = false;
        }

        return result;
    }

    NavPath Pathfinder::smoothPath(const NavPath& path, int subdivisions) {
        if (path.Waypoints.size() < 3) return path;

        NavPath smoothed;
        smoothed.IsComplete = path.IsComplete;
        smoothed.TotalCost = path.TotalCost;

        const auto& pts = path.Waypoints;
        smoothed.Waypoints.push_back(pts.front());

        for (size_t i = 0; i < pts.size() - 1; i++) {
            glm::vec3 p0 = pts[std::max(static_cast<int>(i) - 1, 0)];
            glm::vec3 p1 = pts[i];
            glm::vec3 p2 = pts[std::min(i + 1, pts.size() - 1)];
            glm::vec3 p3 = pts[std::min(i + 2, pts.size() - 1)];

            for (int s = 1; s <= subdivisions; s++) {
                float t = static_cast<float>(s) / subdivisions;

                // Catmull-Rom interpolation
                float t2 = t * t;
                float t3 = t2 * t;

                glm::vec3 point = 0.5f * (
                    (2.0f * p1) +
                    (-p0 + p2) * t +
                    (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                    (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
                );

                smoothed.Waypoints.push_back(point);
            }
        }

        return smoothed;
    }

    // =====================================================================
    // NavigationAgent
    // =====================================================================

    void NavigationAgent::setDestination(const glm::vec3& target) {
        m_Destination = target;
        if (m_Pathfinder) {
            m_CurrentPath = m_Pathfinder->findPath(m_Position, target);
            if (!m_CurrentPath.Waypoints.empty()) {
                m_Status = Status::Moving;
                m_CurrentWaypointIndex = 0;
            } else {
                m_Status = Status::Failed;
                if (OnPathFailed) OnPathFailed();
            }
        }
    }

    void NavigationAgent::update(float dt) {
        if (m_Status != Status::Moving) return;
        if (m_CurrentWaypointIndex >= static_cast<int>(m_CurrentPath.Waypoints.size())) {
            m_Status = Status::Reached;
            m_CurrentSpeed = 0.0f;
            m_Velocity = glm::vec3(0.0f);
            if (OnReachDestination) OnReachDestination();
            return;
        }

        glm::vec3 target = m_CurrentPath.Waypoints[m_CurrentWaypointIndex];
        glm::vec3 toTarget = target - m_Position;
        float dist = glm::length(toTarget);

        // Check if we reached current waypoint
        if (dist < m_StoppingDistance) {
            m_CurrentWaypointIndex++;
            if (m_CurrentWaypointIndex >= static_cast<int>(m_CurrentPath.Waypoints.size())) {
                m_Status = Status::Reached;
                m_CurrentSpeed = 0.0f;
                m_Velocity = glm::vec3(0.0f);
                if (OnReachDestination) OnReachDestination();
                return;
            }
            return;
        }

        // Accelerate toward target
        glm::vec3 direction = toTarget / dist;
        m_CurrentSpeed = glm::min(m_CurrentSpeed + m_Acceleration * dt, m_Speed);

        // Slow down near destination
        float remainingDist = 0.0f;
        for (int i = m_CurrentWaypointIndex; i < static_cast<int>(m_CurrentPath.Waypoints.size()) - 1; i++) {
            remainingDist += glm::distance(m_CurrentPath.Waypoints[i], m_CurrentPath.Waypoints[i + 1]);
        }
        remainingDist += dist;

        float slowdownDist = m_Speed * m_Speed / (2.0f * m_Acceleration);
        if (remainingDist < slowdownDist) {
            m_CurrentSpeed = glm::max(m_Speed * (remainingDist / slowdownDist), 0.5f);
        }

        m_Velocity = direction * m_CurrentSpeed;
        m_Position += m_Velocity * dt;
    }

    void NavigationAgent::stop() {
        m_Status = Status::Idle;
        m_CurrentSpeed = 0.0f;
        m_Velocity = glm::vec3(0.0f);
        m_CurrentPath.Waypoints.clear();
    }

}
