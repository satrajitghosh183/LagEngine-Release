#pragma once

#include "EditorPanel.hpp"
#include <vector>
#include <deque>
#include <chrono>

namespace editor {

/**
 * @brief Performance Monitor Panel - shows FPS, memory, and profiling data
 * Similar to Unity's Profiler or LumixEngine's Performance window
 */
class PerformancePanel : public EditorPanel {
public:
    PerformancePanel();
    
    void render() override;
    void update(float dt) override;
    
    // Add timing samples
    void addFrameTime(float ms);
    void addPhysicsTime(float ms);
    void addRenderTime(float ms);

private:
    void renderFrameTimeGraph();
    void renderMemoryStats();
    void renderSystemInfo();
    void renderDetailedTimings();
    void renderPhysicsStats();
    
    // Frame time history
    static constexpr size_t HISTORY_SIZE = 120;
    std::deque<float> m_frameTimeHistory;
    std::deque<float> m_physicsTimeHistory;
    std::deque<float> m_renderTimeHistory;
    
    // Current stats
    float m_currentFps = 0.0f;
    float m_avgFps = 0.0f;
    float m_minFps = 0.0f;
    float m_maxFps = 0.0f;
    
    float m_currentFrameTime = 0.0f;
    float m_avgFrameTime = 0.0f;
    
    // Memory stats (demo values)
    size_t m_usedMemory = 0;
    size_t m_totalMemory = 0;
    size_t m_gpuMemory = 0;
    
    // Timing breakdown
    float m_physicsTime = 0.0f;
    float m_renderTime = 0.0f;
    float m_updateTime = 0.0f;
    float m_imguiTime = 0.0f;
    
    // Physics stats
    int m_activeRigidBodies = 0;
    int m_sleepingBodies = 0;
    int m_particleCount = 0;
    int m_springCount = 0;
    int m_contactCount = 0;
    
    // Timing
    std::chrono::high_resolution_clock::time_point m_lastUpdate;
    float m_updateInterval = 0.1f;  // Update stats every 100ms
    float m_timeSinceUpdate = 0.0f;
    
    // Graph settings
    float m_graphScale = 33.33f;  // Max ms to show (30 fps)
    bool m_showDetailed = false;
    int m_selectedGraph = 0;  // 0=FPS, 1=Frame Time, 2=Memory
};

} // namespace editor

