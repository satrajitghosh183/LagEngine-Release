#include "SkeletalIK.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {

    // Helper: resolve world-space bone positions along a chain.
    static void ComputeWorldPositions(const AnimationPose& pose,
                                       const std::vector<int>& chain,
                                       std::vector<glm::vec3>& outPositions) {
        outPositions.resize(chain.size());
        glm::vec3 pos{0.0f};
        for (size_t i = 0; i < chain.size(); i++) {
            int idx = chain[i];
            if (idx < 0 || idx >= static_cast<int>(pose.Bones.size())) {
                outPositions[i] = pos;
                continue;
            }
            pos += pose.Bones[idx].Position;
            outPositions[i] = pos;
        }
    }

    // ---- TwoBoneIK ---------------------------------------------------------

    bool TwoBoneIK::Solve(AnimationPose& pose, const IKChainSpec& chain) {
        if (chain.BoneIndices.size() != 3) return false;
        int r = chain.BoneIndices[0];
        int m = chain.BoneIndices[1];
        int e = chain.BoneIndices[2];
        if (r < 0 || m < 0 || e < 0) return false;
        if ((size_t)std::max({r, m, e}) >= pose.Bones.size()) return false;

        glm::vec3 rootPos = pose.Bones[r].Position;
        glm::vec3 midPos  = pose.Bones[r].Position + pose.Bones[m].Position;
        glm::vec3 endPos  = midPos + pose.Bones[e].Position;

        float upperLen = glm::length(midPos - rootPos);
        float lowerLen = glm::length(endPos - midPos);
        float totalLen = upperLen + lowerLen;

        glm::vec3 toTarget = chain.TargetPosition - rootPos;
        float dist = glm::length(toTarget);
        if (dist < 1e-4f) return false;

        // If out of reach: just point at target
        if (dist >= totalLen) {
            glm::vec3 dir = toTarget / dist;
            pose.Bones[m].Position = dir * upperLen;
            pose.Bones[e].Position = dir * lowerLen;
            return true;
        }

        // Law of cosines for bend angle
        float cosElbow = (upperLen * upperLen + lowerLen * lowerLen - dist * dist)
                         / (2.0f * upperLen * lowerLen);
        cosElbow = std::max(-1.0f, std::min(1.0f, cosElbow));
        float elbowAngle = std::acos(cosElbow);

        float cosShoulder = (upperLen * upperLen + dist * dist - lowerLen * lowerLen)
                            / (2.0f * upperLen * dist);
        cosShoulder = std::max(-1.0f, std::min(1.0f, cosShoulder));
        float shoulderAngle = std::acos(cosShoulder);

        glm::vec3 targetDir = toTarget / dist;
        glm::vec3 bendAxis = glm::cross(targetDir, chain.PoleVector);
        if (glm::length(bendAxis) < 1e-4f) bendAxis = glm::vec3(0, 0, 1);
        else bendAxis = glm::normalize(bendAxis);

        glm::quat shoulderRot = glm::angleAxis(-shoulderAngle, bendAxis);
        glm::vec3 upperDir = shoulderRot * targetDir;
        glm::vec3 newMidPos = rootPos + upperDir * upperLen;

        glm::vec3 newEndPos = chain.TargetPosition;

        pose.Bones[m].Position = newMidPos - rootPos;
        pose.Bones[e].Position = newEndPos - newMidPos;

        (void)elbowAngle; // Derived implicitly from chosen positions
        return true;
    }

    // ---- CCDIK -------------------------------------------------------------

    bool CCDIK::Solve(AnimationPose& pose, const IKChainSpec& chain) {
        if (chain.BoneIndices.size() < 2) return false;
        const auto& c = chain.BoneIndices;
        for (int idx : c) {
            if (idx < 0 || (size_t)idx >= pose.Bones.size()) return false;
        }

        std::vector<glm::vec3> positions;
        for (int it = 0; it < chain.MaxIterations; it++) {
            ComputeWorldPositions(pose, c, positions);

            glm::vec3 end = positions.back();
            if (glm::distance(end, chain.TargetPosition) < chain.Tolerance) return true;

            // Iterate from second-last to root
            for (int i = static_cast<int>(c.size()) - 2; i >= 0; i--) {
                ComputeWorldPositions(pose, c, positions);
                glm::vec3 bonePos = positions[i];
                glm::vec3 endPos  = positions.back();

                glm::vec3 toEnd    = endPos - bonePos;
                glm::vec3 toTarget = chain.TargetPosition - bonePos;

                if (glm::length(toEnd) < 1e-4f || glm::length(toTarget) < 1e-4f) continue;

                glm::vec3 from = glm::normalize(toEnd);
                glm::vec3 to   = glm::normalize(toTarget);

                float cosAng = std::max(-1.0f, std::min(1.0f, glm::dot(from, to)));
                float angle = std::acos(cosAng);
                if (angle < 1e-4f) continue;

                glm::vec3 axis = glm::cross(from, to);
                if (glm::length(axis) < 1e-4f) continue;
                axis = glm::normalize(axis);

                glm::quat rot = glm::angleAxis(angle, axis);
                pose.Bones[c[i]].Rotation = rot * pose.Bones[c[i]].Rotation;
            }
        }
        return true;
    }

    // ---- FABRIK ------------------------------------------------------------

    bool FABRIKSolver::Solve(AnimationPose& pose, const IKChainSpec& chain) {
        if (chain.BoneIndices.size() < 2) return false;
        const auto& c = chain.BoneIndices;
        for (int idx : c) {
            if (idx < 0 || (size_t)idx >= pose.Bones.size()) return false;
        }

        // Compute bone lengths (distance between consecutive bones)
        std::vector<glm::vec3> positions;
        ComputeWorldPositions(pose, c, positions);
        std::vector<float> lengths(c.size() - 1);
        for (size_t i = 0; i + 1 < positions.size(); i++) {
            lengths[i] = glm::distance(positions[i], positions[i + 1]);
        }
        float totalLen = 0.0f;
        for (float l : lengths) totalLen += l;

        glm::vec3 rootPos = positions.front();
        float distToTarget = glm::distance(rootPos, chain.TargetPosition);

        // If unreachable, fully extend toward target
        if (distToTarget > totalLen) {
            glm::vec3 dir = glm::normalize(chain.TargetPosition - rootPos);
            for (size_t i = 1; i < positions.size(); i++) {
                positions[i] = positions[i - 1] + dir * lengths[i - 1];
            }
        } else {
            for (int it = 0; it < chain.MaxIterations; it++) {
                // Backward pass: end → root
                positions.back() = chain.TargetPosition;
                for (int i = static_cast<int>(positions.size()) - 2; i >= 0; i--) {
                    glm::vec3 d = positions[i] - positions[i + 1];
                    float len = glm::length(d);
                    if (len < 1e-5f) continue;
                    positions[i] = positions[i + 1] + (d / len) * lengths[i];
                }
                // Forward pass: root → end
                positions.front() = rootPos;
                for (size_t i = 1; i < positions.size(); i++) {
                    glm::vec3 d = positions[i] - positions[i - 1];
                    float len = glm::length(d);
                    if (len < 1e-5f) continue;
                    positions[i] = positions[i - 1] + (d / len) * lengths[i - 1];
                }
                if (glm::distance(positions.back(), chain.TargetPosition) < chain.Tolerance) break;
            }
        }

        // Write back as relative positions
        pose.Bones[c[0]].Position = positions[0];
        for (size_t i = 1; i < c.size(); i++) {
            pose.Bones[c[i]].Position = positions[i] - positions[i - 1];
        }
        return true;
    }

    // ---- IKSystem ----------------------------------------------------------

    void IKSystem::AddChain(const IKChainSpec& spec, Method method) {
        m_Chains.push_back({ spec, method, true });
    }

    void IKSystem::RemoveChain(size_t index) {
        if (index < m_Chains.size()) m_Chains.erase(m_Chains.begin() + index);
    }

    void IKSystem::Apply(AnimationPose& pose) {
        for (auto& entry : m_Chains) {
            if (!entry.Enabled) continue;
            switch (entry.SolveMethod) {
                case Method::TwoBone: TwoBoneIK::Solve(pose, entry.Spec); break;
                case Method::CCD:     CCDIK::Solve(pose, entry.Spec); break;
                case Method::FABRIK:  FABRIKSolver::Solve(pose, entry.Spec); break;
            }
        }
    }

}
