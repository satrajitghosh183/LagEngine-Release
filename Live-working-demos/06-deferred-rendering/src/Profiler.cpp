#include "Profiler.h"
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>

Profiler::Profiler()
    : m_frameTime(0.0f)
    , m_fps(0.0f)
    , m_fpsAccumulator(0.0f)
    , m_fpsFrames(0)
    , m_logging(false)
    , m_loggedFrames(0)
{
}

Profiler& Profiler::instance() {
    static Profiler profiler;
    return profiler;
}

void Profiler::beginFrame() {
    m_frameStart = std::chrono::high_resolution_clock::now();
    m_frameData.clear();
}

void Profiler::endFrame() {
    m_frameEnd = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(m_frameEnd - m_frameStart);
    m_frameTime = duration.count() / 1000.0f; // Convert to milliseconds
    
    // Update FPS
    m_fpsFrames++;
    m_fpsAccumulator += m_frameTime;
    if (m_fpsFrames >= 60) {
        float avgFrameTime = m_fpsAccumulator / m_fpsFrames;
        m_fps = 1000.0f / avgFrameTime;
        m_fpsAccumulator = 0.0f;
        m_fpsFrames = 0;
    }
    
    // Log to CSV if enabled
    if (m_logging && m_logFile.is_open()) {
        if (m_loggedFrames == 0) {
            // Write header
            m_logFile << "frame,frame_time_ms,fps";
            for (const auto& data : m_frameData) {
                m_logFile << "," << data.name << "_ms";
            }
            m_logFile << "\n";
        }
        
        m_logFile << m_loggedFrames << "," << m_frameTime << "," << m_fps;
        for (const auto& data : m_frameData) {
            m_logFile << "," << data.duration;
        }
        m_logFile << "\n";
        
        m_loggedFrames++;
    }
}

void Profiler::startProfile(const std::string& name) {
    m_startTimes[name] = std::chrono::high_resolution_clock::now();
}

void Profiler::endProfile(const std::string& name) {
    auto it = m_startTimes.find(name);
    if (it != m_startTimes.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second);
        
        ProfileData data;
        data.name = name;
        data.startTime = getCurrentTime();
        data.endTime = data.startTime + duration.count() / 1000.0f;
        data.duration = duration.count() / 1000.0f; // Convert to milliseconds
        
        m_frameData.push_back(data);
        m_startTimes.erase(it);
    }
}

void Profiler::startLogging(const std::string& filename) {
    m_logFile.open(filename);
    m_logging = true;
    m_loggedFrames = 0;
}

void Profiler::stopLogging() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
    m_logging = false;
}

float Profiler::getCurrentTime() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_frameStart);
    return duration.count() / 1000.0f;
}

