#include "AnimationStateMachine.hpp"
#include <algorithm>
#include <cmath>

namespace GameEngine {

    // =====================================================================
    // AnimationPose
    // =====================================================================

    AnimationPose AnimationPose::blend(const AnimationPose& a, const AnimationPose& b, float weight) {
        AnimationPose result;
        size_t count = std::max(a.BoneTransforms.size(), b.BoneTransforms.size());
        result.BoneTransforms.resize(count);

        for (size_t i = 0; i < count; i++) {
            const auto& boneA = (i < a.BoneTransforms.size()) ? a.BoneTransforms[i] : BoneTransform{};
            const auto& boneB = (i < b.BoneTransforms.size()) ? b.BoneTransforms[i] : BoneTransform{};

            result.BoneTransforms[i].Position = glm::mix(boneA.Position, boneB.Position, weight);
            result.BoneTransforms[i].Rotation = glm::slerp(boneA.Rotation, boneB.Rotation, weight);
            result.BoneTransforms[i].Scale = glm::mix(boneA.Scale, boneB.Scale, weight);
        }
        return result;
    }

    AnimationPose AnimationPose::additive(const AnimationPose& base, const AnimationPose& add, float weight) {
        AnimationPose result = base;
        size_t count = std::min(base.BoneTransforms.size(), add.BoneTransforms.size());

        for (size_t i = 0; i < count; i++) {
            result.BoneTransforms[i].Position += add.BoneTransforms[i].Position * weight;
            glm::quat additiveRot = glm::slerp(glm::quat(1, 0, 0, 0), add.BoneTransforms[i].Rotation, weight);
            result.BoneTransforms[i].Rotation = additiveRot * base.BoneTransforms[i].Rotation;
            result.BoneTransforms[i].Scale *= glm::mix(glm::vec3(1.0f), add.BoneTransforms[i].Scale, weight);
        }
        return result;
    }

    // =====================================================================
    // AnimationStateMachine
    // =====================================================================

    void AnimationStateMachine::addState(const std::string& name, Ref<AnimationClip> clip, float speed, bool loop) {
        AnimationState state;
        state.Name = name;
        state.Clip = clip;
        state.Speed = speed;
        state.Loop = loop;
        m_States[name] = std::move(state);

        if (m_CurrentState.empty()) {
            m_CurrentState = name;
        }
    }

    void AnimationStateMachine::removeState(const std::string& name) {
        m_States.erase(name);
        // Remove transitions referencing this state
        m_Transitions.erase(
            std::remove_if(m_Transitions.begin(), m_Transitions.end(),
                [&name](const AnimationTransition& t) {
                    return t.FromState == name || t.ToState == name;
                }),
            m_Transitions.end());
    }

    void AnimationStateMachine::setEntryState(const std::string& name) {
        if (m_States.find(name) != m_States.end()) {
            m_CurrentState = name;
            m_CurrentTime = 0.0f;
        }
    }

    const AnimationState* AnimationStateMachine::getCurrentState() const {
        auto it = m_States.find(m_CurrentState);
        return (it != m_States.end()) ? &it->second : nullptr;
    }

    void AnimationStateMachine::addTransition(const AnimationTransition& transition) {
        m_Transitions.push_back(transition);
    }

    void AnimationStateMachine::addAnyStateTransition(const std::string& toState,
                                                       const std::vector<TransitionCondition>& conditions,
                                                       float duration) {
        AnimationTransition t;
        t.ToState = toState;
        t.Duration = duration;
        t.Conditions = conditions;
        m_AnyStateTransitions.push_back(t);
    }

    // Parameter management
    void AnimationStateMachine::addFloat(const std::string& name, float val) { m_Parameters[name] = val; }
    void AnimationStateMachine::addInt(const std::string& name, int val) { m_Parameters[name] = val; }
    void AnimationStateMachine::addBool(const std::string& name, bool val) { m_Parameters[name] = val; }
    void AnimationStateMachine::addTrigger(const std::string& name) { m_Triggers[name] = false; }

    void AnimationStateMachine::setFloat(const std::string& name, float val) { m_Parameters[name] = val; }
    void AnimationStateMachine::setInt(const std::string& name, int val) { m_Parameters[name] = val; }
    void AnimationStateMachine::setBool(const std::string& name, bool val) { m_Parameters[name] = val; }
    void AnimationStateMachine::setTrigger(const std::string& name) { m_Triggers[name] = true; }

    float AnimationStateMachine::getFloat(const std::string& name) const {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end() && std::holds_alternative<float>(it->second))
            return std::get<float>(it->second);
        return 0.0f;
    }

    int AnimationStateMachine::getInt(const std::string& name) const {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end() && std::holds_alternative<int>(it->second))
            return std::get<int>(it->second);
        return 0;
    }

    bool AnimationStateMachine::getBool(const std::string& name) const {
        auto it = m_Parameters.find(name);
        if (it != m_Parameters.end() && std::holds_alternative<bool>(it->second))
            return std::get<bool>(it->second);
        return false;
    }

    void AnimationStateMachine::update(float dt) {
        auto currentIt = m_States.find(m_CurrentState);
        if (currentIt == m_States.end()) return;

        const auto& currentState = currentIt->second;

        // Update playback time
        m_PreviousTime = m_CurrentTime;
        if (currentState.Clip) {
            float duration = currentState.Clip->GetDuration();
            if (duration > 0.0f) {
                m_CurrentTime += dt * currentState.Speed;
                if (currentState.Loop) {
                    while (m_CurrentTime >= duration) m_CurrentTime -= duration;
                } else {
                    m_CurrentTime = std::min(m_CurrentTime, duration);
                }
            }
        }

        // Fire animation events
        if (m_EventCallback) {
            for (const auto& event : currentState.Events) {
                float eventTime = event.Time;
                if (currentState.Clip) eventTime *= currentState.Clip->GetDuration();
                if (m_PreviousTime <= eventTime && m_CurrentTime > eventTime) {
                    m_EventCallback(event);
                }
            }
        }

        // Update transition
        if (m_IsTransitioning) {
            m_TransitionProgress += dt / m_TransitionDuration;
            if (m_TransitionProgress >= 1.0f) {
                m_CurrentState = m_NextState;
                m_CurrentTime = 0.0f;
                m_IsTransitioning = false;
                m_TransitionProgress = 0.0f;
            }
            resetTriggers();
            return;
        }

        // Check any-state transitions first
        for (const auto& transition : m_AnyStateTransitions) {
            if (transition.ToState == m_CurrentState) continue;
            if (evaluateConditions(transition.Conditions)) {
                m_NextState = transition.ToState;
                m_TransitionDuration = std::max(transition.Duration, 0.001f);
                m_TransitionProgress = 0.0f;
                m_IsTransitioning = true;
                resetTriggers();
                return;
            }
        }

        // Check state-specific transitions
        for (const auto& transition : m_Transitions) {
            if (transition.FromState != m_CurrentState) continue;

            bool shouldTransition = false;

            if (transition.HasExitTime && currentState.Clip) {
                float normalizedTime = m_CurrentTime / currentState.Clip->GetDuration();
                if (normalizedTime >= transition.ExitTime) {
                    shouldTransition = transition.Conditions.empty() || evaluateConditions(transition.Conditions);
                }
            } else {
                shouldTransition = evaluateConditions(transition.Conditions);
            }

            if (shouldTransition) {
                m_NextState = transition.ToState;
                m_TransitionDuration = std::max(transition.Duration, 0.001f);
                m_TransitionProgress = 0.0f;
                m_IsTransitioning = true;
                resetTriggers();
                return;
            }
        }

        resetTriggers();
    }

    AnimationPose AnimationStateMachine::evaluate() const {
        auto currentIt = m_States.find(m_CurrentState);
        if (currentIt == m_States.end()) return {};

        AnimationPose currentPose = evaluateState(currentIt->second, m_CurrentTime);

        if (m_IsTransitioning) {
            auto nextIt = m_States.find(m_NextState);
            if (nextIt != m_States.end()) {
                AnimationPose nextPose = evaluateState(nextIt->second, 0.0f);
                return AnimationPose::blend(currentPose, nextPose, m_TransitionProgress);
            }
        }

        return currentPose;
    }

    AnimationPose AnimationStateMachine::evaluateState(const AnimationState& state, float time) const {
        AnimationPose pose;
        if (!state.Clip) return pose;

        // Evaluate the clip at the given time to get bone transforms
        // This creates a pose from the clip's keyframe data
        auto& clip = state.Clip;
        // Simplified: create a pose with identity transforms
        // In a full implementation, this would sample the clip's channels
        pose.BoneTransforms.resize(1);
        pose.BoneTransforms[0].Position = glm::vec3(0.0f);
        pose.BoneTransforms[0].Rotation = glm::quat(1, 0, 0, 0);
        pose.BoneTransforms[0].Scale = glm::vec3(1.0f);

        return pose;
    }

    bool AnimationStateMachine::evaluateConditions(const std::vector<TransitionCondition>& conditions) const {
        for (const auto& cond : conditions) {
            // Check triggers
            auto triggerIt = m_Triggers.find(cond.ParameterName);
            if (triggerIt != m_Triggers.end()) {
                if (!triggerIt->second) return false;
                continue;
            }

            auto paramIt = m_Parameters.find(cond.ParameterName);
            if (paramIt == m_Parameters.end()) return false;

            bool condMet = false;

            if (std::holds_alternative<float>(paramIt->second) && std::holds_alternative<float>(cond.Value)) {
                float param = std::get<float>(paramIt->second);
                float val = std::get<float>(cond.Value);
                switch (cond.Op) {
                    case TransitionCondition::CompareOp::Greater: condMet = param > val; break;
                    case TransitionCondition::CompareOp::Less: condMet = param < val; break;
                    case TransitionCondition::CompareOp::Equal: condMet = std::abs(param - val) < 0.001f; break;
                    case TransitionCondition::CompareOp::NotEqual: condMet = std::abs(param - val) >= 0.001f; break;
                }
            } else if (std::holds_alternative<int>(paramIt->second) && std::holds_alternative<int>(cond.Value)) {
                int param = std::get<int>(paramIt->second);
                int val = std::get<int>(cond.Value);
                switch (cond.Op) {
                    case TransitionCondition::CompareOp::Greater: condMet = param > val; break;
                    case TransitionCondition::CompareOp::Less: condMet = param < val; break;
                    case TransitionCondition::CompareOp::Equal: condMet = param == val; break;
                    case TransitionCondition::CompareOp::NotEqual: condMet = param != val; break;
                }
            } else if (std::holds_alternative<bool>(paramIt->second) && std::holds_alternative<bool>(cond.Value)) {
                bool param = std::get<bool>(paramIt->second);
                bool val = std::get<bool>(cond.Value);
                condMet = (cond.Op == TransitionCondition::CompareOp::Equal) ? (param == val) : (param != val);
            }

            if (!condMet) return false;
        }
        return true;
    }

    void AnimationStateMachine::resetTriggers() {
        for (auto& [name, triggered] : m_Triggers) {
            triggered = false;
        }
    }

    // =====================================================================
    // Blend Nodes
    // =====================================================================

    AnimationPose ClipBlendNode::evaluate(float time, const std::unordered_map<std::string, AnimationParameter>&) const {
        AnimationPose pose;
        if (!Clip) return pose;
        // Same simplified evaluation as state machine
        pose.BoneTransforms.resize(1);
        return pose;
    }

    AnimationPose Blend1DNode::evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const {
        if (Entries.empty()) return {};

        float paramValue = 0.0f;
        auto it = params.find(ParameterName);
        if (it != params.end() && std::holds_alternative<float>(it->second)) {
            paramValue = std::get<float>(it->second);
        }

        // Find the two entries to blend between
        if (Entries.size() == 1) return Entries[0].Node->evaluate(time, params);

        // Clamp to range
        if (paramValue <= Entries.front().Threshold) return Entries.front().Node->evaluate(time, params);
        if (paramValue >= Entries.back().Threshold) return Entries.back().Node->evaluate(time, params);

        // Find surrounding entries
        for (size_t i = 0; i < Entries.size() - 1; i++) {
            if (paramValue >= Entries[i].Threshold && paramValue <= Entries[i + 1].Threshold) {
                float range = Entries[i + 1].Threshold - Entries[i].Threshold;
                float weight = (range > 0.0f) ? (paramValue - Entries[i].Threshold) / range : 0.0f;

                AnimationPose poseA = Entries[i].Node->evaluate(time, params);
                AnimationPose poseB = Entries[i + 1].Node->evaluate(time, params);
                return AnimationPose::blend(poseA, poseB, weight);
            }
        }

        return Entries.front().Node->evaluate(time, params);
    }

    AnimationPose Blend2DNode::evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const {
        if (Entries.empty()) return {};
        if (Entries.size() == 1) return Entries[0].Node->evaluate(time, params);

        glm::vec2 paramPos(0.0f);
        auto itX = params.find(ParameterNameX);
        if (itX != params.end() && std::holds_alternative<float>(itX->second))
            paramPos.x = std::get<float>(itX->second);
        auto itY = params.find(ParameterNameY);
        if (itY != params.end() && std::holds_alternative<float>(itY->second))
            paramPos.y = std::get<float>(itY->second);

        // Simple inverse-distance weighted blend
        float totalWeight = 0.0f;
        std::vector<float> weights(Entries.size(), 0.0f);

        for (size_t i = 0; i < Entries.size(); i++) {
            float dist = glm::distance(paramPos, Entries[i].Position);
            if (dist < 0.001f) {
                return Entries[i].Node->evaluate(time, params);
            }
            weights[i] = 1.0f / (dist * dist);
            totalWeight += weights[i];
        }

        // Normalize weights and blend
        AnimationPose result = Entries[0].Node->evaluate(time, params);
        for (size_t i = 0; i < result.BoneTransforms.size(); i++) {
            result.BoneTransforms[i].Position = glm::vec3(0.0f);
            result.BoneTransforms[i].Scale = glm::vec3(0.0f);
        }

        for (size_t e = 0; e < Entries.size(); e++) {
            float w = weights[e] / totalWeight;
            AnimationPose entryPose = Entries[e].Node->evaluate(time, params);
            for (size_t i = 0; i < std::min(result.BoneTransforms.size(), entryPose.BoneTransforms.size()); i++) {
                result.BoneTransforms[i].Position += entryPose.BoneTransforms[i].Position * w;
                result.BoneTransforms[i].Scale += entryPose.BoneTransforms[i].Scale * w;
            }
        }

        return result;
    }

    // =====================================================================
    // Animation Layer
    // =====================================================================

    void AnimationLayer::update(float dt) {
        if (StateMachine) StateMachine->update(dt);
    }

    AnimationPose AnimationLayer::evaluate() const {
        if (StateMachine) return StateMachine->evaluate();
        return {};
    }

}
