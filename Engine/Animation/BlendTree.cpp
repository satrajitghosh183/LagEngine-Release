#include "BlendTree.hpp"
#include <algorithm>
#include <cstdlib>

namespace GameEngine {

    AnimationPose AnimationPose::Blend(const AnimationPose& a, const AnimationPose& b, float t) {
        if (t <= 0.0f) return a;
        if (t >= 1.0f) return b;

        AnimationPose out;
        size_t count = std::min(a.Bones.size(), b.Bones.size());
        out.Bones.resize(count);
        for (size_t i = 0; i < count; i++) {
            out.Bones[i].Position = glm::mix(a.Bones[i].Position, b.Bones[i].Position, t);
            out.Bones[i].Rotation = glm::slerp(a.Bones[i].Rotation, b.Bones[i].Rotation, t);
            out.Bones[i].Scale    = glm::mix(a.Bones[i].Scale,    b.Bones[i].Scale,    t);
        }
        return out;
    }

    AnimationPose AnimationPose::Add(const AnimationPose& base, const AnimationPose& additive, float weight) {
        AnimationPose out;
        size_t count = std::min(base.Bones.size(), additive.Bones.size());
        out.Bones.resize(count);
        for (size_t i = 0; i < count; i++) {
            out.Bones[i].Position = base.Bones[i].Position + additive.Bones[i].Position * weight;
            glm::quat addRot = glm::slerp(glm::quat(1, 0, 0, 0), additive.Bones[i].Rotation, weight);
            out.Bones[i].Rotation = addRot * base.Bones[i].Rotation;
            out.Bones[i].Scale    = base.Bones[i].Scale * glm::mix(glm::vec3(1.0f), additive.Bones[i].Scale, weight);
        }
        return out;
    }

    AnimationPose ClipNode::Evaluate(float deltaTime, const BlendTreeParameters&) {
        if (!Clip) return AnimationPose{};

        m_Time += deltaTime * TimeScale;
        float duration = Clip->GetDuration();
        if (duration > 0.0f) {
            if (Loop) m_Time = std::fmod(m_Time, duration);
            else m_Time = std::min(m_Time, duration);
        }

        // Sample each channel into a pose, indexed by BoneID.
        AnimationPose pose;
        const auto& channels = Clip->GetChannels();
        int maxBone = 0;
        for (const auto& ch : channels) maxBone = std::max(maxBone, ch.BoneID);
        pose.Bones.resize(static_cast<size_t>(maxBone + 1));

        for (const auto& ch : channels) {
            glm::vec3 pos, scl; glm::quat rot;
            ch.GetTransformAtTime(m_Time, pos, rot, scl);
            if (ch.BoneID >= 0 && ch.BoneID < static_cast<int>(pose.Bones.size())) {
                pose.Bones[ch.BoneID].Position = pos;
                pose.Bones[ch.BoneID].Rotation = rot;
                pose.Bones[ch.BoneID].Scale    = scl;
            }
        }
        return pose;
    }

    AnimationPose Blend1DNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (Children.empty()) return {};
        if (Children.size() == 1) return Children[0].Child->Evaluate(deltaTime, params);

        float p = params.GetFloat(ParameterName);

        // Find the two children bracketing p
        std::vector<Entry> sorted = Children;
        std::sort(sorted.begin(), sorted.end(),
                  [](const Entry& a, const Entry& b) { return a.Position < b.Position; });

        if (p <= sorted.front().Position) return sorted.front().Child->Evaluate(deltaTime, params);
        if (p >= sorted.back().Position)  return sorted.back().Child->Evaluate(deltaTime, params);

        for (size_t i = 0; i + 1 < sorted.size(); i++) {
            if (p >= sorted[i].Position && p <= sorted[i+1].Position) {
                float span = sorted[i+1].Position - sorted[i].Position;
                float t = span > 0.0f ? (p - sorted[i].Position) / span : 0.0f;
                auto a = sorted[i].Child->Evaluate(deltaTime, params);
                auto b = sorted[i+1].Child->Evaluate(deltaTime, params);
                return AnimationPose::Blend(a, b, t);
            }
        }
        return sorted.front().Child->Evaluate(deltaTime, params);
    }

    AnimationPose Blend2DNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (Children.empty()) return {};
        if (Children.size() == 1) return Children[0].Child->Evaluate(deltaTime, params);

        glm::vec2 p{ params.GetFloat(ParameterX), params.GetFloat(ParameterY) };

        // Simple inverse-distance weighted blend across all children (fast, reasonable quality)
        float totalWeight = 0.0f;
        std::vector<float> weights(Children.size());
        for (size_t i = 0; i < Children.size(); i++) {
            float d = glm::distance(p, Children[i].Position);
            float w = 1.0f / (d * d + 0.001f);
            weights[i] = w;
            totalWeight += w;
        }
        if (totalWeight <= 0.0f) return Children[0].Child->Evaluate(deltaTime, params);

        AnimationPose result = Children[0].Child->Evaluate(deltaTime, params);
        float accum = weights[0] / totalWeight;
        for (size_t i = 1; i < Children.size(); i++) {
            auto pose = Children[i].Child->Evaluate(deltaTime, params);
            float w = weights[i] / totalWeight;
            float t = w / std::max(0.0001f, accum + w);
            result = AnimationPose::Blend(result, pose, t);
            accum += w;
        }
        return result;
    }

    AnimationPose AddNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (!Base) return {};
        auto basePose = Base->Evaluate(deltaTime, params);
        if (!Additive) return basePose;
        float w = WeightParameter.empty() ? FixedWeight : params.GetFloat(WeightParameter, FixedWeight);
        auto addPose = Additive->Evaluate(deltaTime, params);
        return AnimationPose::Add(basePose, addPose, w);
    }

    AnimationPose MirrorNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (!Input) return {};
        auto pose = Input->Evaluate(deltaTime, params);

        AnimationPose mirrored = pose;
        for (size_t i = 0; i < MirrorMap.size(); i++) {
            int partner = MirrorMap[i];
            if (partner < 0 || partner == static_cast<int>(i)) continue;
            if (partner < static_cast<int>(pose.Bones.size()) && i < pose.Bones.size()) {
                // Mirror across YZ plane (x-flip)
                mirrored.Bones[i] = pose.Bones[partner];
                mirrored.Bones[i].Position.x = -mirrored.Bones[i].Position.x;
                mirrored.Bones[i].Rotation.x = -mirrored.Bones[i].Rotation.x;
                mirrored.Bones[i].Rotation.w = -mirrored.Bones[i].Rotation.w;
            }
        }
        return mirrored;
    }

    AnimationPose RandomNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (Children.empty()) return {};
        if (m_CurrentIndex < 0 || m_CurrentIndex >= static_cast<int>(Children.size())) {
            m_CurrentIndex = std::rand() % Children.size();
        }
        return Children[m_CurrentIndex]->Evaluate(deltaTime, params);
    }

    AnimationPose TimeScaleNode::Evaluate(float deltaTime, const BlendTreeParameters& params) {
        if (!Input) return {};
        float scale = ScaleParameter.empty() ? FixedScale : params.GetFloat(ScaleParameter, FixedScale);
        return Input->Evaluate(deltaTime * scale, params);
    }

}
