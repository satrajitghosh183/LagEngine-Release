
#include "Animator.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    Animator::Animator(const Ref<Skeleton>& skeleton)
        : m_Skeleton(skeleton), m_CurrentTime(0.0f) {
        
        // Initialize bone matrices
        if (m_Skeleton) {
            m_FinalBoneMatrices.resize(m_Skeleton->GetBoneCount(), glm::mat4(1.0f));
        }
    }

    void Animator::Update(float deltaTime) {
        if (!IsPlaying || !m_CurrentAnimation || !m_Skeleton) {
            return;
        }
        
        // Update time
        float duration = m_CurrentAnimation->GetDuration();
        float ticksPerSecond = m_CurrentAnimation->GetTicksPerSecond();
        
        m_CurrentTime += deltaTime * ticksPerSecond * PlaybackSpeed;
        
        // Handle looping
        if (m_CurrentTime > duration) {
            if (IsLooping) {
                m_CurrentTime = fmod(m_CurrentTime, duration);
            } else {
                m_CurrentTime = duration;
                IsPlaying = false;
            }
        }
        
        // Calculate bone transforms
        CalculateBoneTransforms(m_CurrentTime);
    }

    void Animator::Play(const std::string& animationName, bool loop) {
        auto it = m_Animations.find(animationName);
        if (it != m_Animations.end()) {
            m_CurrentAnimation = it->second;
            m_CurrentTime = 0.0f;
            IsPlaying = true;
            IsLooping = loop;
        }
    }

    void Animator::Stop() {
        IsPlaying = false;
        m_CurrentTime = 0.0f;
    }

    void Animator::Pause() {
        IsPlaying = false;
    }

    void Animator::Resume() {
        if (m_CurrentAnimation) {
            IsPlaying = true;
        }
    }

    void Animator::AddAnimation(const Ref<AnimationClip>& clip) {
        if (clip) {
            m_Animations[clip->GetName()] = clip;
        }
    }

    void Animator::CalculateBoneTransforms(float time) {
        if (!m_CurrentAnimation || !m_Skeleton) {
            return;
        }
        
        // Get transforms from animation channels
        std::vector<glm::mat4> localTransforms(m_Skeleton->GetBoneCount(), glm::mat4(1.0f));
        
        for (const auto& channel : m_CurrentAnimation->GetChannels()) {
            if (channel.BoneID >= 0 && channel.BoneID < static_cast<int>(m_Skeleton->GetBoneCount())) {
                glm::vec3 position, scale;
                glm::quat rotation;
                
                channel.GetTransformAtTime(time, position, rotation, scale);
                
                // Build transform matrix
                glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
                glm::mat4 rotationMatrix = glm::mat4_cast(rotation);
                glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
                
                localTransforms[channel.BoneID] = translationMatrix * rotationMatrix * scaleMatrix;
            }
        }
        
        // Update bone local transforms
        auto& bones = m_Skeleton->GetBones();
        for (size_t i = 0; i < bones.size(); i++) {
            bones[i].LocalTransform = localTransforms[i];
        }
        
        // Calculate global transforms starting from root
        int rootBone = m_Skeleton->GetRootBone();
        UpdateBoneHierarchy(rootBone, glm::mat4(1.0f));
        
        // Calculate final matrices (global transform * offset matrix)
        for (size_t i = 0; i < bones.size(); i++) {
            m_FinalBoneMatrices[i] = bones[i].GlobalTransform * bones[i].OffsetMatrix;
        }
    }

    void Animator::UpdateBoneHierarchy(int boneIndex, const glm::mat4& parentTransform) {
        if (boneIndex < 0 || boneIndex >= static_cast<int>(m_Skeleton->GetBoneCount())) {
            return;
        }
        
        auto& bone = m_Skeleton->GetBone(boneIndex);
        
        // Calculate global transform
        bone.GlobalTransform = parentTransform * bone.LocalTransform;
        
        // Update children
        for (int childIndex : bone.Children) {
            UpdateBoneHierarchy(childIndex, bone.GlobalTransform);
        }
    }

}
