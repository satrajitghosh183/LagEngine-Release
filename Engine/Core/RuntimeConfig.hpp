#pragma once

#include <cstdint>
#include <string>

namespace GameEngine {

    /**
     * @brief Runtime configuration for engine initialization
     * 
     * Used by both games and editor to configure engine subsystems
     */
    struct RuntimeConfig {
        uint32_t width = 1280;
        uint32_t height = 720;
        std::string title = "GameEngine App";
        bool vsync = true;
        bool headless = false;
        bool enableDebug = false;
        
        RuntimeConfig() = default;
        RuntimeConfig(uint32_t w, uint32_t h, const std::string& t = "GameEngine App")
            : width(w), height(h), title(t) {}
    };

}

