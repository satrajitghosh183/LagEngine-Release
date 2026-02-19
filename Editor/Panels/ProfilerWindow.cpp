#include "ProfilerWindow.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Graphics/Renderer3D.hpp"
#include <imgui.h>
#include <algorithm>
#include <vector>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#else
#include <fstream>
#include <sstream>
#endif

namespace GameEngine {

    ProfilerWindow::ProfilerWindow() {
        m_FrameTimeHistory.resize(MAX_HISTORY, 0.0f);
    }

    void ProfilerWindow::OnImGuiRender() {
        ImGui::Begin("Profiler", &m_IsOpen);

        RenderFrameStats();
        ImGui::Separator();
        RenderSystemTimings();
        ImGui::Separator();
        RenderMemoryStats();
        ImGui::Separator();
        RenderTimeline();

        ImGui::End();
    }

    void ProfilerWindow::RecordTiming(const std::string& systemName, float timeMs) {
        auto& timing = m_SystemTimings[systemName];
        timing.Name = systemName;
        timing.TimeMs = timeMs;
        
        // Update statistics
        timing.AverageTimeMs = (timing.AverageTimeMs * 0.9f) + (timeMs * 0.1f); // Exponential moving average
        timing.MinTimeMs = std::min(timing.MinTimeMs, timeMs);
        timing.MaxTimeMs = std::max(timing.MaxTimeMs, timeMs);
        timing.CallCount++;
    }

    void ProfilerWindow::RecordFrameTime(float frameTimeMs) {
        m_CurrentFrameTime = frameTimeMs;
        m_AverageFrameTime = (m_AverageFrameTime * 0.9f) + (frameTimeMs * 0.1f);
        m_MinFrameTime = std::min(m_MinFrameTime, frameTimeMs);
        m_MaxFrameTime = std::max(m_MaxFrameTime, frameTimeMs);
        
        if (frameTimeMs > 0.0f) {
            m_FPS = static_cast<uint32_t>(1000.0f / frameTimeMs);
        }
        
        // Add to history
        m_FrameTimeHistory.push_back(frameTimeMs);
        if (m_FrameTimeHistory.size() > MAX_HISTORY) {
            m_FrameTimeHistory.pop_front();
        }
    }

    void ProfilerWindow::RenderFrameStats() {
        ImGui::Text("Frame Statistics");
        
        ImGui::Text("FPS: %u", m_FPS);
        ImGui::Text("Frame Time: %.2f ms", m_CurrentFrameTime);
        ImGui::Text("Average: %.2f ms", m_AverageFrameTime);
        ImGui::Text("Min: %.2f ms", m_MinFrameTime);
        ImGui::Text("Max: %.2f ms", m_MaxFrameTime);
        
        // Frame time graph (copy deque to vector - deque has no data() for contiguous access)
        if (!m_FrameTimeHistory.empty()) {
            std::vector<float> history(m_FrameTimeHistory.begin(), m_FrameTimeHistory.end());
            ImGui::PlotLines("Frame Time (ms)", history.data(),
                           static_cast<int>(history.size()),
                           0, nullptr, 0.0f, 50.0f, ImVec2(0, 80));
        }
    }

    void ProfilerWindow::RenderSystemTimings() {
        ImGui::Text("System Timings");
        
        if (ImGui::BeginTable("Timings", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("System");
            ImGui::TableSetupColumn("Current (ms)");
            ImGui::TableSetupColumn("Average (ms)");
            ImGui::TableSetupColumn("Min (ms)");
            ImGui::TableSetupColumn("Max (ms)");
            ImGui::TableHeadersRow();
            
            for (auto& [name, timing] : m_SystemTimings) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", timing.Name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", timing.TimeMs);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", timing.AverageTimeMs);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", timing.MinTimeMs);
                ImGui::TableNextColumn();
                ImGui::Text("%.2f", timing.MaxTimeMs);
            }
            
            ImGui::EndTable();
        }
    }

    static size_t GetProcessMemoryBytes() {
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS_EX pmc = {};
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
            return pmc.WorkingSetSize;
        return 0;
#else
        std::ifstream f("/proc/self/status");
        std::string line;
        while (std::getline(f, line)) {
            if (line.compare(0, 6, "VmRSS:") == 0) {
                size_t kb = 0;
                std::istringstream iss(line.substr(6));
                if (iss >> kb) return kb * 1024;
                break;
            }
        }
        return 0;
#endif
    }

    void ProfilerWindow::RenderMemoryStats() {
        ImGui::Text("Memory Statistics");
        m_UsedMemory = GetProcessMemoryBytes();
        ImGui::Text("Process: %.2f MB", m_UsedMemory / (1024.0f * 1024.0f));
    }

    void ProfilerWindow::RenderTimeline() {
        ImGui::Text("Timeline");
        // TODO: Implement timeline visualization
        ImGui::Text("Timeline view not yet implemented");
    }

}
