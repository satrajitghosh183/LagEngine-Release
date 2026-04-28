#pragma once

#include <functional>
#include <vector>
#include <cstddef>

namespace GameEngine {
namespace UI {

    // Type-safe signal/slot (like Godot's signal system).
    // Connect slots with lambdas or function objects; emit forwards args.
    template <typename... Args>
    class UISignal {
    public:
        using Slot = std::function<void(Args...)>;
        using Handle = size_t;

        Handle Connect(Slot slot) {
            Handle h = m_NextHandle++;
            m_Slots.push_back({h, std::move(slot)});
            return h;
        }

        void Disconnect(Handle handle) {
            for (auto it = m_Slots.begin(); it != m_Slots.end(); ++it) {
                if (it->HandleID == handle) { m_Slots.erase(it); return; }
            }
        }

        void DisconnectAll() { m_Slots.clear(); }

        void Emit(Args... args) const {
            for (const auto& s : m_Slots) s.Callback(args...);
        }

        size_t SlotCount() const { return m_Slots.size(); }

    private:
        struct Entry { Handle HandleID; Slot Callback; };
        std::vector<Entry> m_Slots;
        Handle m_NextHandle = 1;
    };

}}
