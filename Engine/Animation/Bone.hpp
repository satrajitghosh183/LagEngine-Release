#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace GameEngine {

    /**
     * @brief Skeletal bone
     */
    struct Bone {
        std::string Name;
        int ID;
        glm::mat4 OffsetMatrix;      // Bind pose transform
        glm::mat4 LocalTransform;    // Current local transform
        glm::mat4 GlobalTransform;   // Current global transform
        
        int ParentID = -1;
        std::vector<int> Children;
    };

    /**
     * @brief Bone keyframe
     */
    struct KeyFrame {
        float Time;
        glm::vec3 Position;
        glm::quat Rotation;
        glm::vec3 Scale;
    };

    /**
     * @brief Animation channel (bone animation data)
     */
    struct AnimationChannel {
        int BoneID;
        std::vector<KeyFrame> KeyFrames;
        
        /**
         * @brief Interpolate transform at time
         */
        void GetTransformAtTime(float time, glm::vec3& position, 
                               glm::quat& rotation, glm::vec3& scale) const;
        
    private:
        int FindKeyFrame(float time) const;
    };
}