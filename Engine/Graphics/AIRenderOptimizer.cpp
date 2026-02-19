#include "AIRenderOptimizer.hpp"
#include <algorithm>

namespace GameEngine {

    void AIRenderOptimizer::RecordFrame(const FrameMetrics& metrics) {
        if (!Enabled) return;
        m_FrameCount++;

        if (m_FrameCount == 1) {
            m_ShortEMA_CPU = m_LongEMA_CPU = metrics.cpuTimeMs;
            m_ShortEMA_GPU = m_LongEMA_GPU = metrics.gpuTimeMs;
            m_ShortEMA_Frame = metrics.frameTimeMs;
            m_AvgGPUUtil = metrics.gpuUtilization;
        } else {
            m_ShortEMA_CPU = SHORT_ALPHA * metrics.cpuTimeMs + (1.0f - SHORT_ALPHA) * m_ShortEMA_CPU;
            m_LongEMA_CPU = LONG_ALPHA * metrics.cpuTimeMs + (1.0f - LONG_ALPHA) * m_LongEMA_CPU;
            m_ShortEMA_GPU = SHORT_ALPHA * metrics.gpuTimeMs + (1.0f - SHORT_ALPHA) * m_ShortEMA_GPU;
            m_LongEMA_GPU = LONG_ALPHA * metrics.gpuTimeMs + (1.0f - LONG_ALPHA) * m_LongEMA_GPU;
            m_ShortEMA_Frame = SHORT_ALPHA * metrics.frameTimeMs + (1.0f - SHORT_ALPHA) * m_ShortEMA_Frame;
            m_AvgGPUUtil = LONG_ALPHA * metrics.gpuUtilization + (1.0f - LONG_ALPHA) * m_AvgGPUUtil;
        }
    }

    AIRenderOptimizer::Prediction AIRenderOptimizer::Analyze() const {
        Prediction pred;
        if (m_FrameCount < 10) {
            pred.reasoning = "Insufficient data (need 10+ frames)";
            pred.confidence = 0.0f;
            return pred;
        }

        float cpuRatio = m_ShortEMA_CPU / std::max(m_ShortEMA_Frame, 0.001f);
        float gpuRatio = m_ShortEMA_GPU / std::max(m_ShortEMA_Frame, 0.001f);

        if (cpuRatio > 0.75f) {
            pred.bottleneck = Bottleneck::CPUBound;
            pred.recommendedStreamCount = std::clamp(static_cast<int>(cpuRatio * 6.0f), 2, 8);
            pred.shouldInjectCompute = false;
            pred.confidence = std::min(cpuRatio, 1.0f);
            pred.reasoning = "CPU bound: CPU takes " + std::to_string(int(cpuRatio * 100)) + "% of frame time";
        } else if (m_AvgGPUUtil < 0.55f && gpuRatio < 0.5f) {
            pred.bottleneck = Bottleneck::GPUUnderutilized;
            pred.recommendedStreamCount = std::clamp(static_cast<int>((1.0f - m_AvgGPUUtil) * 6.0f), 3, 8);
            pred.shouldInjectCompute = true;
            pred.confidence = 1.0f - m_AvgGPUUtil;
            pred.reasoning = "GPU underutilized at " + std::to_string(int(m_AvgGPUUtil * 100)) + "%";
        } else if (gpuRatio > 0.75f) {
            pred.bottleneck = Bottleneck::GPUBound;
            pred.recommendedStreamCount = 2;
            pred.shouldInjectCompute = false;
            pred.confidence = std::min(gpuRatio, 1.0f);
            pred.reasoning = "GPU bound: GPU takes " + std::to_string(int(gpuRatio * 100)) + "% of frame time";
        } else {
            pred.bottleneck = Bottleneck::Balanced;
            pred.recommendedStreamCount = 3;
            pred.shouldInjectCompute = false;
            pred.confidence = 0.8f;
            pred.reasoning = "Balanced workload";
        }

        return pred;
    }

    void AIRenderOptimizer::Reset() {
        m_FrameCount = 0;
        m_ShortEMA_CPU = m_LongEMA_CPU = 0.0f;
        m_ShortEMA_GPU = m_LongEMA_GPU = 0.0f;
        m_ShortEMA_Frame = 0.0f;
        m_AvgGPUUtil = 0.0f;
    }

}
