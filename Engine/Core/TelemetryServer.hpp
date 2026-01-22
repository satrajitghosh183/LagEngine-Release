#pragma once

#include "Runtime/ITelemetryServer.hpp"
#include <vector>
#include <deque>
#include <filesystem>

namespace GameEngine {

    /**
     * @brief Telemetry server implementation
     * 
     * Collects frame metrics for RL subsystem
     * Basic implementation - full implementation in Phase 10
     */
    class TelemetryServer : public ITelemetryServer {
    public:
        TelemetryServer();
        ~TelemetryServer() override = default;

        void Initialize() override;
        void Shutdown() override;

        void RecordFrameMetrics(const FrameMetrics& metrics) override;
        void RecordCPUTiming(const std::string& stage, float ms) override;
        void RecordGPUTiming(const std::string& pass, float ms) override;

        void ExportDataset(const std::filesystem::path& path) override;

    private:
        // Ring buffer for 60 seconds @ 60 FPS = 3600 samples
        static constexpr size_t MAX_SAMPLES = 3600;
        std::deque<FrameMetrics> m_FrameMetrics;
        std::deque<std::pair<std::string, float>> m_CPUTimings;
        std::deque<std::pair<std::string, float>> m_GPUTimings;
    };

}

