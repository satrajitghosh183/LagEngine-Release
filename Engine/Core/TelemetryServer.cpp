#include "TelemetryServer.hpp"
#include "Logger.hpp"
#include <fstream>

namespace GameEngine {

    TelemetryServer::TelemetryServer() {
    }

    void TelemetryServer::Initialize() {
        m_FrameMetrics.clear();
        m_CPUTimings.clear();
        m_GPUTimings.clear();
        GE_CORE_INFO("TelemetryServer initialized");
    }

    void TelemetryServer::Shutdown() {
        GE_CORE_INFO("TelemetryServer shutdown");
    }

    void TelemetryServer::RecordFrameMetrics(const FrameMetrics& metrics) {
        m_FrameMetrics.push_back(metrics);
        if (m_FrameMetrics.size() > MAX_SAMPLES) {
            m_FrameMetrics.pop_front();
        }
    }

    void TelemetryServer::RecordCPUTiming(const std::string& stage, float ms) {
        m_CPUTimings.push_back({stage, ms});
        if (m_CPUTimings.size() > MAX_SAMPLES) {
            m_CPUTimings.pop_front();
        }
    }

    void TelemetryServer::RecordGPUTiming(const std::string& pass, float ms) {
        m_GPUTimings.push_back({pass, ms});
        if (m_GPUTimings.size() > MAX_SAMPLES) {
            m_GPUTimings.pop_front();
        }
    }

    void TelemetryServer::ExportDataset(const std::filesystem::path& path) {
        // TODO: Implement binary export in Phase 10
        GE_CORE_INFO("Telemetry export to {0} (stub - full implementation in Phase 10)", path.string());
    }

}

