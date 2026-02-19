#pragma once

#include "../../Engine/Core/Base.hpp"
#include <imgui.h>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

namespace GameEngine {

    /**
     * @brief Profiler window - performance monitoring
     * 
     * Features:
     * - Frame time and FPS display
     * - CPU/GPU timing breakdown
     * - Per-system timing (rendering, physics, scripting, etc.)
     * - Memory usage statistics
     * - Timeline view
     */
    class ProfilerWindow {
    public:
        ProfilerWindow();
        ~ProfilerWindow() = default;

        void OnImGuiRender();

        /**
         * @brief Record timing for a system
         */
        void RecordTiming(const std::string& systemName, float timeMs);

        /**
         * @brief Record frame time
         */
        void RecordFrameTime(float frameTimeMs);

        /**
         * @brief Toggle window visibility
         */
        void Toggle() { m_IsOpen = !m_IsOpen; }
        
        /**
         * @brief Set window visibility
         */
        void SetOpen(bool open) { m_IsOpen = open; }
        
        /**
         * @brief Check if window is open
         */
        bool IsOpen() const { return m_IsOpen; }

    private:
        void RenderFrameStats();
        void RenderSystemTimings();
        void RenderMemoryStats();
        void RenderTimeline();

    private:
        bool m_IsOpen = false;
        
        // Frame statistics
        float m_CurrentFrameTime = 0.0f;
        float m_AverageFrameTime = 0.0f;
        float m_MinFrameTime = 1000.0f;
        float m_MaxFrameTime = 0.0f;
        uint32_t m_FPS = 0;
        
        // Frame time history for graph
        std::deque<float> m_FrameTimeHistory;
        static constexpr size_t MAX_HISTORY = 300;
        
        // System timings
        struct SystemTiming {
            std::string Name;
            float TimeMs = 0.0f;
            float AverageTimeMs = 0.0f;
            float MinTimeMs = 1000.0f;
            float MaxTimeMs = 0.0f;
            uint32_t CallCount = 0;
        };
        std::unordered_map<std::string, SystemTiming> m_SystemTimings;
        
        // Memory stats
        size_t m_TotalMemory = 0;
        size_t m_UsedMemory = 0;
        size_t m_FreeMemory = 0;
    };

}
