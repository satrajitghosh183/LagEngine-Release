#include "Command.hpp"

namespace GameEngine {

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

