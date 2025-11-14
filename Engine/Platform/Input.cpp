#include "Input.hpp"
#include "../Core/Application.hpp"
#include <GLFW/glfw3.h>

namespace GameEngine {

    Input& Input::GetInstance() {
        static Input instance;
        return instance;
    }

    void Input::Update() {
        GetInstance().UpdateImpl();
    }

    void Input::UpdateImpl() {
        // Copy current states to previous
        memcpy(m_State.PrevKeyStates, m_State.KeyStates, sizeof(m_State.KeyStates));
        memcpy(m_State.PrevMouseButtonStates, m_State.MouseButtonStates, sizeof(m_State.MouseButtonStates));
        
        m_State.PrevMousePosition = m_State.MousePosition;
        
        // Get window
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        
        // Update key states
        for (int key = 32; key < 512; key++) {
            m_State.KeyStates[key] = glfwGetKey(window, key) == GLFW_PRESS;
        }
        
        // Update mouse button states
        for (int button = 0; button < 8; button++) {
            m_State.MouseButtonStates[button] = glfwGetMouseButton(window, button) == GLFW_PRESS;
        }
        
        // Update mouse position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        m_State.MousePosition = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        
        // Calculate mouse delta
        m_State.MouseDelta = m_State.MousePosition - m_State.PrevMousePosition;
        
        // Mouse scroll is set by callback (reset each frame)
        m_State.MouseScroll = glm::vec2(0.0f);
    }

    bool Input::IsKeyPressed(KeyCode keycode) {
        return GetInstance().IsKeyPressedImpl(keycode);
    }

    bool Input::IsKeyPressedImpl(KeyCode keycode) {
        int key = static_cast<int>(keycode);
        if (key >= 0 && key < 512) {
            return m_State.KeyStates[key];
        }
        return false;
    }

    bool Input::IsKeyJustPressed(KeyCode keycode) {
        return GetInstance().IsKeyJustPressedImpl(keycode);
    }

    bool Input::IsKeyJustPressedImpl(KeyCode keycode) {
        int key = static_cast<int>(keycode);
        if (key >= 0 && key < 512) {
            return m_State.KeyStates[key] && !m_State.PrevKeyStates[key];
        }
        return false;
    }

    bool Input::IsKeyJustReleased(KeyCode keycode) {
        int key = static_cast<int>(keycode);
        if (key >= 0 && key < 512) {
            auto& state = GetInstance().m_State;
            return !state.KeyStates[key] && state.PrevKeyStates[key];
        }
        return false;
    }

    bool Input::IsMouseButtonPressed(MouseButton button) {
        int btn = static_cast<int>(button);
        if (btn >= 0 && btn < 8) {
            return GetInstance().m_State.MouseButtonStates[btn];
        }
        return false;
    }

    bool Input::IsMouseButtonJustPressed(MouseButton button) {
        int btn = static_cast<int>(button);
        if (btn >= 0 && btn < 8) {
            auto& state = GetInstance().m_State;
            return state.MouseButtonStates[btn] && !state.PrevMouseButtonStates[btn];
        }
        return false;
    }

    glm::vec2 Input::GetMousePosition() {
        return GetInstance().m_State.MousePosition;
    }

    glm::vec2 Input::GetMouseDelta() {
        return GetInstance().m_State.MouseDelta;
    }

    glm::vec2 Input::GetMouseScroll() {
        return GetInstance().m_State.MouseScroll;
    }

    void Input::SetCursorVisible(bool visible) {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        glfwSetInputMode(window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
    }

    void Input::SetCursorLocked(bool locked) {
        auto* window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
        glfwSetInputMode(window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    void Input::MapAction(const std::string& action, KeyCode key) {
        GetInstance().m_ActionMap[action] = key;
    }

    bool Input::IsActionPressed(const std::string& action) {
        auto& map = GetInstance().m_ActionMap;
        auto it = map.find(action);
        if (it != map.end()) {
            return IsKeyPressed(it->second);
        }
        return false;
    }

    void Input::MapAxis(const std::string& axis, KeyCode positive, KeyCode negative) {
        GetInstance().m_AxisMap[axis] = { positive, negative };
    }

    float Input::GetAxis(const std::string& axis) {
        auto& map = GetInstance().m_AxisMap;
        auto it = map.find(axis);
        if (it != map.end()) {
            float value = 0.0f;
            if (IsKeyPressed(it->second.first)) value += 1.0f;
            if (IsKeyPressed(it->second.second)) value -= 1.0f;
            return value;
        }
        return 0.0f;
    }
}