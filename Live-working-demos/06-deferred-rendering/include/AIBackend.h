#pragma once

#include <vector>
#include <deque>
#include <cmath>
#include <algorithm>
#include <memory>
#include <string>
#include "BottleneckType.h"

// AI Backend - Predictive Pipeline Bubble Management System
// This implements intelligent, adaptive scheduling using pattern recognition
// and predictive modeling without requiring machine learning training

struct FrameMetrics {
    float cpuTime;
    float gpuTime;
    float frameTime;
    float gpuUtilization;
    int streamCount;
    float timestamp;
};

class AIBackend {
public:
    AIBackend();
    
    // Main prediction and decision interface
    struct Prediction {
        BottleneckType predictedBottleneck;
        int recommendedStreamCount;
        bool shouldInjectCompute;
        float confidence;
        float predictedFrameTime;
        std::string reasoning;
    };
    
    Prediction analyzeAndPredict(const FrameMetrics& currentMetrics);
    
    // Update with frame metrics (learning/training phase)
    void recordFrame(const FrameMetrics& metrics);
    
    // Enable/disable AI backend
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    // Get historical data for visualization
    const std::deque<FrameMetrics>& getHistory() const { return m_history; }
    
    // Pattern recognition
    struct Pattern {
        float trend;           // Positive = improving, negative = degrading
        float volatility;      // How much metrics fluctuate
        float correlation;     // CPU-GPU correlation
        bool isAnomaly;        // Detected anomaly
    };
    
    Pattern detectPatterns() const;
    
    // Get confidence score
    float getConfidence() const { return m_confidence; }
    
    // Reset learning
    void reset();
    
private:
    bool m_enabled;
    
    // Historical data window
    std::deque<FrameMetrics> m_history;
    static constexpr size_t HISTORY_SIZE = 120; // ~2 seconds at 60 FPS
    
    // Exponential moving averages with different time constants
    float m_emaShortCPU;
    float m_emaShortGPU;
    float m_emaLongCPU;
    float m_emaLongGPU;
    
    // Trend detection
    float m_cpuTrend;
    float m_gpuTrend;
    
    // Pattern matching
    float m_confidence;
    
    // Predictive model parameters
    struct ModelParams {
        float cpuWeight;
        float gpuWeight;
        float utilWeight;
        float streamOverhead;
        float optimalUtilization;
    };
    ModelParams m_model;
    
    // Learning/adaptation
    void updateModel();
    float predictFrameTime(int streamCount, float cpuLoad, float gpuLoad) const;
    int findOptimalStreamCount(float cpuLoad, float gpuLoad) const;
    
    // Pattern analysis
    float calculateTrend(const std::vector<float>& values) const;
    float calculateVolatility(const std::vector<float>& values) const;
    float calculateCorrelation(const std::vector<float>& cpu, const std::vector<float>& gpu) const;
    bool detectAnomaly(const FrameMetrics& metrics) const;
    
    // Intelligent decision making
    std::string generateReasoning(const Prediction& pred, const FrameMetrics& metrics) const;
};

