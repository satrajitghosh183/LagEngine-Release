#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <functional>
#include "BottleneckType.h"

class AIBackend;

class PipelineController {
public:
    PipelineController();
    
    // Update with frame metrics
    void updateFrame(float cpuTime, float gpuTime, float frameTime, float gpuUtilization);
    
    // Get current policy decisions
    BottleneckType getBottleneck() const { return m_bottleneck; }
    int getOptimalStreamCount() const { return m_optimalStreamCount; }
    bool shouldInjectExtraCompute() const { return m_shouldInjectExtraCompute; }
    
    // Enable/disable adaptive policy
    void setAdaptive(bool enabled) { m_adaptiveEnabled = enabled; }
    bool isAdaptive() const { return m_adaptiveEnabled; }
    
    // Get EMA estimates
    float getEMACPU() const { return m_emaCPU; }
    float getEMAGPU() const { return m_emaGPU; }
    float getEMAUtil() const { return m_emaUtil; }
    
    // Set stream count limits
    void setStreamCountLimits(int min, int max);
    
    // Reset
    void reset();
    
    // AI Backend integration
    std::shared_ptr<AIBackend> getAIBackend();
    
    // Set scheduler callback for stream adaptation (called from main loop)
    void setSchedulerAdaptCallback(std::function<void(int)> callback);

private:
    std::shared_ptr<AIBackend> m_aiBackend;
    // Exponential moving averages
    float m_emaCPU;
    float m_emaGPU;
    float m_emaUtil;
    float m_alpha; // EMA smoothing factor
    
    // State machine
    enum class UtilState {
        LowUtil,    // < 0.6
        MediumUtil, // 0.6 - 0.9
        HighUtil    // > 0.9
    };
    UtilState m_currentState;
    float m_hysteresisLow;
    float m_hysteresisHigh;
    
    BottleneckType m_bottleneck;
    int m_optimalStreamCount;
    int m_minStreams;
    int m_maxStreams;
    bool m_shouldInjectExtraCompute;
    
    bool m_adaptiveEnabled;
    
    // Callback to scheduler for stream adaptation
    std::function<void(int)> m_schedulerAdaptCallback;
    
    void updateEMA(float cpuTime, float gpuTime, float utilization);
    void updateBottleneck();
    void updateStreamCount();
    void updateStateMachine();
    float getCurrentUtilization() const;
};

