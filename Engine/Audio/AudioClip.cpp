#include "AudioClip.hpp"
#include "../Core/Logger.hpp"
#include <fstream>
#include <cstring>

namespace GameEngine {

    AudioClip::AudioClip(const std::string& path)
        : m_BufferID(0), m_Path(path), m_Duration(0.0f) {
        
        // Determine file type by extension
        std::string extension;
        size_t dotPos = path.rfind('.');
        if (dotPos != std::string::npos) {
            extension = path.substr(dotPos + 1);
        }
        
        // Convert to lowercase for comparison
        for (char& c : extension) {
            c = static_cast<char>(tolower(c));
        }
        
        bool loaded = false;
        if (extension == "wav") {
            loaded = LoadWAV(path);
        } else if (extension == "ogg") {
            loaded = LoadOGG(path);
        } else {
            GE_CORE_ERROR("Unsupported audio format: {0}", extension);
        }
        
        if (!loaded) {
            GE_CORE_ERROR("Failed to load audio clip: {0}", path);
        }
    }

    AudioClip::~AudioClip() {
        // Note: In a real implementation, we'd release the OpenAL buffer here
        // For now, we just log that we're cleaning up
        if (m_BufferID != 0) {
            // alDeleteBuffers(1, &m_BufferID); // Would be used with OpenAL
            m_BufferID = 0;
        }
    }

    bool AudioClip::LoadWAV(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            GE_CORE_ERROR("Cannot open WAV file: {0}", path);
            return false;
        }
        
        // Read WAV header
        char riff[4];
        file.read(riff, 4);
        if (std::strncmp(riff, "RIFF", 4) != 0) {
            GE_CORE_ERROR("Invalid WAV file (no RIFF header): {0}", path);
            return false;
        }
        
        // Skip file size
        file.seekg(4, std::ios::cur);
        
        char wave[4];
        file.read(wave, 4);
        if (std::strncmp(wave, "WAVE", 4) != 0) {
            GE_CORE_ERROR("Invalid WAV file (no WAVE marker): {0}", path);
            return false;
        }
        
        // Find fmt chunk
        char chunkId[4];
        uint32_t chunkSize;
        
        while (file.read(chunkId, 4)) {
            file.read(reinterpret_cast<char*>(&chunkSize), 4);
            
            if (std::strncmp(chunkId, "fmt ", 4) == 0) {
                // Read format data
                uint16_t audioFormat, numChannels;
                uint32_t sampleRate, byteRate;
                uint16_t blockAlign, bitsPerSample;
                
                file.read(reinterpret_cast<char*>(&audioFormat), 2);
                file.read(reinterpret_cast<char*>(&numChannels), 2);
                file.read(reinterpret_cast<char*>(&sampleRate), 4);
                file.read(reinterpret_cast<char*>(&byteRate), 4);
                file.read(reinterpret_cast<char*>(&blockAlign), 2);
                file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
                
                // Skip any extra format bytes
                if (chunkSize > 16) {
                    file.seekg(chunkSize - 16, std::ios::cur);
                }
                
            } else if (std::strncmp(chunkId, "data", 4) == 0) {
                // Found data chunk
                // In a real implementation, we'd create an OpenAL buffer here
                // For now, just calculate duration
                
                // Duration = data size / (sample rate * channels * bytes per sample)
                // We don't have all the info here, so estimate
                // Assuming 44100 Hz, stereo, 16-bit
                float bytesPerSecond = 44100.0f * 2.0f * 2.0f;
                m_Duration = static_cast<float>(chunkSize) / bytesPerSecond;
                
                // Fake buffer ID for now
                m_BufferID = 1;
                
                GE_CORE_INFO("Loaded WAV: {0}, duration: {1:.2f}s", path, m_Duration);
                return true;
            } else {
                // Skip unknown chunk
                file.seekg(chunkSize, std::ios::cur);
            }
        }
        
        GE_CORE_ERROR("No data chunk found in WAV file: {0}", path);
        return false;
    }

    bool AudioClip::LoadOGG(const std::string& path) {
        // OGG loading would require a library like stb_vorbis or libvorbis
        // For now, create a placeholder implementation
        
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) {
            GE_CORE_ERROR("Cannot open OGG file: {0}", path);
            return false;
        }
        
        // Check OGG signature
        char signature[4];
        file.read(signature, 4);
        if (std::strncmp(signature, "OggS", 4) != 0) {
            GE_CORE_ERROR("Invalid OGG file: {0}", path);
            return false;
        }
        
        // Placeholder: Estimate duration based on file size
        file.seekg(0, std::ios::end);
        size_t fileSize = static_cast<size_t>(file.tellg());
        
        // Rough estimate: ~128kbps bitrate
        m_Duration = static_cast<float>(fileSize) / (128000.0f / 8.0f);
        m_BufferID = 1; // Fake buffer ID
        
        GE_CORE_WARN("OGG loading is placeholder only. File: {0}", path);
        return true;
    }

}
