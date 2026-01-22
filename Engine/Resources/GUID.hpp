#pragma once

#include "../Core/Base.hpp"
#include <string>
#include <cstdint>
#include <functional>

namespace GameEngine {

    /**
     * @brief GUID for asset references
     */
    class GUID {
    public:
        GUID();
        GUID(uint64_t high, uint64_t low);
        explicit GUID(const std::string& str);
        
        /**
         * @brief Generate new GUID
         */
        static GUID Generate();
        
        /**
         * @brief Convert to string
         */
        std::string ToString() const;
        
        /**
         * @brief Check if valid
         */
        bool IsValid() const;
        
        /**
         * @brief Comparison operators
         */
        bool operator==(const GUID& other) const;
        bool operator!=(const GUID& other) const;
        bool operator<(const GUID& other) const;
        
        /**
         * @brief Get hash value
         */
        size_t GetHash() const {
            return std::hash<uint64_t>()(m_High) ^ (std::hash<uint64_t>()(m_Low) << 1);
        }
        
    private:
        uint64_t m_High;
        uint64_t m_Low;
    };

}

// Hash specialization for std::unordered_map
namespace std {
    template<>
    struct hash<GameEngine::GUID> {
        size_t operator()(const GameEngine::GUID& guid) const {
            return guid.GetHash();
        }
    };
}
