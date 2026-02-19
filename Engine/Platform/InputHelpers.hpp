#pragma once

#include "Input.hpp"
#include "../Core/Base.hpp"

namespace GameEngine {
namespace InputHelpers {

    /**
     * @brief Wicked Engine-style input helpers
     * 
     * Provides convenience functions matching Wicked Engine's input API
     */

    /**
     * @brief Check if key was just pressed this frame
     */
    bool Press(KeyCode key);

    /**
     * @brief Check if key is currently held down
     */
    bool Down(KeyCode key);

    /**
     * @brief Key constants matching Wicked Engine style
     */
    namespace Keys {
        constexpr KeyCode KEYBOARD_BUTTON_SPACE = KeyCode::Space;
        constexpr KeyCode KEYBOARD_BUTTON_LEFT = KeyCode::Left;
        constexpr KeyCode KEYBOARD_BUTTON_RIGHT = KeyCode::Right;
        constexpr KeyCode KEYBOARD_BUTTON_UP = KeyCode::Up;
        constexpr KeyCode KEYBOARD_BUTTON_DOWN = KeyCode::Down;
        constexpr KeyCode KEYBOARD_BUTTON_ESCAPE = KeyCode::Escape;
        constexpr KeyCode KEYBOARD_BUTTON_ENTER = KeyCode::Enter;
        constexpr KeyCode KEYBOARD_BUTTON_TAB = KeyCode::Tab;
        constexpr KeyCode KEYBOARD_BUTTON_BACKSPACE = KeyCode::Backspace;
        constexpr KeyCode KEYBOARD_BUTTON_DELETE = KeyCode::Delete;
        constexpr KeyCode KEYBOARD_BUTTON_HOME = KeyCode::Home;
        constexpr KeyCode KEYBOARD_BUTTON_END = KeyCode::End;
        constexpr KeyCode KEYBOARD_BUTTON_PAGEUP = KeyCode::PageUp;
        constexpr KeyCode KEYBOARD_BUTTON_PAGEDOWN = KeyCode::PageDown;
        constexpr KeyCode KEYBOARD_BUTTON_F1 = KeyCode::F1;
        constexpr KeyCode KEYBOARD_BUTTON_F2 = KeyCode::F2;
        constexpr KeyCode KEYBOARD_BUTTON_F3 = KeyCode::F3;
        constexpr KeyCode KEYBOARD_BUTTON_F4 = KeyCode::F4;
        constexpr KeyCode KEYBOARD_BUTTON_F5 = KeyCode::F5;
        constexpr KeyCode KEYBOARD_BUTTON_F6 = KeyCode::F6;
        constexpr KeyCode KEYBOARD_BUTTON_F7 = KeyCode::F7;
        constexpr KeyCode KEYBOARD_BUTTON_F8 = KeyCode::F8;
        constexpr KeyCode KEYBOARD_BUTTON_F9 = KeyCode::F9;
        constexpr KeyCode KEYBOARD_BUTTON_F10 = KeyCode::F10;
        constexpr KeyCode KEYBOARD_BUTTON_F11 = KeyCode::F11;
        constexpr KeyCode KEYBOARD_BUTTON_F12 = KeyCode::F12;
        constexpr KeyCode MOUSE_BUTTON_LEFT = static_cast<KeyCode>(256);
        constexpr KeyCode MOUSE_BUTTON_RIGHT = static_cast<KeyCode>(257);
        constexpr KeyCode MOUSE_BUTTON_MIDDLE = static_cast<KeyCode>(258);
    }

}

}
