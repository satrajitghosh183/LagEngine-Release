#include "UI.h"
#include "Profiler.h"
#include "AIBackend.h"
#include <imgui.h>
#ifndef IMGUI_IMPL_OPENGL_LOADER_GLAD
#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#endif
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>
#include <algorithm>
#include <sstream>

UI::UI()
    : m_initialized(false)
{
}

UI::~UI() {
    shutdown();
}

bool UI::initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    // Use GLSL 420 (compatible with OpenGL 4.2+)
    ImGui_ImplOpenGL3_Init("#version 420");
    
    m_initialized = true;
    return true;
}

void UI::shutdown() {
    if (m_initialized) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_initialized = false;
    }
}

void UI::beginFrame() {
    if (!m_initialized) return;
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame() {
    if (!m_initialized) return;
    
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
    
    // FPS and frame time
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.3f ms", frameTime);
    ImGui::Separator();
    
    // Scheduling mode selector
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
    
    // Work stealing info (from CPU workers)
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
    
    // AI Backend section - THE AI STUFF!
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("=== AI BACKEND - Predictive Pipeline Controller ===");
    ImGui::PopStyleColor();
    
    auto aiBackend = controller->getAIBackend();
    if (aiBackend && aiBackend->isEnabled()) {
        auto patterns = aiBackend->detectPatterns();
        
        // Get current metrics for AI analysis
        FrameMetrics currentMetrics;
        currentMetrics.cpuTime = controller->getEMACPU();
        currentMetrics.gpuTime = controller->getEMAGPU();
        currentMetrics.frameTime = frameTime;
        currentMetrics.gpuUtilization = controller->getEMAUtil();
        currentMetrics.streamCount = controller->getOptimalStreamCount();
        
        // Get AI prediction
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
    
    // Pipeline controller info
    ImGui::Text("Pipeline Controller:");
    const char* bottlenecks[] = { "CPU Bound", "GPU Underutilized", "Balanced" };
    ImGui::Text("Bottleneck: %s", bottlenecks[(int)controller->getBottleneck()]);
    ImGui::Text("Optimal Streams: %d", controller->getOptimalStreamCount());
    ImGui::Text("EMA CPU: %.3f ms", controller->getEMACPU());
    ImGui::Text("EMA GPU: %.3f ms", controller->getEMAGPU());
    ImGui::Text("EMA Utilization: %.1f%%", controller->getEMAUtil() * 100.0f);
    ImGui::Separator();
    
    // Per-pass chart
    renderPerPassChart(scheduler);
    ImGui::Separator();
    
    // Utilization bars
    renderUtilizationBars(scheduler, controller);
    ImGui::Separator();
    
    // Timeline
    renderTimeline(scheduler);
    
    ImGui::End();
}

void UI::renderPerPassChart(DAGScheduler* scheduler) {
    if (!scheduler) return;
    
    ImGui::Text("Per-Pass Timing:");
    
    auto& profiler = Profiler::instance();
    auto& frameData = profiler.getFrameData();
    
    // Find max duration for scaling
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
    
    // GPU utilization (rough estimate)
    float gpuUtil = controller->getEMAUtil();
    ImGui::ProgressBar(gpuUtil, ImVec2(0.0f, 0.0f), "GPU");
    ImGui::SameLine();
    ImGui::Text("%.1f%%", gpuUtil * 100.0f);
    
    // CPU submission (based on frame time)
    float frameTime = Profiler::instance().getFrameTime();
    float targetFrameTime = 16.67f; // 60 FPS
    float cpuUtil = std::min(1.0f, targetFrameTime / frameTime);
    ImGui::ProgressBar(cpuUtil, ImVec2(0.0f, 0.0f), "CPU Submission");
    ImGui::SameLine();
    ImGui::Text("%.1f%%", cpuUtil * 100.0f);
}

void UI::renderTimeline(DAGScheduler* scheduler) {
    if (!scheduler) return;
    
    ImGui::Text("Timeline (simplified):");
    // TODO: Implement visual timeline with colored blocks
    ImGui::Text("Timeline visualization coming soon...");
}

