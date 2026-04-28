#include "UI.h"
#include "Profiler.h"
#include "AIBackend.h"
#include "VulkanBase.hpp"
#include <imgui.h>
#include <algorithm>
#include <sstream>

UI::UI()
    : m_vkBase(nullptr)
    , m_initialized(false)
{
}

UI::~UI() {
    shutdown();
}

bool UI::initialize(vkdemo::VulkanBase* vkBase) {
    m_vkBase = vkBase;
    // ImGui is initialized by VulkanBase when EnableImGui=true
    m_initialized = true;
    return true;
}

void UI::shutdown() {
    // VulkanBase handles ImGui shutdown
    m_initialized = false;
}

void UI::beginFrame() {
    if (!m_initialized || !m_vkBase) return;
    m_vkBase->ImGuiNewFrame();
}

void UI::endFrame(VkCommandBuffer cmd) {
    if (!m_initialized || !m_vkBase) return;
    m_vkBase->ImGuiRender(cmd);
}

void UI::render(DAGScheduler* scheduler, PipelineController* controller,
                RenderGraph* graph, float frameTime, float fps) {
    if (!m_initialized) return;
    renderMainWindow(scheduler, controller, graph, frameTime, fps);
}

void UI::renderMainWindow(DAGScheduler* scheduler, PipelineController* controller,
                         RenderGraph* graph, float frameTime, float fps) {
    if (!scheduler || !controller || !graph) return;

    ImGui::Begin("GPU Scheduling Lab");

    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", frameTime);
    ImGui::Separator();

    ImGui::Text("Scheduling Mode:");
    const char* modes[] = { "Baseline", "DAG Only", "DAG + Work Stealing", "Full System" };
    int currentMode = (int)scheduler->getMode();
    if (ImGui::Combo("##Mode", &currentMode, modes, 4)) {
        scheduler->setMode((SchedulingMode)currentMode);
        if ((SchedulingMode)currentMode >= SchedulingMode::DAGWorkStealing) {
            scheduler->enableWorkStealing(true);
        } else {
            scheduler->enableWorkStealing(false);
        }
        if ((SchedulingMode)currentMode == SchedulingMode::FullSystem) {
            controller->setAdaptive(true);
        } else {
            controller->setAdaptive(false);
        }
    }
    ImGui::Separator();

    if (scheduler->isWorkStealingEnabled()) {
        auto cpuStats = scheduler->getCPUWorkerStats();
        int totalSteals = 0;
        int totalAttempts = 0;
        for (const auto& s : cpuStats) {
            totalSteals += s.successfulSteals;
            totalAttempts += s.stealAttempts;
        }
        ImGui::Text("Work Stealing: ON");
        ImGui::Text("Successful Steals: %d", totalSteals);
        ImGui::Text("Steal Attempts: %d", totalAttempts);
    } else {
        ImGui::Text("Work Stealing: OFF");
    }
    ImGui::Separator();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("=== AI BACKEND - Predictive Pipeline Controller ===");
    ImGui::PopStyleColor();

    auto aiBackend = controller->getAIBackend();
    if (aiBackend && aiBackend->isEnabled()) {
        auto patterns = aiBackend->detectPatterns();

        FrameMetrics currentMetrics;
        currentMetrics.cpuTime = controller->getEMACPU();
        currentMetrics.gpuTime = controller->getEMAGPU();
        currentMetrics.frameTime = frameTime;
        currentMetrics.gpuUtilization = controller->getEMAUtil();
        currentMetrics.streamCount = controller->getOptimalStreamCount();

        auto prediction = aiBackend->analyzeAndPredict(currentMetrics);

        ImGui::Text("AI Status: ACTIVE");
        ImGui::Text("Confidence: %.1f%%", prediction.confidence * 100.0f);

        ImGui::Separator();
        ImGui::Text("AI Recommendation:");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text("  Recommended Streams: %d", prediction.recommendedStreamCount);
        ImGui::Text("  Current Streams: %d", controller->getOptimalStreamCount());
        if (prediction.recommendedStreamCount != controller->getOptimalStreamCount()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
            ImGui::Text("  [AI WANTS TO CHANGE STREAMS!]");
            ImGui::PopStyleColor();
        }
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Text("AI Reasoning:");
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
        ImGui::TextWrapped("%s", prediction.reasoning.c_str());
        ImGui::PopStyleColor();

        ImGui::Separator();
        ImGui::Text("Pattern Analysis:");
        ImGui::Text("  Trend: %.4f %s", patterns.trend, patterns.trend < 0 ? "(degrading)" : patterns.trend > 0 ? "(improving)" : "(stable)");
        ImGui::Text("  Volatility: %.3f %s", patterns.volatility, patterns.volatility > 0.2f ? "[HIGH]" : "[LOW]");
        ImGui::Text("  CPU-GPU Correlation: %.3f", patterns.correlation);
        if (patterns.isAnomaly) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.5f, 0.0f, 1.0f));
            ImGui::Text("  [ANOMALY DETECTED]");
            ImGui::PopStyleColor();
        }

        ImGui::Separator();
        ImGui::Text("Historical Data Points: %zu", aiBackend->getHistory().size());
    } else {
        ImGui::Text("AI Status: DISABLED");
    }

    ImGui::Separator();

    ImGui::Text("Pipeline Controller:");
    const char* bottlenecks[] = { "CPU Bound", "GPU Underutilized", "Balanced" };
    ImGui::Text("Bottleneck: %s", bottlenecks[(int)controller->getBottleneck()]);
    ImGui::Text("Optimal Streams: %d", controller->getOptimalStreamCount());
    ImGui::Text("EMA CPU: %.3f ms", controller->getEMACPU());
    ImGui::Text("EMA GPU: %.3f ms", controller->getEMAGPU());
    ImGui::Text("EMA Utilization: %.1f%%", controller->getEMAUtil() * 100.0f);
    ImGui::Separator();

    renderPerPassChart(scheduler);
    ImGui::Separator();

    renderUtilizationBars(scheduler, controller);
    ImGui::Separator();

    renderTimeline(scheduler);

    ImGui::End();
}

void UI::renderPerPassChart(DAGScheduler* scheduler) {
    if (!scheduler) return;

    ImGui::Text("Per-Pass Timing:");

    auto& profiler = Profiler::instance();
    auto& frameData = profiler.getFrameData();

    float maxDuration = 0.0f;
    for (const auto& data : frameData) {
        maxDuration = std::max(maxDuration, data.duration);
    }

    if (maxDuration > 0.0f) {
        for (const auto& data : frameData) {
            float ratio = data.duration / maxDuration;
            ImGui::ProgressBar(ratio, ImVec2(0.0f, 0.0f), data.name.c_str());
            ImGui::SameLine();
            ImGui::Text("%.3f ms", data.duration);
        }
    }
}

void UI::renderUtilizationBars(DAGScheduler* scheduler, PipelineController* controller) {
    if (!scheduler || !controller) return;

    ImGui::Text("Utilization:");

    float gpuUtil = controller->getEMAUtil();
    ImGui::ProgressBar(gpuUtil, ImVec2(0.0f, 0.0f), "GPU");
    ImGui::SameLine();
    ImGui::Text("%.1f%%", gpuUtil * 100.0f);

    float frameTime = Profiler::instance().getFrameTime();
    float targetFrameTime = 16.67f;
    float cpuUtil = std::min(1.0f, targetFrameTime / frameTime);
    ImGui::ProgressBar(cpuUtil, ImVec2(0.0f, 0.0f), "CPU Submission");
    ImGui::SameLine();
    ImGui::Text("%.1f%%", cpuUtil * 100.0f);
}

void UI::renderTimeline(DAGScheduler* scheduler) {
    if (!scheduler) return;

    ImGui::Text("Timeline (simplified):");
    ImGui::Text("Timeline visualization coming soon...");
}
