#include "Bone.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace GameEngine {

    void AnimationChannel::GetTransformAtTime(float time, glm::vec3& position, 
                                               glm::quat& rotation, glm::vec3& scale) const {
        if (KeyFrames.empty()) {
            position = glm::vec3(0.0f);
            rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            scale = glm::vec3(1.0f);
            return;
        }
        
        if (KeyFrames.size() == 1) {
            position = KeyFrames[0].Position;
            rotation = KeyFrames[0].Rotation;
            scale = KeyFrames[0].Scale;
            return;
        }
        
        // Find keyframe
        int keyIndex = FindKeyFrame(time);
        int nextKeyIndex = keyIndex + 1;
        
        if (nextKeyIndex >= static_cast<int>(KeyFrames.size())) {
            // Use last keyframe
            position = KeyFrames.back().Position;
            rotation = KeyFrames.back().Rotation;
            scale = KeyFrames.back().Scale;
            return;
        }
        
        // Interpolate between keyframes
        const KeyFrame& key0 = KeyFrames[keyIndex];
        const KeyFrame& key1 = KeyFrames[nextKeyIndex];
        
        float deltaTime = key1.Time - key0.Time;
        float factor = (time - key0.Time) / deltaTime;
        factor = glm::clamp(factor, 0.0f, 1.0f);
        
        // Linear interpolation for position and scale
        position = glm::mix(key0.Position, key1.Position, factor);
        scale = glm::mix(key0.Scale, key1.Scale, factor);
        
        // Spherical linear interpolation for rotation
        rotation = glm::slerp(key0.Rotation, key1.Rotation, factor);
    }

    int AnimationChannel::FindKeyFrame(float time) const {
        for (size_t i = 0; i < KeyFrames.size() - 1; i++) {
            if (time < KeyFrames[i + 1].Time) {
                return static_cast<int>(i);
            }
        }
        return static_cast<int>(KeyFrames.size() - 1);
    }

}
