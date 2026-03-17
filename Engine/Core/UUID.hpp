#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>

namespace GameEngine {

    /**
     * @brief Universally Unique Identifier
     *
     * 64-bit UUID for entity and resource identification
     * Thread-safe generation using atomic operations
     */
    class UUID {
    public:
        UUID();
        UUID(uint64_t uuid);
        UUID(const UUID&) = default;

        operator uint64_t() const { return m_UUID; }

        std::string ToString() const;

        bool operator==(const UUID& other)  const { return m_UUID == other.m_UUID; }
        bool operator!=(const UUID& other)  const { return m_UUID != other.m_UUID; }
        bool operator==(uint64_t other)     const { return m_UUID == other; }
        bool operator!=(uint64_t other)     const { return m_UUID != other; }

        friend bool operator==(uint64_t lhs, const UUID& rhs) { return lhs == rhs.m_UUID; }
        friend bool operator!=(uint64_t lhs, const UUID& rhs) { return lhs != rhs.m_UUID; }

    private:
        uint64_t m_UUID;
    };
}

// Hash function for UUID (for use in unordered_map)
namespace std {
    template<>
    struct hash<GameEngine::UUID> {
        std::size_t operator()(const GameEngine::UUID& uuid) const {
            return hash<uint64_t>()(static_cast<uint64_t>(uuid));
        }
    };
}
