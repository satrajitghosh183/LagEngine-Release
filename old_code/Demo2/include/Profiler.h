#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

struct ProfileData {
    std::string name;
    float startTime;
    float endTime;
    float duration;
};

class Profiler {
public:
    static Profiler& instance();
    
    void beginFrame();
    void endFrame();
    
    void startProfile(const std::string& name);
    void endProfile(const std::string& name);
    
    float getFrameTime() const { return m_frameTime; }
    float getFPS() const { return m_fps > 0.0f ? m_fps : (m_frameTime > 0.0f ? 1000.0f / m_frameTime : 0.0f); }
    
    const std::vector<ProfileData>& getFrameData() const { return m_frameData; }
    
    // CSV logging
    void startLogging(const std::string& filename);
    void stopLogging();
    bool isLogging() const { return m_logging; }
    
private:
    Profiler();
    
    std::chrono::high_resolution_clock::time_point m_frameStart;
    std::chrono::high_resolution_clock::time_point m_frameEnd;
    
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> m_startTimes;
    std::vector<ProfileData> m_frameData;
    
    float m_frameTime;
    float m_fps;
    float m_fpsAccumulator;
    int m_fpsFrames;
    
    bool m_logging;
    std::ofstream m_logFile;
    int m_loggedFrames;
    
    float getCurrentTime() const;
};

