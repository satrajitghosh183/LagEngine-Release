#pragma once

#include "../Core/Base.hpp"
#include "AnimationClip.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <variant>

namespace GameEngine {

    // A pose is a snapshot of all bone transforms at a point in time
    struct AnimationPose {
        struct BoneTransform {
            glm::vec3 Position{0.0f};
            glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};
            glm::vec3 Scale{1.0f};
        };
        std::vector<BoneTransform> BoneTransforms;

        static AnimationPose blend(const AnimationPose& a, const AnimationPose& b, float weight);
        static AnimationPose additive(const AnimationPose& base, const AnimationPose& additive, float weight);
    };

    // Animation event triggered at a specific time in a clip
    struct AnimationEvent {
        std::string Name;
        float Time = 0.0f; // Normalized [0,1]
        std::string StringData;
        float FloatData = 0.0f;
        int IntData = 0;
    };

    // Condition for a state transition
    struct TransitionCondition {
        enum class CompareOp { Greater, Less, Equal, NotEqual };
        std::string ParameterName;
        CompareOp Op = CompareOp::Equal;
        std::variant<float, int, bool> Value;
    };

    // Transition between two states
    struct AnimationTransition {
        std::string FromState;
        std::string ToState;
        float Duration = 0.25f; // Blend time in seconds
        bool HasExitTime = false;
        float ExitTime = 1.0f; // Normalized time to auto-transition
        std::vector<TransitionCondition> Conditions;
    };

    // A state in the state machine
    struct AnimationState {
        std::string Name;
        Ref<AnimationClip> Clip;
        float Speed = 1.0f;
        bool Loop = true;
        std::vector<AnimationEvent> Events;
    };

    // Parameter types
    using AnimationParameter = std::variant<float, int, bool>;

    class AnimationStateMachine {
    public:
        AnimationStateMachine() = default;

        // States
        void addState(const std::string& name, Ref<AnimationClip> clip, float speed = 1.0f, bool loop = true);
        void removeState(const std::string& name);
        void setEntryState(const std::string& name);
        const AnimationState* getCurrentState() const;
        const std::string& getCurrentStateName() const { return m_CurrentState; }
        bool isInTransition() const { return m_IsTransitioning; }
        float getTransitionProgress() const { return m_TransitionProgress; }

        // Transitions
        void addTransition(const AnimationTransition& transition);
        void addAnyStateTransition(const std::string& toState, const std::vector<TransitionCondition>& conditions,
                                   float duration = 0.25f);

        // Parameters
        void addFloat(const std::string& name, float defaultValue = 0.0f);
        void addInt(const std::string& name, int defaultValue = 0);
        void addBool(const std::string& name, bool defaultValue = false);
        void addTrigger(const std::string& name);

        void setFloat(const std::string& name, float value);
        void setInt(const std::string& name, int value);
        void setBool(const std::string& name, bool value);
        void setTrigger(const std::string& name);

        float getFloat(const std::string& name) const;
        int getInt(const std::string& name) const;
        bool getBool(const std::string& name) const;

        // Update
        void update(float dt);

        // Get current pose (blended if transitioning)
        AnimationPose evaluate() const;

        // Events
        void setEventCallback(std::function<void(const AnimationEvent&)> callback) { m_EventCallback = callback; }

        // Accessors
        const std::unordered_map<std::string, AnimationState>& getStates() const { return m_States; }
        const std::vector<AnimationTransition>& getTransitions() const { return m_Transitions; }
        const std::unordered_map<std::string, AnimationParameter>& getParameters() const { return m_Parameters; }

    private:
        bool evaluateConditions(const std::vector<TransitionCondition>& conditions) const;
        void resetTriggers();
        AnimationPose evaluateState(const AnimationState& state, float time) const;

        std::unordered_map<std::string, AnimationState> m_States;
        std::vector<AnimationTransition> m_Transitions;
        std::vector<AnimationTransition> m_AnyStateTransitions;
        std::unordered_map<std::string, AnimationParameter> m_Parameters;
        std::unordered_map<std::string, bool> m_Triggers;

        std::string m_CurrentState;
        std::string m_NextState;
        float m_CurrentTime = 0.0f;
        float m_PreviousTime = 0.0f;

        bool m_IsTransitioning = false;
        float m_TransitionDuration = 0.0f;
        float m_TransitionProgress = 0.0f;

        std::function<void(const AnimationEvent&)> m_EventCallback;
    };

    // =====================================================================
    // Blend Tree
    // =====================================================================

    class BlendNode {
    public:
        virtual ~BlendNode() = default;
        virtual AnimationPose evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const = 0;
    };

    class ClipBlendNode : public BlendNode {
    public:
        Ref<AnimationClip> Clip;
        float Speed = 1.0f;

        AnimationPose evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const override;
    };

    class Blend1DNode : public BlendNode {
    public:
        struct BlendEntry {
            Ref<BlendNode> Node;
            float Threshold = 0.0f;
        };

        std::string ParameterName;
        std::vector<BlendEntry> Entries; // Sorted by threshold

        AnimationPose evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const override;
    };

    class Blend2DNode : public BlendNode {
    public:
        struct BlendEntry2D {
            Ref<BlendNode> Node;
            glm::vec2 Position{0.0f};
        };

        std::string ParameterNameX;
        std::string ParameterNameY;
        std::vector<BlendEntry2D> Entries;

        AnimationPose evaluate(float time, const std::unordered_map<std::string, AnimationParameter>& params) const override;
    };

    // =====================================================================
    // Animation Layer
    // =====================================================================

    class AnimationLayer {
    public:
        enum class BlendMode { Override, Additive };

        std::string Name = "Base Layer";
        BlendMode Mode = BlendMode::Override;
        float Weight = 1.0f;
        std::vector<bool> BoneMask; // If empty, affects all bones

        Scope<AnimationStateMachine> StateMachine;

        void update(float dt);
        AnimationPose evaluate() const;
    };

}
