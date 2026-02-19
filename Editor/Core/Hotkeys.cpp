#include "Hotkeys.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Platform/Input.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace GameEngine {

    static std::string KeyCodeToString(KeyCode key) {
        switch (key) {
            case KeyCode::Space: return "Space";
            case KeyCode::Apostrophe: return "'";
            case KeyCode::Comma: return ",";
            case KeyCode::Minus: return "-";
            case KeyCode::Period: return ".";
            case KeyCode::Slash: return "/";
            case KeyCode::D0: return "0";
            case KeyCode::D1: return "1";
            case KeyCode::D2: return "2";
            case KeyCode::D3: return "3";
            case KeyCode::D4: return "4";
            case KeyCode::D5: return "5";
            case KeyCode::D6: return "6";
            case KeyCode::D7: return "7";
            case KeyCode::D8: return "8";
            case KeyCode::D9: return "9";
            case KeyCode::Semicolon: return ";";
            case KeyCode::Equal: return "=";
            case KeyCode::A: return "A";
            case KeyCode::B: return "B";
            case KeyCode::C: return "C";
            case KeyCode::D: return "D";
            case KeyCode::E: return "E";
            case KeyCode::F: return "F";
            case KeyCode::G: return "G";
            case KeyCode::H: return "H";
            case KeyCode::I: return "I";
            case KeyCode::J: return "J";
            case KeyCode::K: return "K";
            case KeyCode::L: return "L";
            case KeyCode::M: return "M";
            case KeyCode::N: return "N";
            case KeyCode::O: return "O";
            case KeyCode::P: return "P";
            case KeyCode::Q: return "Q";
            case KeyCode::R: return "R";
            case KeyCode::S: return "S";
            case KeyCode::T: return "T";
            case KeyCode::U: return "U";
            case KeyCode::V: return "V";
            case KeyCode::W: return "W";
            case KeyCode::X: return "X";
            case KeyCode::Y: return "Y";
            case KeyCode::Z: return "Z";
            case KeyCode::Escape: return "Esc";
            case KeyCode::Enter: return "Enter";
            case KeyCode::Tab: return "Tab";
            case KeyCode::Backspace: return "Backspace";
            case KeyCode::Insert: return "Insert";
            case KeyCode::Delete: return "Delete";
            case KeyCode::Right: return "Right";
            case KeyCode::Left: return "Left";
            case KeyCode::Down: return "Down";
            case KeyCode::Up: return "Up";
            case KeyCode::PageUp: return "PageUp";
            case KeyCode::PageDown: return "PageDown";
            case KeyCode::Home: return "Home";
            case KeyCode::End: return "End";
            case KeyCode::F1: return "F1";
            case KeyCode::F2: return "F2";
            case KeyCode::F3: return "F3";
            case KeyCode::F4: return "F4";
            case KeyCode::F5: return "F5";
            case KeyCode::F6: return "F6";
            case KeyCode::F7: return "F7";
            case KeyCode::F8: return "F8";
            case KeyCode::F9: return "F9";
            case KeyCode::F10: return "F10";
            case KeyCode::F11: return "F11";
            case KeyCode::F12: return "F12";
            default: return "Unknown";
        }
    }

    HotkeyManager::HotkeyManager() {
        m_HotkeyActions = GetDefaultHotkeys();
    }

    bool HotkeyManager::CheckInput(EditorActions action) const {
        const HotkeyInfo& hotkey = m_HotkeyActions[static_cast<size_t>(action)];
        bool ret = false;
        
        if (hotkey.m_Press) {
            ret = Input::IsKeyJustPressed(hotkey.m_Key);
        } else {
            ret = Input::IsKeyPressed(hotkey.m_Key);
        }
        
        if (hotkey.m_Control) {
            ret = ret && (Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl));
        }
        
        if (hotkey.m_Shift) {
            ret = ret && (Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift));
        }
        
        return ret;
    }

    std::string HotkeyManager::GetInputString(EditorActions action) const {
        const HotkeyInfo& hotkey = m_HotkeyActions[static_cast<size_t>(action)];
        std::string ret;
        
        if (hotkey.m_Control) {
            ret += "Ctrl + ";
        }
        if (hotkey.m_Shift) {
            ret += "Shift + ";
        }
        
        // Convert KeyCode to string
        std::string keyName = KeyCodeToString(hotkey.m_Key);
        ret += keyName;
        
        return ret;
    }

    void HotkeyManager::SetHotkey(EditorActions action, const HotkeyInfo& info) {
        m_HotkeyActions[static_cast<size_t>(action)] = info;
    }

    const HotkeyInfo& HotkeyManager::GetHotkey(EditorActions action) const {
        return m_HotkeyActions[static_cast<size_t>(action)];
    }

    void HotkeyManager::LoadFromConfig(const std::string& configPath) {
        std::ifstream f(configPath);
        if (!f.is_open()) return;
        try {
            nlohmann::json j;
            f >> j;
            for (size_t i = 0; i < static_cast<size_t>(EditorActions::COUNT); i++) {
                std::string key = std::to_string(i);
                if (j.contains(key) && j[key].is_object()) {
                    auto& o = j[key];
                    if (o.contains("key") && o["key"].is_number_integer())
                        m_HotkeyActions[i].m_Key = static_cast<KeyCode>(o["key"].get<int>());
                    if (o.contains("press") && o["press"].is_boolean())
                        m_HotkeyActions[i].m_Press = o["press"].get<bool>();
                    if (o.contains("control") && o["control"].is_boolean())
                        m_HotkeyActions[i].m_Control = o["control"].get<bool>();
                    if (o.contains("shift") && o["shift"].is_boolean())
                        m_HotkeyActions[i].m_Shift = o["shift"].get<bool>();
                }
            }
        } catch (const std::exception& e) {
            GE_CORE_WARN("Failed to load hotkey config: {0}", e.what());
        }
    }

    void HotkeyManager::SaveToConfig(const std::string& configPath) const {
        try {
            nlohmann::json j;
            for (size_t i = 0; i < static_cast<size_t>(EditorActions::COUNT); i++) {
                j[std::to_string(i)] = {
                    {"key", static_cast<int>(m_HotkeyActions[i].m_Key)},
                    {"press", m_HotkeyActions[i].m_Press},
                    {"control", m_HotkeyActions[i].m_Control},
                    {"shift", m_HotkeyActions[i].m_Shift}
                };
            }
            std::ofstream f(configPath);
            if (f.is_open()) f << j.dump(2);
        } catch (const std::exception& e) {
            GE_CORE_WARN("Failed to save hotkey config: {0}", e.what());
        }
    }

    std::array<HotkeyInfo, static_cast<size_t>(EditorActions::COUNT)> HotkeyManager::GetDefaultHotkeys() {
        std::array<HotkeyInfo, static_cast<size_t>(EditorActions::COUNT)> defaults;
        
        // Camera movement
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_FORWARD)] = {KeyCode::W, false, false, false};
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_BACKWARD)] = {KeyCode::S, false, false, false};
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_LEFT)] = {KeyCode::A, false, false, false};
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_RIGHT)] = {KeyCode::D, false, false, false};
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_UP)] = {KeyCode::E, false, false, false};
        defaults[static_cast<size_t>(EditorActions::MOVE_CAMERA_DOWN)] = {KeyCode::Q, false, false, false};
        
        // Entity actions
        defaults[static_cast<size_t>(EditorActions::DUPLICATE_ENTITY)] = {KeyCode::D, true, true, false};
        defaults[static_cast<size_t>(EditorActions::DELETE_ENTITY)] = {KeyCode::Delete, true, false, false};
        
        // Selection
        defaults[static_cast<size_t>(EditorActions::SELECT_ALL_ENTITIES)] = {KeyCode::A, true, true, false};
        defaults[static_cast<size_t>(EditorActions::DESELECT_ALL_ENTITIES)] = {KeyCode::Escape, true, false, false};
        defaults[static_cast<size_t>(EditorActions::FOCUS_ON_SELECTION)] = {KeyCode::F, false, false, false};
        defaults[static_cast<size_t>(EditorActions::RENAME_SELECTED)] = {KeyCode::F2, true, false, false};
        
        // Edit actions
        defaults[static_cast<size_t>(EditorActions::UNDO_ACTION)] = {KeyCode::Z, true, true, false};
        defaults[static_cast<size_t>(EditorActions::REDO_ACTION)] = {KeyCode::Y, true, true, false};
        defaults[static_cast<size_t>(EditorActions::COPY_ACTION)] = {KeyCode::C, true, true, false};
        defaults[static_cast<size_t>(EditorActions::CUT_ACTION)] = {KeyCode::X, true, true, false};
        defaults[static_cast<size_t>(EditorActions::PASTE_ACTION)] = {KeyCode::V, true, true, false};
        defaults[static_cast<size_t>(EditorActions::DELETE_ACTION)] = {KeyCode::Delete, true, false, false};
        
        // Transform toggles
        defaults[static_cast<size_t>(EditorActions::MOVE_TOGGLE_ACTION)] = {KeyCode::D1, true, false, false};
        defaults[static_cast<size_t>(EditorActions::ROTATE_TOGGLE_ACTION)] = {KeyCode::D2, true, false, false};
        defaults[static_cast<size_t>(EditorActions::SCALE_TOGGLE_ACTION)] = {KeyCode::D3, true, false, false};
        defaults[static_cast<size_t>(EditorActions::LOCAL_GLOBAL_TOGGLE_ACTION)] = {KeyCode::D4, true, false, false};
        
        // Scene actions
        defaults[static_cast<size_t>(EditorActions::SAVE_SCENE)] = {KeyCode::S, true, true, false};
        defaults[static_cast<size_t>(EditorActions::SAVE_SCENE_AS)] = {KeyCode::S, true, true, true};
        defaults[static_cast<size_t>(EditorActions::NEW_SCENE)] = {KeyCode::N, true, true, false};
        defaults[static_cast<size_t>(EditorActions::OPEN_SCENE)] = {KeyCode::O, true, true, false};
        
        // View actions
        defaults[static_cast<size_t>(EditorActions::WIREFRAME_MODE)] = {KeyCode::W, true, true, false};
        defaults[static_cast<size_t>(EditorActions::ORTHO_CAMERA)] = {KeyCode::O, true, false, false};
        defaults[static_cast<size_t>(EditorActions::INSPECTOR_MODE)] = {KeyCode::I, false, false, false};
        
        // Screenshot actions
        defaults[static_cast<size_t>(EditorActions::SCREENSHOT)] = {KeyCode::F3, true, false, false};
        defaults[static_cast<size_t>(EditorActions::SCREENSHOT_ALPHA)] = {KeyCode::F4, true, false, false};
        
        return defaults;
    }

}
