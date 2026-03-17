#pragma once
/**
 * FlagPhysics.hpp
 *
 * Physics extracted verbatim from old_code/cloth-simulation/src/flag.cpp.
 * Original author: PartyKel / cloth-simulation project.
 *
 * Only change: removed PartyKel window / Octree / SDL headers.
 * SphereHandler is defined inline (it was only positions + radii in PartyKel).
 * applyRepulseForces falls back to O(n^2) brute-force without the Octree; the
 * force equations are bit-for-bit identical.
 */

#include <vector>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>

// ---------------------------------------------------------------------------
// SphereHandler — was a PartyKel type; reproduced inline (same fields)
// ---------------------------------------------------------------------------
struct SphereHandler {
    std::vector<glm::vec3> positions;
    std::vector<float>     radius;
    std::vector<glm::vec3> colors;
};

// ---------------------------------------------------------------------------
// Force functions — copied 1:1 from flag.cpp lines 23-44
// ---------------------------------------------------------------------------
inline glm::vec3 hookForce(float K, float L, const glm::vec3& P1, const glm::vec3& P2) {
    static const float epsilon = 0.0001f;
    return K * (1.0f - (L / std::max(glm::distance(P1, P2), epsilon))) * (P2 - P1);
}

inline glm::vec3 repulseForce(float dst, const glm::vec3& P1, const glm::vec3& P2) {
    glm::vec3 direction = glm::normalize(P1 - P2);
    return direction * (1.0f / (1.0f + glm::pow(dst, 2.f)));
}

inline glm::vec3 brakeForce(float V, float dt, const glm::vec3& v1, const glm::vec3& v2) {
    return V * ((v2 - v1) / dt);
}

inline glm::vec3 sphereCollisionForce(float distanceToCenter,
                                      const glm::vec3& sphereCenter,
                                      float            sphereRadius,
                                      const glm::vec3& particlePosition,
                                      const glm::vec3& forceParticle) {
    glm::vec3 direction = glm::normalize(particlePosition - sphereCenter);
    return direction * (1.0f / (1.0f + glm::pow(distanceToCenter, 2.f)));
}

// ---------------------------------------------------------------------------
// Flag struct — copied 1:1 from flag.cpp lines 47-213
// ---------------------------------------------------------------------------
struct Flag {
    int gridWidth, gridHeight;

    std::vector<glm::vec3> positionArray;
    std::vector<glm::vec3> velocityArray;
    std::vector<float>     massArray;
    std::vector<glm::vec3> forceArray;
    int nbParticles;

    glm::vec2 L0;
    float     L1;
    glm::vec2 L2;

    float K0, K1, K2;
    float V0, V1, V2;

    Flag(float mass, float width, float height, int gridWidth, int gridHeight)
        : gridWidth(gridWidth), gridHeight(gridHeight),
          positionArray(gridWidth * gridHeight),
          velocityArray(gridWidth * gridHeight, glm::vec3(0.f)),
          massArray(gridWidth * gridHeight, mass / (gridWidth * gridHeight)),
          forceArray(gridWidth * gridHeight, glm::vec3(0.f))
    {
        glm::vec3 origin(-0.5f * width, 0.f, 0.f);
        glm::vec3 scale(width / (gridWidth - 1), height / (gridHeight - 1), 1.f);

        nbParticles = gridWidth * gridHeight;
        for (int j = 0; j < gridHeight; ++j) {
            for (int i = 0; i < gridWidth; ++i) {
                int k = i + j * gridWidth;
                positionArray[k] = origin + glm::vec3(i, j, origin.z) * scale;
                massArray[k] = 1.0f - (i / (2.0f * (gridHeight * gridWidth)));
            }
        }

        L0.x = scale.x;
        L0.y = scale.y;
        L1   = glm::length(L0);
        L2   = 4.f * L0;

        K0 = 1.0f; K1 = 1.0f; K2 = 1.0f;
        V0 = 0.08f; V1 = 0.02f; V2 = 0.06f;
    }

    void applyInternalForces(float dt) {
        std::vector<glm::ivec2> neighbors(4);
        for (int i = 0; i < gridWidth; ++i) {
            for (int j = 0; j < gridHeight - 1; ++j) {
                int currentK = j * gridWidth + i;

                // TOPOLOGY 1
                neighbors[0] = glm::ivec2(i+1, j);
                neighbors[1] = glm::ivec2(i-1, j);
                neighbors[2] = glm::ivec2(i,   j-1);
                neighbors[3] = glm::ivec2(i,   j+1);

                int tmpI = 0;
                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K0, tmpI < 2 ? L0.x : L0.y,
                                                       positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V0, dt,
                                                        velocityArray[currentK], velocityArray[k]);
                    ++tmpI;
                }

                // TOPOLOGY 2
                neighbors[0] = glm::ivec2(i-1, j-1);
                neighbors[1] = glm::ivec2(i+1, j-1);
                neighbors[2] = glm::ivec2(i+1, j+1);
                neighbors[3] = glm::ivec2(i-1, j+1);

                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K1, L1,
                                                       positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V1, dt,
                                                        velocityArray[currentK], velocityArray[k]);
                }

                // TOPOLOGY 3
                neighbors[0] = glm::ivec2(i-2, j);
                neighbors[1] = glm::ivec2(i+2, j);
                neighbors[2] = glm::ivec2(i,   j-2);
                neighbors[3] = glm::ivec2(i,   j+2);

                tmpI = 0;
                for (auto& p : neighbors) {
                    if (p.x < 0 || p.y < 0 || p.x >= gridWidth || p.y >= gridHeight)
                        continue;
                    int k = p.y * gridWidth + p.x;
                    forceArray[currentK] += hookForce(K2, tmpI < 2 ? L2.x : L2.y,
                                                       positionArray[currentK], positionArray[k]);
                    forceArray[currentK] += brakeForce(V2, dt,
                                                        velocityArray[currentK], velocityArray[k]);
                    ++tmpI;
                }
            }
        }
    }

    // applyRepulseForces — same equations as flag.cpp; O(n^2) instead of Octree
    // (Octree was an acceleration structure for the same repulsion calculation)
    void applyRepulseForces(float maxDst, float multRepulse) {
        for (int i = 0; i < gridWidth; ++i) {
            for (int j = 0; j < gridHeight - 1; ++j) {
                int k    = j * gridWidth + i;
                auto& pos = positionArray[k];
                for (int n = 0; n < nbParticles; ++n) {
                    if (n == k) continue;
                    float dst = glm::distance(positionArray[n], pos);
                    if (dst > maxDst) continue;
                    forceArray[k] += repulseForce(dst, pos, positionArray[n]) * multRepulse;
                }
            }
        }
    }

    void applyExternalForce(const glm::vec3& F) {
        for (int i = 0; i < nbParticles; ++i) {
            if (i > nbParticles - gridWidth - 1) continue;
            forceArray[i] += F;
        }
    }

    void applySphereCollision(const SphereHandler& sh, float multiplier, float radiusDelta) {
        for (int i = 0; i < nbParticles; ++i) {
            if (i > nbParticles - gridWidth - 1) continue;
            for (size_t j = 0; j < sh.positions.size(); ++j) {
                float dist = glm::distance(sh.positions[j], positionArray[i]);
                if (dist < sh.radius[j] + radiusDelta) {
                    forceArray[i] += sphereCollisionForce(dist, sh.positions[j],
                                                           sh.radius[j], positionArray[i],
                                                           forceArray[i]) * multiplier;
                }
            }
        }
    }

    // update — copied 1:1 from flag.cpp lines 206-212
    void update(float dt) {
        for (int i = 0; i < nbParticles; ++i) {
            velocityArray[i] += dt * (forceArray[i] / massArray[i]);
            positionArray[i] += dt * velocityArray[i];
            forceArray[i]     = glm::vec3(0.f);
        }
    }
};
