#pragma once

#include <chrono>

namespace GameEngine {

    /**
     * @brief High-resolution time management system
     * 
     * Features:
     * - Delta time calculation
     * - Fixed timestep accumulator for physics
     * - Time scaling (slow motion, pause)
     * - Frame rate tracking
     */
    class Time {
    public:
        /**
         * @brief Initialize time system
         */
        static void Init();
        
        /**
         * @brief Update time (call once per frame)
         */
        static void Update();
        
        /**
         * @brief Get delta time in seconds
         */
        static float GetDeltaTime() { return s_DeltaTime * s_TimeScale; }
        
        /**
         * @brief Get unscaled delta time in seconds
         */
        static float GetUnscaledDeltaTime() { return s_DeltaTime; }
        
        /**
         * @brief Get time since engine start in seconds
         */
        static float GetTime() { return s_Time; }
        
        /**
         * @brief Get current frame count
         */
        static uint64_t GetFrameCount() { return s_FrameCount; }
        
        /**
         * @brief Get current FPS
         */
        static float GetFPS() { return s_FPS; }
        
        /**
         * @brief Set time scale (1.0 = normal, 0.5 = half speed, 2.0 = double speed)
         */
        static void SetTimeScale(float scale) { s_TimeScale = scale; }
        
        /**
         * @brief Get current time scale
         */
        static float GetTimeScale() { return s_TimeScale; }
        
        /**
         * @brief Get fixed timestep for physics (default 1/60 = 0.0166s)
         */
        static float GetFixedTimestep() { return s_FixedTimestep; }
        
        /**
         * @brief Set fixed timestep for physics
         */
        static void SetFixedTimestep(float timestep) { s_FixedTimestep = timestep; }
        
    private:
        using Clock = std::chrono::high_resolution_clock;
        using TimePoint = std::chrono::time_point<Clock>;
        
        static TimePoint s_StartTime;
        static TimePoint s_LastFrameTime;
        static float s_DeltaTime;
        static float s_Time;
        static float s_TimeScale;
        static float s_FixedTimestep;
        static uint64_t s_FrameCount;
        static float s_FPS;
        static float s_FPSAccumulator;
        static int s_FPSFrameCount;
    };
}