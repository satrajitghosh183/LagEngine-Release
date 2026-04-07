#include "PipelineController.h"
#include "AIBackend.h"
#include <algorithm>
#include <cmath>
#include <memory>

PipelineController::PipelineController()
    : m_emaCPU(0.0f)
    , m_emaGPU(0.0f)
    , m_emaUtil(0.0f)
    , m_alpha(0.1f)
    , m_currentState(UtilState::MediumUtil)
    , m_hysteresisLow(0.6f)
    , m_hysteresisHigh(0.9f)
    , m_bottleneck(BottleneckType::Balanced)
    , m_optimalStreamCount(4)
    , m_minStreams(2)
    , m_maxStreams(8)
    , m_shouldInjectExtraCompute(false)
    , m_adaptiveEnabled(true)
    , m_aiBackend(std::make_shared<AIBackend>())
    , m_schedulerAdaptCallback(nullptr)
{
}

void PipelineController::updateFrame(float cpuTime, float gpuTime, float frameTime, float gpuUtilization) {
    updateEMA(cpuTime, gpuTime, gpuUtilization);
    
    // Record frame for AI backend
    if (m_aiBackend && m_adaptiveEnabled) {
        FrameMetrics metrics;
        metrics.cpuTime = cpuTime;
        metrics.gpuTime = gpuTime;
        metrics.frameTime = frameTime;
        metrics.gpuUtilization = gpuUtilization;
        metrics.streamCount = m_optimalStreamCount;
        metrics.timestamp = frameTime; // Simplified
        
        m_aiBackend->recordFrame(metrics);
        
        // Get AI prediction
        auto prediction = m_aiBackend->analyzeAndPredict(metrics);
        
        // Use AI recommendations when confidence is high
        if (prediction.confidence > 0.6f && m_adaptiveEnabled) {
            m_bottleneck = prediction.predictedBottleneck;
            int oldStreamCount = m_optimalStreamCount;
            m_optimalStreamCount = std::clamp(prediction.recommendedStreamCount, m_minStreams, m_maxStreams);
            m_shouldInjectExtraCompute = prediction.shouldInjectCompute;
            
            // Notify scheduler to adapt stream count if it changed
            if (m_optimalStreamCount != oldStreamCount && m_schedulerAdaptCallback) {
                m_schedulerAdaptCallback(m_optimalStreamCount);
            }
        } else {
            // Fall back to rule-based system
            updateBottleneck();
            updateStateMachine();
            int oldStreamCount = m_optimalStreamCount;
            updateStreamCount();
            
            // Notify scheduler if stream count changed
            if (m_optimalStreamCount != oldStreamCount && m_schedulerAdaptCallback) {
                m_schedulerAdaptCallback(m_optimalStreamCount);
            }
        }
    } else {
        // Use rule-based system only
        updateBottleneck();
        updateStateMachine();
        updateStreamCount();
    }
}

void PipelineController::updateEMA(float cpuTime, float gpuTime, float utilization) {
    if (m_emaCPU == 0.0f && m_emaGPU == 0.0f) {
        // First frame
        m_emaCPU = cpuTime;
        m_emaGPU = gpuTime;
        m_emaUtil = utilization;
    } else {
        m_emaCPU = m_alpha * cpuTime + (1.0f - m_alpha) * m_emaCPU;
        m_emaGPU = m_alpha * gpuTime + (1.0f - m_alpha) * m_emaGPU;
        m_emaUtil = m_alpha * utilization + (1.0f - m_alpha) * m_emaUtil;
    }
}

void PipelineController::updateBottleneck() {
    if (!m_adaptiveEnabled) {
        m_bottleneck = BottleneckType::Balanced;
        return;
    }
    
    const float cpuBoundEps = 0.002f; // 2ms difference threshold
    const float utilLowThresh = 0.6f;
    
    if (m_emaCPU > m_emaGPU + cpuBoundEps) {
        m_bottleneck = BottleneckType::CPUBound;
    } else if (m_emaUtil < utilLowThresh) {
        m_bottleneck = BottleneckType::GPUUnderutilized;
    } else {
        m_bottleneck = BottleneckType::Balanced;
    }
}

void PipelineController::updateStateMachine() {
    if (!m_adaptiveEnabled) {
        return;
    }
    
    float util = getCurrentUtilization();
    
    switch (m_currentState) {
        case UtilState::LowUtil:
            if (util >= m_hysteresisLow) {
                m_currentState = UtilState::MediumUtil;
            }
            break;
        case UtilState::MediumUtil:
            if (util < m_hysteresisLow) {
                m_currentState = UtilState::LowUtil;
            } else if (util > m_hysteresisHigh) {
                m_currentState = UtilState::HighUtil;
            }
            break;
        case UtilState::HighUtil:
            if (util <= m_hysteresisHigh) {
                m_currentState = UtilState::MediumUtil;
            }
            break;
    }
}

void PipelineController::updateStreamCount() {
    if (!m_adaptiveEnabled) {
        return;
    }
    
    switch (m_bottleneck) {
        case BottleneckType::CPUBound:
            // Reduce streams to reduce submission overhead
            m_optimalStreamCount = std::max(m_minStreams, m_optimalStreamCount - 1);
            m_shouldInjectExtraCompute = false;
            break;
            
        case BottleneckType::GPUUnderutilized:
            // Increase streams to better utilize GPU
            m_optimalStreamCount = std::min(m_maxStreams, m_optimalStreamCount + 1);
            // Enable extra compute when GPU is underutilized
            m_shouldInjectExtraCompute = (m_currentState == UtilState::LowUtil);
            break;
            
        case BottleneckType::Balanced:
            // Keep current settings, maybe slight adjustment
            if (m_currentState == UtilState::LowUtil) {
                m_optimalStreamCount = std::min(m_maxStreams, m_optimalStreamCount + 1);
            } else if (m_currentState == UtilState::HighUtil) {
                m_optimalStreamCount = std::max(m_minStreams, m_optimalStreamCount - 1);
            }
            m_shouldInjectExtraCompute = false;
            break;
    }
}

float PipelineController::getCurrentUtilization() const {
    return m_emaUtil;
}

void PipelineController::setStreamCountLimits(int min, int max) {
    m_minStreams = std::max(1, min);
    m_maxStreams = std::max(m_minStreams, max);
    m_optimalStreamCount = std::clamp(m_optimalStreamCount, m_minStreams, m_maxStreams);
}

void PipelineController::reset() {
    m_emaCPU = 0.0f;
    m_emaGPU = 0.0f;
    m_emaUtil = 0.0f;
    m_bottleneck = BottleneckType::Balanced;
    m_optimalStreamCount = 4;
    m_currentState = UtilState::MediumUtil;
    m_shouldInjectExtraCompute = false;
    if (m_aiBackend) {
        m_aiBackend->reset();
    }
}

std::shared_ptr<AIBackend> PipelineController::getAIBackend() {
    return m_aiBackend;
}

void PipelineController::setSchedulerAdaptCallback(std::function<void(int)> callback) {
    m_schedulerAdaptCallback = callback;
}

