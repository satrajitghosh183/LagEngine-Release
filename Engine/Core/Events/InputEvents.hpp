#pragma once

#include "Event.hpp"

namespace GameEngine {

    /**
     * @brief Key pressed event
     */
    class KeyPressedEvent : public Event {
    public:
        KeyPressedEvent(int keycode, int repeatCount)
            : m_KeyCode(keycode), m_RepeatCount(repeatCount) {}
        
        int GetKeyCode() const { return m_KeyCode; }
        int GetRepeatCount() const { return m_RepeatCount; }
        
        std::string ToString() const override {
            return "KeyPressedEvent: " + std::to_string(m_KeyCode) + " (" + std::to_string(m_RepeatCount) + " repeats)";
        }
        
        EVENT_CLASS_TYPE(KeyPressed)
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
        
    private:
        int m_KeyCode;
        int m_RepeatCount;
    };

    /**
     * @brief Key released event
     */
    class KeyReleasedEvent : public Event {
    public:
        KeyReleasedEvent(int keycode)
            : m_KeyCode(keycode) {}
        
        int GetKeyCode() const { return m_KeyCode; }
        
        std::string ToString() const override {
            return "KeyReleasedEvent: " + std::to_string(m_KeyCode);
        }
        
        EVENT_CLASS_TYPE(KeyReleased)
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
        
    private:
        int m_KeyCode;
    };

    /**
     * @brief Mouse button pressed event
     */
    class MouseButtonPressedEvent : public Event {
    public:
        MouseButtonPressedEvent(int button)
            : m_Button(button) {}
        
        int GetMouseButton() const { return m_Button; }
        
        std::string ToString() const override {
            return "MouseButtonPressedEvent: " + std::to_string(m_Button);
        }
        
        EVENT_CLASS_TYPE(MouseButtonPressed)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
        
    private:
        int m_Button;
    };

    /**
     * @brief Mouse button released event
     */
    class MouseButtonReleasedEvent : public Event {
    public:
        MouseButtonReleasedEvent(int button)
            : m_Button(button) {}
        
        int GetMouseButton() const { return m_Button; }
        
        std::string ToString() const override {
            return "MouseButtonReleasedEvent: " + std::to_string(m_Button);
        }
        
        EVENT_CLASS_TYPE(MouseButtonReleased)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput | EventCategoryMouseButton)
        
    private:
        int m_Button;
    };

    /**
     * @brief Mouse moved event
     */
    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(float x, float y)
            : m_MouseX(x), m_MouseY(y) {}
        
        float GetX() const { return m_MouseX; }
        float GetY() const { return m_MouseY; }
        
        std::string ToString() const override {
            return "MouseMovedEvent: " + std::to_string(m_MouseX) + ", " + std::to_string(m_MouseY);
        }
        
        EVENT_CLASS_TYPE(MouseMoved)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
        
    private:
        float m_MouseX, m_MouseY;
    };

    /**
     * @brief Mouse scrolled event
     */
    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(float xOffset, float yOffset)
            : m_XOffset(xOffset), m_YOffset(yOffset) {}
        
        float GetXOffset() const { return m_XOffset; }
        float GetYOffset() const { return m_YOffset; }
        
        std::string ToString() const override {
            return "MouseScrolledEvent: " + std::to_string(m_XOffset) + ", " + std::to_string(m_YOffset);
        }
        
        EVENT_CLASS_TYPE(MouseScrolled)
        EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)
        
    private:
        float m_XOffset, m_YOffset;
    };
}