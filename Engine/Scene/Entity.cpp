#include "Entity.hpp"
#include "Scene.hpp"
#include "../Core/Logger.hpp"

namespace GameEngine {

    Entity::Entity(UUID id, Scene* scene)
        : m_UUID(id), m_Scene(scene) {
    }

    bool Entity::IsValid() const {
        return m_Scene != nullptr && m_UUID != UUID(0) && m_Scene->HasEntity(m_UUID);
    }

    const std::string& Entity::GetName() const {
        if (!IsValid()) {
            static std::string empty = "";
            return empty;
        }
        return m_Scene->GetEntityData(m_UUID).Name;
    }

    void Entity::SetName(const std::string& name) {
        if (IsValid()) {
            m_Scene->UpdateEntityNameIndex(m_UUID, name);
        }
    }

    const std::string& Entity::GetTag() const {
        if (!IsValid()) {
            static std::string empty = "";
            return empty;
        }
        return m_Scene->GetEntityData(m_UUID).Tag;
    }

    void Entity::SetTag(const std::string& tag) {
        if (IsValid()) {
            m_Scene->GetEntityData(m_UUID).Tag = tag;
        }
    }

    bool Entity::IsActive() const {
        if (!IsValid()) return false;
        return m_Scene->GetEntityData(m_UUID).Active;
    }

    void Entity::SetActive(bool active) {
        if (IsValid()) {
            m_Scene->GetEntityData(m_UUID).Active = active;
        }
    }

    void Entity::SetParent(Entity parent) {
        if (!IsValid()) return;
        auto& entityData = m_Scene->GetEntityData(m_UUID);

        if (!parent.IsValid()) {
            // Clear parent
            if (entityData.Parent != UUID(0)) {
                auto& oldParentData = m_Scene->GetEntityData(entityData.Parent);
                auto it = std::find(oldParentData.Children.begin(), oldParentData.Children.end(), m_UUID);
                if (it != oldParentData.Children.end()) oldParentData.Children.erase(it);
            }
            entityData.Parent = UUID(0);
            return;
        }
        if (parent.GetUUID() == m_UUID) return; // self-parent is no-op

        // Cycle detection: if proposed parent is a descendant of this, we would create a cycle
        Entity ancestor = parent;
        while (ancestor.IsValid()) {
            if (ancestor.GetUUID() == m_UUID) {
                GE_CORE_WARN("Entity::SetParent: Would create cycle in hierarchy (parent is descendant of this entity). Ignored.");
                return;
            }
            ancestor = ancestor.GetParent();
        }

        auto& parentData = m_Scene->GetEntityData(parent.GetUUID());

        // Remove from old parent
        if (entityData.Parent != UUID(0)) {
            auto& oldParentData = m_Scene->GetEntityData(entityData.Parent);
            auto it = std::find(oldParentData.Children.begin(), oldParentData.Children.end(), m_UUID);
            if (it != oldParentData.Children.end()) {
                oldParentData.Children.erase(it);
            }
        }
        
        // Set new parent
        entityData.Parent = parent.GetUUID();
        parentData.Children.push_back(m_UUID);
    }

    Entity Entity::GetParent() const {
        if (!IsValid()) return Entity();
        
        auto& entityData = m_Scene->GetEntityData(m_UUID);
        if (entityData.Parent != UUID(0)) {
            return Entity(entityData.Parent, m_Scene);
        }
        
        return Entity();
    }

    void Entity::AddChild(Entity child) {
        if (child.IsValid()) {
            child.SetParent(*this);
        }
    }

    void Entity::RemoveChild(Entity child) {
        if (!IsValid() || !child.IsValid()) return;
        
        auto& entityData = m_Scene->GetEntityData(m_UUID);
        auto& childData = m_Scene->GetEntityData(child.GetUUID());
        
        if (childData.Parent == m_UUID) {
            childData.Parent = 0;
            
            auto it = std::find(entityData.Children.begin(), entityData.Children.end(), child.GetUUID());
            if (it != entityData.Children.end()) {
                entityData.Children.erase(it);
            }
        }
    }

    std::vector<Entity> Entity::GetChildren() const {
        if (!IsValid()) return {};

        auto& entityData = m_Scene->GetEntityData(m_UUID);

        std::vector<Entity> children;
        children.reserve(entityData.Children.size());

        for (UUID childID : entityData.Children) {
            children.emplace_back(childID, m_Scene);
        }

        return children;
    }

    bool Entity::HasParent() const {
        if (!IsValid()) return false;
        return m_Scene->GetEntityData(m_UUID).Parent != UUID(0);
    }

    void Entity::Destroy() {
        if (IsValid()) {
            m_Scene->DestroyEntity(*this);
        }
    }

    std::vector<Component*> Entity::GetAllComponents() {
        if (!IsValid()) return {};
        return m_Scene->GetAllComponents(m_UUID);
    }
}