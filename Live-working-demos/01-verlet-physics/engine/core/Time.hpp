#pragma once
#include <chrono>

// Simple time utilities for deltaTime and frame timing
namespace engine::core::time {
    class Time {
    private:
        std::chrono::steady_clock::time_point lastTime;
        float deltaTime = 0.0f;
    public:
        Time() {
            lastTime = std::chrono::steady_clock::now();
        }

        void update() {
            auto now = std::chrono::steady_clock::now();
            deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;
        }

        float getDeltaTime() const { return deltaTime; }
    };
}

