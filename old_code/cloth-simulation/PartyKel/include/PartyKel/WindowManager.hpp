#pragma once

#include <SDL/SDL.h>

namespace PartyKel {

class WindowManager {
public:
    WindowManager(uint32_t w, uint32_t h, const char* title);

    ~WindowManager();

    WindowManager(const WindowManager&) = delete;

    WindowManager& operator =(const WindowManager&) = delete;

    bool pollEvent(SDL_Event& e) const {
        return SDL_PollEvent(&e);
    }

   
    void startMainLoop() {
        m_nStartTime = SDL_GetTicks();
    }

   
    float update();

    void setFramerate(uint32_t fps) {
        m_nFPS = fps;
        m_nFrameDuration = 1000.f / m_nFPS;
    }
private:
    uint32_t m_nFPS;
    uint32_t m_nFrameDuration;

    uint32_t m_nStartTime;
};

}
