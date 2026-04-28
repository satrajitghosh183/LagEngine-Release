#pragma once

#include "BlendTree.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace GameEngine {

    // A skeletal IK chain: root bone index, end-effector, optional pole vector.
    // Operates on an AnimationPose's bone list.
    struct IKChainSpec {
        std::vector<int> BoneIndices; // root...end
        glm::vec3 TargetPosition{0.0f};
        glm::vec3 PoleVector{0, 0, 1};
        float Weight = 1.0f;
        int MaxIterations = 10;
        float Tolerance = 0.001f;
    };

    class TwoBoneIK {
    public:
        // Analytical solve for exactly 3 bones (root, mid, end) aiming end at target.
        // Uses law of cosines + pole vector to lock knee/elbow plane.
        static bool Solve(AnimationPose& pose, const IKChainSpec& chain);
    };

    class CCDIK {
    public:
        // Iterative cyclic coordinate descent. Works for N-bone chains.
        static bool Solve(AnimationPose& pose, const IKChainSpec& chain);
    };

    class FABRIKSolver {
    public:
        // Forward-and-backward reaching IK. Faster than CCD for long chains.
        static bool Solve(AnimationPose& pose, const IKChainSpec& chain);
    };

    // Runtime IK system: owns a set of chains, applies them after animation evaluation.
    class IKSystem {
    public:
        enum class Method { TwoBone, CCD, FABRIK };

        struct Entry {
            IKChainSpec Spec;
            Method SolveMethod;
            bool Enabled = true;
        };

        void AddChain(const IKChainSpec& spec, Method method = Method::CCD);
        void RemoveChain(size_t index);
        void Apply(AnimationPose& pose);

        std::vector<Entry>& GetChains() { return m_Chains; }
        const std::vector<Entry>& GetChains() const { return m_Chains; }

    private:
        std::vector<Entry> m_Chains;
    };

}
