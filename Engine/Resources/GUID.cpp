#include "GUID.hpp"
#include "../Core/Logger.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace GameEngine {

    GUID::GUID()
        : m_High(0)
        , m_Low(0) {
    }

    GUID::GUID(uint64_t high, uint64_t low)
        : m_High(high)
        , m_Low(low) {
    }

    GUID::GUID(const std::string& str) {
        // Parse GUID string format: "01234567-89ab-cdef-0123-456789abcdef"
        // For now, simple hash-based conversion
        std::hash<std::string> hasher;
        size_t hash = hasher(str);
        m_High = hash;
        m_Low = hash >> 32;
    }

    GUID GUID::Generate() {
        static std::random_device rd;
        static std::mt19937_64 gen(rd());
        static std::uniform_int_distribution<uint64_t> dis;
        
        return GUID(dis(gen), dis(gen));
    }

    std::string GUID::ToString() const {
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(8) << (m_High >> 32) << "-"
           << std::setw(4) << ((m_High >> 16) & 0xFFFF) << "-"
           << std::setw(4) << (m_High & 0xFFFF) << "-"
           << std::setw(4) << (m_Low >> 48) << "-"
           << std::setw(12) << (m_Low & 0xFFFFFFFFFFFFULL);
        return ss.str();
    }

    bool GUID::IsValid() const {
        return m_High != 0 || m_Low != 0;
    }

    bool GUID::operator==(const GUID& other) const {
        return m_High == other.m_High && m_Low == other.m_Low;
    }

    bool GUID::operator!=(const GUID& other) const {
        return !(*this == other);
    }

    bool GUID::operator<(const GUID& other) const {
        if (m_High < other.m_High) return true;
        if (m_High > other.m_High) return false;
        return m_Low < other.m_Low;
    }

}

