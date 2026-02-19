#pragma once

#include "../Core/Base.hpp"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

namespace GameEngine {

    // Forward declarations
    class AudioClip;
    class AudioSource;

    /**
     * @brief Audio engine (OpenAL wrapper)
     * 
     * Features:
     * - 3D spatial audio
     * - Multiple audio sources
     * - Master volume control
     * - Distance attenuation
     * - Doppler effect
     */
    class AudioEngine {
    public:
        /**
         * @brief Initialize audio engine
         */
        static bool Init();
        
        /**
         * @brief Shutdown audio engine
         */
        static void Shutdown();
        
        /**
         * @brief Update audio engine (call each frame)
         */
        static void Update();
        
        /**
         * @brief Set listener position (usually camera position)
         */
        static void SetListenerPosition(const glm::vec3& position);
        
        /**
         * @brief Set listener orientation
         */
        static void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);
        
        /**
         * @brief Set listener velocity (for Doppler effect)
         */
        static void SetListenerVelocity(const glm::vec3& velocity);
        
        /**
         * @brief Set master volume [0, 1]
         */
        static void SetMasterVolume(float volume);
        
        /**
         * @brief Get master volume
         */
        static float GetMasterVolume();
        
        /**
         * @brief Play 2D sound (non-spatial)
         */
        static void Play2D(const std::string& clipPath, float volume = 1.0f, bool loop = false);
        
        /**
         * @brief Play 3D sound at position
         */
        static void Play3D(const std::string& clipPath, const glm::vec3& position, 
                          float volume = 1.0f, bool loop = false);

        /**
         * @brief Stop playback of a clip by path (stops any source playing this clip)
         */
        static void StopClip(const std::string& clipPath);

        /**
         * @brief Load audio clip
         */
        static Ref<AudioClip> LoadClip(const std::string& path);
        
        /**
         * @brief Unload audio clip
         */
        static void UnloadClip(const std::string& path);
        
    private:
        static void* s_Device;
        static void* s_Context;
        static float s_MasterVolume;
        static std::unordered_map<std::string, Ref<AudioClip>> s_LoadedClips;
    };
}