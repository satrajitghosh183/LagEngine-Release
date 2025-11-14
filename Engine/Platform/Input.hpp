#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>

namespace GameEngine {

    /**
     * @brief Input key codes (matching GLFW)
     */
    enum class KeyCode {
        Space = 32,
        Apostrophe = 39,
        Comma = 44,
        Minus = 45,
        Period = 46,
        Slash = 47,
        
        D0 = 48, D1 = 49, D2 = 50, D3 = 51, D4 = 52,
        D5 = 53, D6 = 54, D7 = 55, D8 = 56, D9 = 57,
        
        Semicolon = 59,
        Equal = 61,
        
        A = 65, B = 66, C = 67, D = 68, E = 69, F = 70,
        G = 71, H = 72, I = 73, J = 74, K = 75, L = 76,
        M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82,
        S = 83, T = 84, U = 85, V = 86, W = 87, X = 88,
        Y = 89, Z = 90,
        
        LeftBracket = 91,
        Backslash = 92,
        RightBracket = 93,
        GraveAccent = 96,
        
        Escape = 256,
        Enter = 257,
        Tab = 258,
        Backspace = 259,
        Insert = 260,
        Delete = 261,
        Right = 262,
        Left = 263,
        Down = 264,
        Up = 265,
        PageUp = 266,
        PageDown = 267,
        Home = 268,
        End = 269,
        
        CapsLock = 280,
        ScrollLock = 281,
        NumLock = 282,
        PrintScreen = 283,
        Pause = 284,
        
        F1 = 290, F2 = 291, F3 = 292, F4 = 293,
        F5 = 294, F6 = 295, F7 = 296, F8 = 297,
        F9 = 298, F10 = 299, F11 = 300, F12 = 301,
        
        LeftShift = 340,
        LeftControl = 341,
        LeftAlt = 342,
        LeftSuper = 343,
        RightShift = 344,
        RightControl = 345,
        RightAlt = 346,
        RightSuper = 347
    };

    /**
     * @brief Mouse button codes
     */
    enum class MouseButton {
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7
    };

    /**
     * @brief Input state manager
     * 
     * Provides polling-based input queries
     * Singleton pattern for global access
     */
    class Input {
    public:
        /**
         * @brief Check if key is pressed
         */
        static bool IsKeyPressed(KeyCode keycode);
        
        /**
         * @brief Check if key was just pressed this frame
         */
        static bool IsKeyJustPressed(KeyCode keycode);
        
        /**
         * @brief Check if key was just released this frame
         */
        static bool IsKeyJustReleased(KeyCode keycode);
        
        /**
         * @brief Check if mouse button is pressed
         */
        static bool IsMouseButtonPressed(MouseButton button);
        
        /**
         * @brief Check if mouse button was just pressed
         */
        static bool IsMouseButtonJustPressed(MouseButton button);
        
        /**
         * @brief Get mouse position (screen coordinates)
         */
        static glm::vec2 GetMousePosition();
        
        /**
         * @brief Get mouse delta (movement since last frame)
         */
        static glm::vec2 GetMouseDelta();
        
        /**
         * @brief Get mouse scroll delta
         */
        static glm::vec2 GetMouseScroll();
        
        /**
         * @brief Set cursor visibility
         */
        static void SetCursorVisible(bool visible);
        
        /**
         * @brief Set cursor locked (for FPS games)
         */
        static void SetCursorLocked(bool locked);
        
        /**
         * @brief Input action mapping
         */
        static void MapAction(const std::string& action, KeyCode key);
        static bool IsActionPressed(const std::string& action);
        
        /**
         * @brief Input axis mapping (for analog input)
         */
        static void MapAxis(const std::string& axis, KeyCode positive, KeyCode negative);
        static float GetAxis(const std::string& axis);
        
        /**
         * @brief Update input state (called by engine)
         */
        static void Update();
        
    private:
        Input() = default;
        
        static Input& GetInstance();
        
        void UpdateImpl();
        bool IsKeyPressedImpl(KeyCode keycode);
        bool IsKeyJustPressedImpl(KeyCode keycode);
        
        struct InputState {
            bool KeyStates[512] = { false };
            bool PrevKeyStates[512] = { false };
            
            bool MouseButtonStates[8] = { false };
            bool PrevMouseButtonStates[8] = { false };
            
            glm::vec2 MousePosition = glm::vec2(0.0f);
            glm::vec2 PrevMousePosition = glm::vec2(0.0f);
            glm::vec2 MouseDelta = glm::vec2(0.0f);
            
            glm::vec2 MouseScroll = glm::vec2(0.0f);
        };
        
        InputState m_State;
        
        std::unordered_map<std::string, KeyCode> m_ActionMap;
        std::unordered_map<std::string, std::pair<KeyCode, KeyCode>> m_AxisMap;
    };
}