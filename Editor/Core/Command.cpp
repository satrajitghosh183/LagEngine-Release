#include "Command.hpp"

namespace GameEngine {

    CreateEntityCommand::CreateEntityCommand(const Ref<Scene>& scene, const std::string& name)
        : m_Scene(scene), m_Name(name) {}

    void CreateEntityCommand::Execute() {
        if (!m_Scene) return;
        Entity e = m_Scene->CreateEntity(m_Name);
        m_EntityUUID = e.GetUUID();
    }

    void CreateEntityCommand::Undo() {
        if (!m_Scene || m_EntityUUID == UUID(0)) return;
        Entity e = m_Scene->GetEntityByUUID(m_EntityUUID);
        if (e.IsValid()) m_Scene->DestroyEntity(e);
    }

    DeleteEntityCommand::DeleteEntityCommand(const Ref<Scene>& scene, UUID entityUUID)
        : m_Scene(scene), m_EntityUUID(entityUUID) {}

    void DeleteEntityCommand::Execute() {
        if (!m_Scene || m_EntityUUID == UUID(0)) return;
        Entity e = m_Scene->GetEntityByUUID(m_EntityUUID);
        if (e.IsValid()) {
            m_SavedName = e.GetName();
            m_Scene->DestroyEntity(e);
            m_Executed = true;
        }
    }

    void DeleteEntityCommand::Undo() {
        if (!m_Scene || !m_Executed) return;
        m_Scene->CreateEntityWithUUID(m_EntityUUID, m_SavedName);
    }

    ReparentEntityCommand::ReparentEntityCommand(const Ref<Scene>& scene, UUID entityUUID, UUID oldParentUUID, UUID newParentUUID)
        : m_Scene(scene), m_EntityUUID(entityUUID), m_OldParentUUID(oldParentUUID), m_NewParentUUID(newParentUUID) {}

    void ReparentEntityCommand::Execute() {
        if (!m_Scene) return;
        Entity child = m_Scene->GetEntityByUUID(m_EntityUUID);
        Entity newParent = m_Scene->GetEntityByUUID(m_NewParentUUID);
        if (child.IsValid() && newParent.IsValid()) child.SetParent(newParent);
    }

    void ReparentEntityCommand::Undo() {
        if (!m_Scene) return;
        Entity child = m_Scene->GetEntityByUUID(m_EntityUUID);
        Entity oldParent = m_OldParentUUID != UUID(0) ? m_Scene->GetEntityByUUID(m_OldParentUUID) : Entity();
        if (child.IsValid()) child.SetParent(oldParent);
    }

    CommandHistory::CommandHistory()
        : m_CurrentIndex(0) {
    }

    void CommandHistory::Execute(Scope<Command> cmd) {
        // Remove any commands after current index (when we execute after undo)
        m_History.erase(m_History.begin() + m_CurrentIndex, m_History.end());

        // Try to merge with last command
        if (!m_History.empty() && m_History.back()->Merge(*cmd)) {
            // Merged, don't add new command
            return;
        }

        // Add new command
        cmd->Execute();
        m_History.push_back(std::move(cmd));
        m_CurrentIndex = m_History.size();

        // Limit history size
        if (m_History.size() > MAX_HISTORY) {
            m_History.erase(m_History.begin());
            m_CurrentIndex--;
        }
    }

    void CommandHistory::Undo() {
        if (!CanUndo()) {
            return;
        }

        m_CurrentIndex--;
        m_History[m_CurrentIndex]->Undo();
    }

    void CommandHistory::Redo() {
        if (!CanRedo()) {
            return;
        }

        m_History[m_CurrentIndex]->Execute();
        m_CurrentIndex++;
    }

    bool CommandHistory::CanUndo() const {
        return m_CurrentIndex > 0;
    }

    bool CommandHistory::CanRedo() const {
        return m_CurrentIndex < m_History.size();
    }

    void CommandHistory::Clear() {
        m_History.clear();
        m_CurrentIndex = 0;
    }

    std::string CommandHistory::GetUndoDescription() const {
        if (!CanUndo()) {
            return "";
        }
        return m_History[m_CurrentIndex - 1]->GetDescription();
    }

    std::string CommandHistory::GetRedoDescription() const {
        if (!CanRedo()) {
            return "";
        }
        return m_History[m_CurrentIndex]->GetDescription();
    }

}

