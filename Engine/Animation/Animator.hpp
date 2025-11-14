#pragma once

#include "../Core/Base.hpp"
#include "AnimationClip.hpp"
#include "Skeleton.hpp"
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Animator - plays animations on skeleton
     */
    class Animator {
    public:
        Animator(const Ref<Skeleton>& skeleton);
        
        /**
         * @brief Update animation
         */
        void Update(float deltaTime);
        
        /**
         * @brief Play animation
         */
        void Play(const std::string& animationName, bool loop = true);
        
        /**
         * @brief Stop animation
         */
        void Stop();
        
        /**
         * @brief Pause animation
         */
        void Pause();
        
        /**
         * @brief Resume animation
         */
        void Resume();
        
        /**
         * @brief Add animation clip
         */
        void AddAnimation(const Ref<AnimationClip>& clip);
        
        /**
         * @brief Get final bone transforms
         */
        const std::vector<glm::mat4>& GetFinalBoneMatrices() const { return m_FinalBoneMatrices; }
        
        /**
         * @brief Animation properties
         */
        float PlaybackSpeed = 1.0f;
        bool IsPlaying = false;
        bool IsLooping = true;
        
    private:
        void CalculateBoneTransforms(float time);
        void UpdateBoneHierarchy(int boneIndex, const glm::mat4& parentTransform);
        
    private:
        Ref<Skeleton> m_Skeleton;
        std::unordered_map<std::string, Ref<AnimationClip>> m_Animations;
        
        Ref<AnimationClip> m_CurrentAnimation;
        float m_CurrentTime;
        
        std::vector<glm::mat4> m_FinalBoneMatrices;
    };
}