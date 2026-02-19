#include "TearableCloth.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {
namespace Physics {

    void TearableCloth::Initialize(float width, float height, int resX, int resY) {
        m_Width = width;
        m_Height = height;
        m_ResX = resX;
        m_ResY = resY;

        m_Particles.clear();
        m_Constraints.clear();

        float spacingX = width / static_cast<float>(resX - 1);
        float spacingY = height / static_cast<float>(resY - 1);

        // Create particles in a grid
        m_Particles.resize(resX * resY);
        for (int y = 0; y < resY; ++y) {
            for (int x = 0; x < resX; ++x) {
                int idx = y * resX + x;
                Particle& p = m_Particles[idx];
                p.position = glm::vec3(
                    x * spacingX - width * 0.5f,
                    0.0f,
                    y * spacingY - height * 0.5f
                );
                p.prevPosition = p.position;
                p.velocity = glm::vec3(0.0f);
                p.acceleration = glm::vec3(0.0f);
                p.invMass = 1.0f;
                p.pinned = false;
            }
        }

        // Create structural constraints (horizontal and vertical)
        for (int y = 0; y < resY; ++y) {
            for (int x = 0; x < resX; ++x) {
                int idx = y * resX + x;

                // Horizontal constraint
                if (x < resX - 1) {
                    int right = idx + 1;
                    float restLen = glm::length(m_Particles[right].position - m_Particles[idx].position);
                    m_Constraints.push_back({ idx, right, restLen, true });
                }

                // Vertical constraint
                if (y < resY - 1) {
                    int below = idx + resX;
                    float restLen = glm::length(m_Particles[below].position - m_Particles[idx].position);
                    m_Constraints.push_back({ idx, below, restLen, true });
                }

                // Diagonal shear constraints for stability
                if (x < resX - 1 && y < resY - 1) {
                    int diagA = idx;
                    int diagB = idx + resX + 1;
                    float restLen = glm::length(m_Particles[diagB].position - m_Particles[diagA].position);
                    m_Constraints.push_back({ diagA, diagB, restLen, true });
                }
                if (x > 0 && y < resY - 1) {
                    int diagA = idx;
                    int diagB = idx + resX - 1;
                    float restLen = glm::length(m_Particles[diagB].position - m_Particles[diagA].position);
                    m_Constraints.push_back({ diagA, diagB, restLen, true });
                }
            }
        }

        m_IndicesDirty = true;
        RebuildIndices();
        UpdatePositionCache();
    }

    void TearableCloth::Update(float dt, int iterations) {
        if (m_Particles.empty()) return;

        float subDt = dt / static_cast<float>(iterations);

        for (int iter = 0; iter < iterations; ++iter) {
            // Apply gravity and integrate velocity (Verlet integration)
            for (auto& p : m_Particles) {
                if (p.pinned) continue;

                glm::vec3 temp = p.position;
                glm::vec3 acceleration = Gravity + p.acceleration;

                // Verlet integration
                p.position = p.position + (p.position - p.prevPosition) * Damping + acceleration * subDt * subDt;
                p.prevPosition = temp;
                p.velocity = (p.position - p.prevPosition) / subDt;
                p.acceleration = glm::vec3(0.0f);
            }

            // Solve constraints
            SolveConstraints();

            // Self-collision
            if (SelfCollisionRadius > 0.0f) {
                SolveSelfCollision();
            }
        }

        // Rebuild indices if any constraints were torn
        if (m_IndicesDirty) {
            RebuildIndices();
        }

        UpdatePositionCache();
    }

    void TearableCloth::PinParticle(int index) {
        if (index >= 0 && index < static_cast<int>(m_Particles.size())) {
            m_Particles[index].pinned = true;
            m_Particles[index].invMass = 0.0f;
        }
    }

    void TearableCloth::UnpinParticle(int index) {
        if (index >= 0 && index < static_cast<int>(m_Particles.size())) {
            m_Particles[index].pinned = false;
            m_Particles[index].invMass = 1.0f;
        }
    }

    void TearableCloth::SolveConstraints() {
        for (auto& c : m_Constraints) {
            if (!c.alive) continue;

            Particle& pA = m_Particles[c.indexA];
            Particle& pB = m_Particles[c.indexB];

            glm::vec3 delta = pB.position - pA.position;
            float dist = glm::length(delta);

            if (dist < 1e-7f) continue;

            // Check for tearing
            if (TearingThreshold > 0.0f) {
                float stretchRatio = dist / c.restLength;
                if (stretchRatio > (1.0f + TearingThreshold)) {
                    c.alive = false;
                    m_IndicesDirty = true;
                    continue;
                }
            }

            // Solve distance constraint
            float diff = (dist - c.restLength) / dist;
            float totalInvMass = pA.invMass + pB.invMass;

            if (totalInvMass < 1e-7f) continue;

            glm::vec3 correction = delta * diff * Stiffness;

            if (!pA.pinned) {
                pA.position += correction * (pA.invMass / totalInvMass);
            }
            if (!pB.pinned) {
                pB.position -= correction * (pB.invMass / totalInvMass);
            }
        }
    }

    void TearableCloth::SolveSelfCollision() {
        // Build octree from current positions
        glm::vec3 minBound(std::numeric_limits<float>::max());
        glm::vec3 maxBound(std::numeric_limits<float>::lowest());

        for (const auto& p : m_Particles) {
            minBound = glm::min(minBound, p.position);
            maxBound = glm::max(maxBound, p.position);
        }

        glm::vec3 center = (minBound + maxBound) * 0.5f;
        glm::vec3 extent = (maxBound - minBound) * 0.5f;
        float halfSize = std::max({ extent.x, extent.y, extent.z }) + 0.1f;

        m_Octree = CreateScope<OctreeAccelerator>(center, halfSize);

        for (int i = 0; i < static_cast<int>(m_Particles.size()); ++i) {
            m_Octree->Insert(i, m_Particles[i].position);
        }

        // For each particle, find nearby particles and push apart
        for (int i = 0; i < static_cast<int>(m_Particles.size()); ++i) {
            const auto& p = m_Particles[i];
            auto neighbors = m_Octree->Query(p.position, SelfCollisionRadius);

            for (int j : neighbors) {
                if (j <= i) continue;  // Avoid duplicate pairs

                // Skip adjacent particles in grid (they are connected by constraints)
                int xi = i % m_ResX, yi = i / m_ResX;
                int xj = j % m_ResX, yj = j / m_ResX;
                if (std::abs(xi - xj) <= 1 && std::abs(yi - yj) <= 1) continue;

                Particle& pA = m_Particles[i];
                Particle& pB = m_Particles[j];

                glm::vec3 delta = pB.position - pA.position;
                float dist = glm::length(delta);

                if (dist < SelfCollisionRadius && dist > 1e-7f) {
                    float overlap = SelfCollisionRadius - dist;
                    glm::vec3 dir = delta / dist;
                    float totalInvMass = pA.invMass + pB.invMass;

                    if (totalInvMass < 1e-7f) continue;

                    glm::vec3 correction = dir * overlap * SelfCollisionStiffness;

                    if (!pA.pinned) {
                        pA.position -= correction * (pA.invMass / totalInvMass);
                    }
                    if (!pB.pinned) {
                        pB.position += correction * (pB.invMass / totalInvMass);
                    }
                }
            }
        }
    }

    void TearableCloth::RebuildIndices() {
        m_IndexCache.clear();

        // Build a set of alive edges for fast lookup
        // An edge is represented as the constraint between two grid-adjacent particles
        // We need to check if the structural constraints forming each triangle are alive

        // Create a connectivity map: for each pair (a, b), is there a living constraint?
        std::unordered_set<uint64_t> aliveEdges;
        for (const auto& c : m_Constraints) {
            if (!c.alive) continue;
            int a = std::min(c.indexA, c.indexB);
            int b = std::max(c.indexA, c.indexB);
            uint64_t key = (static_cast<uint64_t>(a) << 32) | static_cast<uint64_t>(b);
            aliveEdges.insert(key);
        }

        auto hasEdge = [&](int a, int b) -> bool {
            int lo = std::min(a, b);
            int hi = std::max(a, b);
            uint64_t key = (static_cast<uint64_t>(lo) << 32) | static_cast<uint64_t>(hi);
            return aliveEdges.find(key) != aliveEdges.end();
        };

        // Generate triangles for each quad in the grid
        for (int y = 0; y < m_ResY - 1; ++y) {
            for (int x = 0; x < m_ResX - 1; ++x) {
                int topLeft = y * m_ResX + x;
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + m_ResX;
                int bottomRight = bottomLeft + 1;

                // Triangle 1: topLeft, bottomLeft, topRight
                if (hasEdge(topLeft, bottomLeft) && hasEdge(topLeft, topRight) && hasEdge(bottomLeft, topRight)) {
                    m_IndexCache.push_back(static_cast<uint32_t>(topLeft));
                    m_IndexCache.push_back(static_cast<uint32_t>(bottomLeft));
                    m_IndexCache.push_back(static_cast<uint32_t>(topRight));
                }

                // Triangle 2: topRight, bottomLeft, bottomRight
                if (hasEdge(topRight, bottomLeft) && hasEdge(topRight, bottomRight) && hasEdge(bottomLeft, bottomRight)) {
                    m_IndexCache.push_back(static_cast<uint32_t>(topRight));
                    m_IndexCache.push_back(static_cast<uint32_t>(bottomLeft));
                    m_IndexCache.push_back(static_cast<uint32_t>(bottomRight));
                }
            }
        }

        m_IndicesDirty = false;
    }

    void TearableCloth::UpdatePositionCache() {
        m_PositionCache.resize(m_Particles.size());
        for (size_t i = 0; i < m_Particles.size(); ++i) {
            m_PositionCache[i] = m_Particles[i].position;
        }
    }

}}
