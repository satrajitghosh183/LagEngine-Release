#pragma once

#include "../Core/Base.hpp"
#include <string>

namespace GameEngine {

    /**
     * @brief Audio clip - loaded audio file
     * 
     * Supports WAV, OGG, MP3 formats
     */
    class AudioClip {
    public:
        AudioClip(const std::string& path);
        ~AudioClip();
        
        /**
         * @brief Get OpenAL buffer ID
         */
        uint32_t GetBufferID() const { return m_BufferID; }
        
        /**
         * @brief Get path
         */
        const std::string& GetPath() const { return m_Path; }
        
        /**
         * @brief Get duration in seconds
         */
        float GetDuration() const { return m_Duration; }
        
        /**
         * @brief Check if valid
         */
        bool IsValid() const { return m_BufferID != 0; }
        
    private:
        bool LoadWAV(const std::string& path);
        bool LoadOGG(const std::string& path);
        
    private:
        uint32_t m_BufferID;
        std::string m_Path;
        float m_Duration;
    };
}