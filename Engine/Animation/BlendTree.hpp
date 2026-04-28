#pragma once

#include "../Core/Base.hpp"
#include "AnimationClip.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <variant>

namespace GameEngine {

    struct AnimationPose {
        struct BoneTransform {
            glm::vec3 Position{0.0f};
            glm::quat Rotation{1, 0, 0, 0};
            glm::vec3 Scale{1.0f};
        };
        std::vector<BoneTransform> Bones;

        static AnimationPose Blend(const AnimationPose& a, const AnimationPose& b, float t);
        static AnimationPose Add(const AnimationPose& base, const AnimationPose& additive, float weight);
    };

    // Shared parameter store for all blend nodes in a tree
    struct BlendTreeParameters {
        std::unordered_map<std::string, float> Floats;
        std::unordered_map<std::string, bool>  Bools;
        std::unordered_map<std::string, int>   Ints;

        float GetFloat(const std::string& name, float defVal = 0.0f) const {
            auto it = Floats.find(name);
            return it == Floats.end() ? defVal : it->second;
        }
        bool GetBool(const std::string& name, bool defVal = false) const {
            auto it = Bools.find(name);
            return it == Bools.end() ? defVal : it->second;
        }
        int GetInt(const std::string& name, int defVal = 0) const {
            auto it = Ints.find(name);
            return it == Ints.end() ? defVal : it->second;
        }
    };

    // Abstract blend node. Subclasses evaluate a pose from their inputs.
    class BlendNode {
    public:
        virtual ~BlendNode() = default;
        virtual AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) = 0;
        std::string Name;
    };

    // Plays a single clip
    class ClipNode : public BlendNode {
    public:
        Ref<AnimationClip> Clip;
        float TimeScale = 1.0f;
        bool Loop = true;
        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters&) override;
    private:
        float m_Time = 0.0f;
    };

    // 1D blend (e.g., locomotion speed) — interpolates between children by a parameter
    class Blend1DNode : public BlendNode {
    public:
        struct Entry { Ref<BlendNode> Child; float Position; };
        std::vector<Entry> Children;
        std::string ParameterName;

        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) override;
    };

    // 2D blend (e.g., directional locomotion) — barycentric interpolation over triangles
    class Blend2DNode : public BlendNode {
    public:
        struct Entry { Ref<BlendNode> Child; glm::vec2 Position; };
        std::vector<Entry> Children;
        std::string ParameterX;
        std::string ParameterY;

        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) override;
    };

    // Additive node — adds a pose on top of a base pose
    class AddNode : public BlendNode {
    public:
        Ref<BlendNode> Base;
        Ref<BlendNode> Additive;
        std::string WeightParameter;
        float FixedWeight = 1.0f;

        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) override;
    };

    // Mirror node — flips a pose left↔right for bilateral animations
    class MirrorNode : public BlendNode {
    public:
        Ref<BlendNode> Input;
        std::vector<int> MirrorMap; // bone index mirror pairs

        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) override;
    };

    // Random — picks one of N children per invocation (can be time-gated)
    class RandomNode : public BlendNode {
    public:
        std::vector<Ref<BlendNode>> Children;
        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters&) override;
    private:
        int m_CurrentIndex = -1;
    };

    // TimeScale — stretches/compresses time of inner node
    class TimeScaleNode : public BlendNode {
    public:
        Ref<BlendNode> Input;
        std::string ScaleParameter;
        float FixedScale = 1.0f;

        AnimationPose Evaluate(float deltaTime, const BlendTreeParameters& params) override;
    };

    // The blend tree wrapper — a root node + parameters
    class BlendTree {
    public:
        Ref<BlendNode> Root;
        BlendTreeParameters Parameters;

        AnimationPose Evaluate(float deltaTime) {
            return Root ? Root->Evaluate(deltaTime, Parameters) : AnimationPose{};
        }

        void SetFloat(const std::string& name, float v) { Parameters.Floats[name] = v; }
        void SetBool(const std::string& name, bool v)   { Parameters.Bools[name]  = v; }
        void SetInt(const std::string& name, int v)     { Parameters.Ints[name]   = v; }
    };

}
