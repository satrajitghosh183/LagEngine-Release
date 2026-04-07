#pragma once

#include "../Core/Base.hpp"
#include "Bone.hpp"
#include <vector>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Skeleton - collection of bones
     */
    class Skeleton {
    public:
        Skeleton() = default;
        
        /**
         * @brief Add bone
         */
        void AddBone(const Bone& bone);
        
        /**
         * @brief Get bone by ID
         */
        Bone& GetBone(int id) { return m_Bones[id]; }
        const Bone& GetBone(int id) const { return m_Bones[id]; }
        
        /**
         * @brief Get bone by name
         */
        Bone* GetBone(const std::string& name);
        
        /**
         * @brief Get all bones
         */
        std::vector<Bone>& GetBones() { return m_Bones; }
        const std::vector<Bone>& GetBones() const { return m_Bones; }
        
        /**
         * @brief Get bone count
         */
        size_t GetBoneCount() const { return m_Bones.size(); }
        
        /**
         * @brief Get root bone
         */
        int GetRootBone() const { return m_RootBone; }
        void SetRootBone(int id) { m_RootBone = id; }
        
    private:
        std::vector<Bone> m_Bones;
        std::unordered_map<std::string, int> m_BoneMap;
        int m_RootBone = 0;
    };
}