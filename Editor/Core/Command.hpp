#pragma once

#include "../../Engine/Core/Base.hpp"
#include <memory>
#include <vector>

namespace GameEngine {

    /**
     * @brief Command base class for undo/redo
     */
    class Command {
    public:
        virtual ~Command() = default;

        /**
         * @brief Execute command
         */
        virtual void Execute() = 0;

        /**
         * @brief Undo command
         */
        virtual void Undo() = 0;

        /**
         * @brief Check if command can be merged with another
         */
        virtual bool Merge(const Command& other) const { return false; }

        /**
         * @brief Get command description
         */
        virtual std::string GetDescription() const { return "Command"; }
    };

    /**
     * @brief Command history for undo/redo
     */
    class CommandHistory {
    public:
        CommandHistory();
        ~CommandHistory() = default;

        /**
         * @brief Execute command and add to history
         */
        void Execute(Scope<Command> cmd);

        /**
         * @brief Undo last command
         */
        void Undo();

        /**
         * @brief Redo last undone command
         */
        void Redo();

        /**
         * @brief Check if undo is available
         */
        bool CanUndo() const;

        /**
         * @brief Check if redo is available
         */
        bool CanRedo() const;

        /**
         * @brief Clear history
         */
        void Clear();

        /**
         * @brief Get undo description
         */
        std::string GetUndoDescription() const;

        /**
         * @brief Get redo description
         */
        std::string GetRedoDescription() const;

    private:
        std::vector<Scope<Command>> m_History;
        size_t m_CurrentIndex;
        static constexpr size_t MAX_HISTORY = 100;
    };

}

