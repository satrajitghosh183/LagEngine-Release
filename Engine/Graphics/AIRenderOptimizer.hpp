#pragma once

#include <string>

namespace GameEngine {

    class AIRenderOptimizer {
    public:
        struct FrameMetrics {
            float cpuTimeMs = 0.0f;
            float gpuTimeMs = 0.0f;
            float frameTimeMs = 0.0f;
            float gpuUtilization = 0.0f;
        };

        enum class Bottleneck { CPUBound, GPUBound, GPUUnderutilized, Balanced };

        struct Prediction {
            Bottleneck bottleneck = Bottleneck::Balanced;
            int recommendedStreamCount = 2;
            bool shouldInjectCompute = false;
            float confidence = 0.0f;
            std::string reasoning;
        };

        void RecordFrame(const FrameMetrics& metrics);
        Prediction Analyze() const;
        void Reset();

        bool Enabled = false;

    private:
        float m_ShortEMA_CPU = 0.0f, m_LongEMA_CPU = 0.0f;
        float m_ShortEMA_GPU = 0.0f, m_LongEMA_GPU = 0.0f;
        float m_ShortEMA_Frame = 0.0f;
        float m_AvgGPUUtil = 0.0f;
        int m_FrameCount = 0;

        static constexpr float SHORT_ALPHA = 0.15f;
        static constexpr float LONG_ALPHA = 0.05f;
    };

}
