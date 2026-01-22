#pragma once

#include "../Base.hpp"
#include <string>
#include <filesystem>

namespace GameEngine {

    /**
     * @brief Telemetry server interface
     * 
     * Provides metrics collection without exposing implementation details
     */
    class ITelemetryServer {
    public:
        virtual ~ITelemetryServer() = default;

        /**
         * @brief Initialize telemetry server
         */
        virtual void Initialize() = 0;

        /**
         * @brief Shutdown telemetry server
         */
        virtual void Shutdown() = 0;

        /**
         * @brief Record frame metrics
         */
        struct FrameMetrics {
            float totalFrameTime = 0.0f;
            float inputTime = 0.0f;
            float simulationTime = 0.0f;
            float physicsTime = 0.0f;
            float cullingTime = 0.0f;
            float renderGraphBuildTime = 0.0f;
            float shadowPassTime = 0.0f;
            float gbufferPassTime = 0.0f;
            float lightingPassTime = 0.0f;
            float postProcessTime = 0.0f;
            uint32_t drawCallCount = 0;
            uint32_t triangleCount = 0;
            uint32_t jobQueueDepth = 0;
            uint32_t renderQueueDepth = 0;
        };

        virtual void RecordFrameMetrics(const FrameMetrics& metrics) = 0;

        /**
         * @brief Record CPU timing for a stage
         */
        virtual void RecordCPUTiming(const std::string& stage, float ms) = 0;

        /**
         * @brief Record GPU timing for a pass
         */
        virtual void RecordGPUTiming(const std::string& pass, float ms) = 0;

        /**
         * @brief Export dataset to file
         */
        virtual void ExportDataset(const std::filesystem::path& path) = 0;
    };

}

