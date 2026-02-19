#pragma once

#include "../../Engine/Platform/Input.hpp"
#include <string>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Editor action types
     */
    enum class EditorActions {
        // Camera movement
        MOVE_CAMERA_FORWARD,
        MOVE_CAMERA_BACKWARD,
        MOVE_CAMERA_LEFT,
        MOVE_CAMERA_RIGHT,
        MOVE_CAMERA_UP,
        MOVE_CAMERA_DOWN,

        // Entity actions
        DUPLICATE_ENTITY,
        DELETE_ENTITY,

        // Selection actions
        SELECT_ALL_ENTITIES,
        DESELECT_ALL_ENTITIES,
        FOCUS_ON_SELECTION,
        RENAME_SELECTED,

        // Edit actions
        UNDO_ACTION,
        REDO_ACTION,
        COPY_ACTION,
        CUT_ACTION,
        PASTE_ACTION,
        DELETE_ACTION,

        // Transform toggles
        MOVE_TOGGLE_ACTION,
        ROTATE_TOGGLE_ACTION,
        SCALE_TOGGLE_ACTION,
        LOCAL_GLOBAL_TOGGLE_ACTION,

        // Scene actions
        SAVE_SCENE,
        SAVE_SCENE_AS,
        NEW_SCENE,
        OPEN_SCENE,

        // View actions
        WIREFRAME_MODE,
        ORTHO_CAMERA,
        INSPECTOR_MODE,

        // Screenshot actions
        SCREENSHOT,
        SCREENSHOT_ALPHA,

        COUNT
    };

    /**
     * @brief Hotkey information structure
     */
    struct HotkeyInfo {
        KeyCode m_Key = KeyCode::None;
        bool m_Press = false;      // true = just pressed, false = held down
        bool m_Control = false;
        bool m_Shift = false;
    };

    /**
     * @brief Hotkey system for editor
     */
    class HotkeyManager {
    public:
        HotkeyManager();
        ~HotkeyManager() = default;

        /**
         * @brief Check if an action's hotkey is currently active
         */
        bool CheckInput(EditorActions action) const;

        /**
         * @brief Get string representation of hotkey for an action
         */
        std::string GetInputString(EditorActions action) const;

        /**
         * @brief Set hotkey for an action
         */
        void SetHotkey(EditorActions action, const HotkeyInfo& info);

        /**
         * @brief Get hotkey for an action
         */
        const HotkeyInfo& GetHotkey(EditorActions action) const;

        /**
         * @brief Load hotkeys from config
         */
        void LoadFromConfig(const std::string& configPath);

        /**
         * @brief Save hotkeys to config
         */
        void SaveToConfig(const std::string& configPath) const;

        /**
         * @brief Get default hotkeys
         */
        static std::array<HotkeyInfo, static_cast<size_t>(EditorActions::COUNT)> GetDefaultHotkeys();

    private:
        std::array<HotkeyInfo, static_cast<size_t>(EditorActions::COUNT)> m_HotkeyActions;
    };

}
