#include "PerformancePanel.hpp"
#include "../core/EditorTheme.hpp"
#include <imgui.h>
#include <algorithm>
#include <numeric>

namespace editor {

PerformancePanel::PerformancePanel() 
    : EditorPanel("Performance", true) {
    // Initialize with some demo data
    for (size_t i = 0; i < HISTORY_SIZE; i++) {
        float noise = static_cast<float>(rand() % 100) / 500.0f;
        m_frameTimeHistory.push_back(16.67f + noise);
        m_physicsTimeHistory.push_back(1.5f + noise * 0.5f);
        m_renderTimeHistory.push_back(8.0f + noise * 2.0f);
    }
    
    m_lastUpdate = std::chrono::high_resolution_clock::now();
    
    // Demo values
    m_usedMemory = 256 * 1024 * 1024;  // 256 MB
    m_totalMemory = 1024 * 1024 * 1024; // 1 GB
    m_gpuMemory = 512 * 1024 * 1024;    // 512 MB
    
    m_activeRigidBodies = 42;
    m_sleepingBodies = 15;
    m_particleCount = 400;
    m_springCount = 760;
    m_contactCount = 128;
}

void PerformancePanel::render() {
    if (!beginPanel()) {
        endPanel();
        return;
    }
    
    // FPS display with color coding
    float fps = m_currentFps > 0 ? m_currentFps : 60.0f;
    ImVec4 fpsColor;
    if (fps >= 60) {
        fpsColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
    } else if (fps >= 30) {
        fpsColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
    } else {
        fpsColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    }
    
    ImGui::PushStyleColor(ImGuiCol_Text, fpsColor);
    ImGui::Text("%.1f FPS", fps);
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::TextDisabled("(%.2f ms)", 1000.0f / fps);
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::Checkbox("Detailed", &m_showDetailed);
    
    ImGui::Separator();
    
    // Tab selection
    if (ImGui::BeginTabBar("PerformanceTabs")) {
        if (ImGui::BeginTabItem("Frame Time")) {
            renderFrameTimeGraph();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Memory")) {
            renderMemoryStats();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Physics")) {
            renderPhysicsStats();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("System")) {
            renderSystemInfo();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    if (m_showDetailed) {
        ImGui::Separator();
        renderDetailedTimings();
    }
    
    endPanel();
}

void PerformancePanel::renderFrameTimeGraph() {
    ImGui::Spacing();
    
    // Stats row
    ImGui::Columns(4, nullptr, false);
    
    ImGui::Text("Current");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "%.2f ms", m_currentFrameTime > 0 ? m_currentFrameTime : 16.67f);
    ImGui::NextColumn();
    
    ImGui::Text("Average");
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "%.2f ms", m_avgFrameTime > 0 ? m_avgFrameTime : 16.5f);
    ImGui::NextColumn();
    
    ImGui::Text("Min FPS");
    ImGui::TextColored(ImVec4(0.9f, 0.5f, 0.3f, 1.0f), "%.1f", m_minFps > 0 ? m_minFps : 58.0f);
    ImGui::NextColumn();
    
    ImGui::Text("Max FPS");
    ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%.1f", m_maxFps > 0 ? m_maxFps : 62.0f);
    ImGui::NextColumn();
    
    ImGui::Columns(1);
    
    ImGui::Spacing();
    
    // Frame time graph
    ImGui::Text("Frame Time (ms)");
    
    std::vector<float> frameData(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
    if (!frameData.empty()) {
        ImGui::PlotLines("##FrameTime", frameData.data(), static_cast<int>(frameData.size()),
                        0, nullptr, 0.0f, m_graphScale,
                        ImVec2(-1, 80));
    }
    
    // Target lines
    ImGui::TextDisabled("Target: 16.67ms (60 FPS) | 33.33ms (30 FPS)");
    
    ImGui::Spacing();
    
    // Timing breakdown
    if (ImGui::TreeNode("Timing Breakdown")) {
        ImGui::Spacing();
        
        // Stacked bar visualization
        float totalTime = m_physicsTime + m_renderTime + m_updateTime + m_imguiTime;
        if (totalTime <= 0) totalTime = 16.67f;
        
        float physicsWidth = (m_physicsTime > 0 ? m_physicsTime : 1.5f) / m_graphScale * ImGui::GetContentRegionAvail().x;
        float renderWidth = (m_renderTime > 0 ? m_renderTime : 8.0f) / m_graphScale * ImGui::GetContentRegionAvail().x;
        float updateWidth = (m_updateTime > 0 ? m_updateTime : 2.0f) / m_graphScale * ImGui::GetContentRegionAvail().x;
        float imguiWidth = (m_imguiTime > 0 ? m_imguiTime : 3.0f) / m_graphScale * ImGui::GetContentRegionAvail().x;
        
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float height = 24.0f;
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        float x = startPos.x;
        drawList->AddRectFilled(ImVec2(x, startPos.y), ImVec2(x + physicsWidth, startPos.y + height),
                               IM_COL32(100, 200, 100, 255));
        x += physicsWidth;
        
        drawList->AddRectFilled(ImVec2(x, startPos.y), ImVec2(x + renderWidth, startPos.y + height),
                               IM_COL32(100, 150, 200, 255));
        x += renderWidth;
        
        drawList->AddRectFilled(ImVec2(x, startPos.y), ImVec2(x + updateWidth, startPos.y + height),
                               IM_COL32(200, 150, 100, 255));
        x += updateWidth;
        
        drawList->AddRectFilled(ImVec2(x, startPos.y), ImVec2(x + imguiWidth, startPos.y + height),
                               IM_COL32(180, 100, 180, 255));
        
        ImGui::Dummy(ImVec2(-1, height + 4));
        
        // Legend
        ImGui::Columns(4, nullptr, false);
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        ImGui::Text("\xef\x83\x88 Physics: %.2fms", m_physicsTime > 0 ? m_physicsTime : 1.5f);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
        ImGui::Text("\xef\x83\x88 Render: %.2fms", m_renderTime > 0 ? m_renderTime : 8.0f);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.4f, 1.0f));
        ImGui::Text("\xef\x83\x88 Update: %.2fms", m_updateTime > 0 ? m_updateTime : 2.0f);
        ImGui::PopStyleColor();
        ImGui::NextColumn();
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.4f, 0.7f, 1.0f));
        ImGui::Text("\xef\x83\x88 UI: %.2fms", m_imguiTime > 0 ? m_imguiTime : 3.0f);
        ImGui::PopStyleColor();
        
        ImGui::Columns(1);
        
        ImGui::TreePop();
    }
    
    // Graph scale slider
    ImGui::Spacing();
    ImGui::SliderFloat("Graph Scale (ms)", &m_graphScale, 16.67f, 100.0f, "%.1f ms");
}

void PerformancePanel::renderMemoryStats() {
    ImGui::Spacing();
    
    // Memory usage bar
    float usedMB = static_cast<float>(m_usedMemory) / (1024 * 1024);
    float totalMB = static_cast<float>(m_totalMemory) / (1024 * 1024);
    float percentage = (m_usedMemory / static_cast<float>(m_totalMemory)) * 100.0f;
    
    ImGui::Text("System Memory");
    
    ImVec4 memColor;
    if (percentage < 50) {
        memColor = ImVec4(0.3f, 0.9f, 0.3f, 1.0f);
    } else if (percentage < 80) {
        memColor = ImVec4(0.9f, 0.9f, 0.3f, 1.0f);
    } else {
        memColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
    }
    
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, memColor);
    char overlay[64];
    snprintf(overlay, sizeof(overlay), "%.0f MB / %.0f MB (%.1f%%)", usedMB, totalMB, percentage);
    ImGui::ProgressBar(m_usedMemory / static_cast<float>(m_totalMemory), ImVec2(-1, 0), overlay);
    ImGui::PopStyleColor();
    
    ImGui::Spacing();
    
    // GPU memory
    float gpuMB = static_cast<float>(m_gpuMemory) / (1024 * 1024);
    ImGui::Text("GPU Memory");
    snprintf(overlay, sizeof(overlay), "%.0f MB", gpuMB);
    ImGui::ProgressBar(0.5f, ImVec2(-1, 0), overlay);
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    
    // Detailed breakdown
    if (ImGui::TreeNode("Memory Breakdown")) {
        ImGui::Columns(2, nullptr, false);
        
        ImGui::Text("Meshes:");
        ImGui::Text("Textures:");
        ImGui::Text("Shaders:");
        ImGui::Text("Physics Data:");
        ImGui::Text("Scene Data:");
        ImGui::Text("Particles:");
        ImGui::Text("Audio:");
        ImGui::Text("Other:");
        
        ImGui::NextColumn();
        
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "45.2 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "128.5 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "2.1 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "35.8 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "12.4 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "8.6 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "15.2 MB");
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "8.2 MB");
        
        ImGui::Columns(1);
        ImGui::TreePop();
    }
    
    // Actions
    ImGui::Spacing();
    if (ImGui::Button("Force GC")) {
        // Would trigger garbage collection
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Caches")) {
        // Would clear various caches
    }
}

void PerformancePanel::renderSystemInfo() {
    ImGui::Spacing();
    
    if (ImGui::CollapsingHeader("GPU Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::TextWrapped("Renderer: OpenGL 4.3");
        ImGui::TextWrapped("Vendor: NVIDIA Corporation");
        ImGui::TextWrapped("Device: NVIDIA GeForce RTX 3080");
        ImGui::TextWrapped("GLSL Version: 4.30");
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("CPU Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::TextWrapped("Processor: Intel Core i9-10900K");
        ImGui::TextWrapped("Cores: 10 (20 threads)");
        ImGui::TextWrapped("Base Clock: 3.70 GHz");
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Display Info")) {
        ImGui::Indent();
        ImGui::Text("Window Size: 1920 x 1080");
        ImGui::Text("Viewport Size: 1280 x 720");
        ImGui::Text("VSync: Enabled");
        ImGui::Text("MSAA: 4x");
        ImGui::Unindent();
    }
    
    if (ImGui::CollapsingHeader("Build Info")) {
        ImGui::Indent();
        ImGui::Text("Version: 1.0.0");
        ImGui::Text("Build: Debug");
        ImGui::Text("Date: %s", __DATE__);
        ImGui::Text("Compiler: MSVC 19.29");
        ImGui::Unindent();
    }
}

void PerformancePanel::renderDetailedTimings() {
    ImGui::Text("Detailed Frame Breakdown");
    ImGui::Spacing();
    
    ImGui::Columns(3, nullptr, true);
    ImGui::SetColumnWidth(0, 200);
    ImGui::SetColumnWidth(1, 100);
    
    // Header
    ImGui::Text("System");
    ImGui::NextColumn();
    ImGui::Text("Time (ms)");
    ImGui::NextColumn();
    ImGui::Text("%%");
    ImGui::NextColumn();
    ImGui::Separator();
    
    float total = 16.67f;  // Assume 60 fps for percentages
    
    auto addRow = [&](const char* name, float time, ImVec4 color) {
        ImGui::TextColored(color, "%s", name);
        ImGui::NextColumn();
        ImGui::Text("%.3f", time);
        ImGui::NextColumn();
        ImGui::Text("%.1f%%", (time / total) * 100.0f);
        ImGui::NextColumn();
    };
    
    addRow("Physics - Broad Phase", 0.12f, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
    addRow("Physics - Narrow Phase", 0.45f, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
    addRow("Physics - Solver", 0.82f, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
    addRow("Physics - Integration", 0.15f, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
    addRow("Cloth Simulation", 0.95f, ImVec4(0.8f, 0.4f, 0.6f, 1.0f));
    addRow("Scene Update", 0.35f, ImVec4(0.8f, 0.6f, 0.4f, 1.0f));
    addRow("Render - Culling", 0.18f, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    addRow("Render - Shadow Pass", 1.25f, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    addRow("Render - Main Pass", 4.85f, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    addRow("Render - Post Process", 1.45f, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    addRow("ImGui", 2.85f, ImVec4(0.7f, 0.4f, 0.7f, 1.0f));
    addRow("Swap Buffers", 2.95f, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    
    ImGui::Columns(1);
}

void PerformancePanel::renderPhysicsStats() {
    ImGui::Spacing();
    
    // Object counts
    if (ImGui::CollapsingHeader("Object Counts", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Columns(2, nullptr, false);
        
        ImGui::Text("Active Rigid Bodies:");
        ImGui::Text("Sleeping Bodies:");
        ImGui::Text("Particles:");
        ImGui::Text("Springs:");
        ImGui::Text("Contact Points:");
        ImGui::Text("Constraints:");
        
        ImGui::NextColumn();
        
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "%d", m_activeRigidBodies);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%d", m_sleepingBodies);
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.7f, 1.0f), "%d", m_particleCount);
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.7f, 1.0f), "%d", m_springCount);
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "%d", m_contactCount);
        ImGui::TextColored(ImVec4(0.95f, 0.7f, 0.3f, 1.0f), "%d", 24);
        
        ImGui::Columns(1);
        ImGui::Unindent();
    }
    
    // Physics timing graph
    if (ImGui::CollapsingHeader("Physics Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        
        std::vector<float> physicsData(m_physicsTimeHistory.begin(), m_physicsTimeHistory.end());
        if (!physicsData.empty()) {
            ImGui::PlotLines("##PhysicsTime", physicsData.data(), static_cast<int>(physicsData.size()),
                            0, "Physics (ms)", 0.0f, 5.0f, ImVec2(-1, 60));
        }
        
        ImGui::Columns(4, nullptr, false);
        
        ImGui::Text("Broad Phase");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "0.12 ms");
        ImGui::NextColumn();
        
        ImGui::Text("Narrow Phase");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "0.45 ms");
        ImGui::NextColumn();
        
        ImGui::Text("Solver");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "0.82 ms");
        ImGui::NextColumn();
        
        ImGui::Text("Integration");
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.95f, 1.0f), "0.15 ms");
        
        ImGui::Columns(1);
        ImGui::Unindent();
    }
    
    // Cloth simulation stats
    if (ImGui::CollapsingHeader("Cloth Simulation")) {
        ImGui::Indent();
        
        ImGui::Text("Cloth Instances: %d", 1);
        ImGui::Text("Total Particles: %d", m_particleCount);
        ImGui::Text("Total Springs: %d", m_springCount);
        ImGui::Text("Substeps per Frame: %d", 4);
        ImGui::Text("Simulation Time: %.2f ms", 0.95f);
        
        ImGui::Unindent();
    }
}

void PerformancePanel::update(float dt) {
    m_timeSinceUpdate += dt;
    
    if (m_timeSinceUpdate >= m_updateInterval) {
        m_currentFrameTime = dt * 1000.0f;
        m_currentFps = 1.0f / dt;
        
        // Add to history
        m_frameTimeHistory.push_back(m_currentFrameTime);
        if (m_frameTimeHistory.size() > HISTORY_SIZE) {
            m_frameTimeHistory.pop_front();
        }
        
        // Calculate averages
        if (!m_frameTimeHistory.empty()) {
            m_avgFrameTime = std::accumulate(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 0.0f) / m_frameTimeHistory.size();
            m_avgFps = 1000.0f / m_avgFrameTime;
            
            auto minMax = std::minmax_element(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
            m_maxFps = 1000.0f / (*minMax.first);
            m_minFps = 1000.0f / (*minMax.second);
        }
        
        m_timeSinceUpdate = 0.0f;
    }
}

void PerformancePanel::addFrameTime(float ms) {
    m_frameTimeHistory.push_back(ms);
    if (m_frameTimeHistory.size() > HISTORY_SIZE) {
        m_frameTimeHistory.pop_front();
    }
}

void PerformancePanel::addPhysicsTime(float ms) {
    m_physicsTime = ms;
    m_physicsTimeHistory.push_back(ms);
    if (m_physicsTimeHistory.size() > HISTORY_SIZE) {
        m_physicsTimeHistory.pop_front();
    }
}

void PerformancePanel::addRenderTime(float ms) {
    m_renderTime = ms;
    m_renderTimeHistory.push_back(ms);
    if (m_renderTimeHistory.size() > HISTORY_SIZE) {
        m_renderTimeHistory.pop_front();
    }
}

} // namespace editor

