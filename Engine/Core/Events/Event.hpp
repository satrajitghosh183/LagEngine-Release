#pragma once

#include "../Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Event types
     */
    enum class EventType {
        None = 0,
        WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
        AppTick, AppUpdate, AppRender,
        KeyPressed, KeyReleased, KeyTyped,
        MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
    };

    /**
     * @brief Event categories (can be combined with bitwise OR)
     */
    enum EventCategory {
        None = 0,
        EventCategoryApplication    = BIT(0),
        EventCategoryInput          = BIT(1),
        EventCategoryKeyboard       = BIT(2),
        EventCategoryMouse          = BIT(3),
        EventCategoryMouseButton    = BIT(4)
    };

    /**
     * @brief Base event class
     * 
     * All events inherit from this class
     * Uses virtual dispatch for type-safe event handling
     */
    class Event {
    public:
        virtual ~Event() = default;
        
        virtual EventType GetEventType() const = 0;
        virtual const char* GetName() const = 0;
        virtual int GetCategoryFlags() const = 0;
        virtual std::string ToString() const { return GetName(); }
        
        bool IsInCategory(EventCategory category) const {
            return GetCategoryFlags() & category;
        }
        
        bool Handled = false;
    };

    /**
     * @brief Event dispatcher
     * 
     * Dispatches events to appropriate handlers
     * Usage:
     *   EventDispatcher dispatcher(event);
     *   dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
     */
    class EventDispatcher {
    public:
        EventDispatcher(Event& event)
            : m_Event(event) {}
        
        template<typename T, typename F>
        bool Dispatch(const F& func) {
            if (m_Event.GetEventType() == T::GetStaticType()) {
                m_Event.Handled = func(static_cast<T&>(m_Event));
                return true;
            }
            return false;
        }
        
    private:
        Event& m_Event;
    };
}

// Helper macros for event class declaration
#define EVENT_CLASS_TYPE(type) \
    static EventType GetStaticType() { return EventType::type; } \
    virtual EventType GetEventType() const override { return GetStaticType(); } \
    virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
    virtual int GetCategoryFlags() const override { return category; }
