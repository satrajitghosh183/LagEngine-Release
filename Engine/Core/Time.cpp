#include "Time.hpp"

namespace GameEngine {

    Time::TimePoint Time::s_StartTime = Time::Clock::now();
    Time::TimePoint Time::s_LastFrameTime;
    float Time::s_DeltaTime = 0.0f;
    float Time::s_Time = 0.0f;
    float Time::s_TimeScale = 1.0f;
    float Time::s_FixedTimestep = 1.0f / 60.0f;
    uint64_t Time::s_FrameCount = 0;
    float Time::s_FPS = 0.0f;
    float Time::s_FPSAccumulator = 0.0f;
    int Time::s_FPSFrameCount = 0;

    void Time::Init() {
        s_StartTime = Clock::now();
        s_LastFrameTime = s_StartTime;
        s_DeltaTime = 0.0f;
        s_Time = 0.0f;
        s_FrameCount = 0;
    }

    void Time::Update() {
        TimePoint currentTime = Clock::now();
        std::chrono::duration<float> duration = currentTime - s_LastFrameTime;
        s_DeltaTime = duration.count();
        
        // Clamp delta time to prevent spiral of death
        if (s_DeltaTime > 0.1f) {
            s_DeltaTime = 0.1f;
        }
        
        s_Time += s_DeltaTime * s_TimeScale;
        s_LastFrameTime = currentTime;
        s_FrameCount++;
        
        // Calculate FPS (update every second)
        s_FPSAccumulator += s_DeltaTime;
        s_FPSFrameCount++;
        
        if (s_FPSAccumulator >= 1.0f) {
            s_FPS = static_cast<float>(s_FPSFrameCount) / s_FPSAccumulator;
            s_FPSAccumulator = 0.0f;
            s_FPSFrameCount = 0;
        }
    }
}