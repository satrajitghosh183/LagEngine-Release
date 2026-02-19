#pragma once

#include "../../Engine/Core/Base.hpp"
#include "../../Engine/Core/UUID.hpp"
#include "../../Engine/Scene/Scene.hpp"
#include "../../Engine/Scene/Entity.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <string>

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

    /**
     * @brief Generic property-set command (stores raw pointer - can dangle if entity/component destroyed).
     * Prefer SetPropertyCallbackCommand or SetEntityPropertyCommand for safety.
     */
    template<typename T>
    class SetPropertyCommand : public Command {
    public:
        SetPropertyCommand(T* target, const T& oldValue, const T& newValue, const std::string& desc = "Set Property")
            : m_Target(target), m_OldValue(oldValue), m_NewValue(newValue), m_Description(desc) {}

        void Execute() override { if (m_Target) *m_Target = m_NewValue; }
        void Undo() override    { if (m_Target) *m_Target = m_OldValue; }
        std::string GetDescription() const override { return m_Description; }

    private:
        T* m_Target;
        T m_OldValue;
        T m_NewValue;
        std::string m_Description;
    };

    /**
     * @brief Create entity command (undo = delete entity)
     */
    class CreateEntityCommand : public Command {
    public:
        CreateEntityCommand(const Ref<Scene>& scene, const std::string& name = "Entity");
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override { return "Create Entity"; }
        UUID GetCreatedUUID() const { return m_EntityUUID; }
    private:
        Ref<Scene> m_Scene;
        std::string m_Name;
        UUID m_EntityUUID = 0;
    };

    /**
     * @brief Delete entity command (undo = recreate entity with same UUID and name)
     */
    class DeleteEntityCommand : public Command {
    public:
        DeleteEntityCommand(const Ref<Scene>& scene, UUID entityUUID);
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override { return "Delete Entity"; }
    private:
        Ref<Scene> m_Scene;
        UUID m_EntityUUID;
        std::string m_SavedName;
        bool m_Executed = false;
    };

    /**
     * @brief Reparent entity command
     */
    class ReparentEntityCommand : public Command {
    public:
        ReparentEntityCommand(const Ref<Scene>& scene, UUID entityUUID, UUID oldParentUUID, UUID newParentUUID);
        void Execute() override;
        void Undo() override;
        std::string GetDescription() const override { return "Reparent Entity"; }
    private:
        Ref<Scene> m_Scene;
        UUID m_EntityUUID;
        UUID m_OldParentUUID;
        UUID m_NewParentUUID;
    };

    /**
     * @brief Property command using getter/setter callbacks (safer for invalidation)
     */
    template<typename T>
    class SetPropertyCallbackCommand : public Command {
    public:
        SetPropertyCallbackCommand(
            std::function<void(const T&)> setter,
            const T& oldValue,
            const T& newValue,
            const std::string& desc = "Set Property")
            : m_Setter(std::move(setter)), m_OldValue(oldValue), m_NewValue(newValue), m_Description(desc) {}

        void Execute() override { m_Setter(m_NewValue); }
        void Undo() override    { m_Setter(m_OldValue); }
        std::string GetDescription() const override { return m_Description; }

    private:
        std::function<void(const T&)> m_Setter;
        T m_OldValue;
        T m_NewValue;
        std::string m_Description;
    };

}

