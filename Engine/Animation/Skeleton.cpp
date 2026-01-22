#include "Skeleton.hpp"

namespace GameEngine {

    void Skeleton::AddBone(const Bone& bone) {
        m_BoneMap[bone.Name] = static_cast<int>(m_Bones.size());
        m_Bones.push_back(bone);
    }

    Bone* Skeleton::GetBone(const std::string& name) {
        auto it = m_BoneMap.find(name);
        if (it != m_BoneMap.end()) {
            return &m_Bones[it->second];
        }
        return nullptr;
    }

}
