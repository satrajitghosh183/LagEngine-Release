#pragma once

#include "Event.hpp"

namespace GameEngine {

    /**
     * @brief Window resize event
     */
    class WindowResizeEvent : public Event {
    public:
        WindowResizeEvent(uint32_t width, uint32_t height)
            : m_Width(width), m_Height(height) {}
        
        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        
        std::string ToString() const override {
            return "WindowResizeEvent: " + std::to_string(m_Width) + "x" + std::to_string(m_Height);
        }
        
        EVENT_CLASS_TYPE(WindowResize)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
        
    private:
        uint32_t m_Width, m_Height;
    };

    /**
     * @brief Window close event
     */
    class WindowCloseEvent : public Event {
    public:
        WindowCloseEvent() = default;
        
        EVENT_CLASS_TYPE(WindowClose)
        EVENT_CLASS_CATEGORY(EventCategoryApplication)
    };
}