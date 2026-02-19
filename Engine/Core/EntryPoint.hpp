#pragma once

#include "Application.hpp"
#include "GraphicsOptions.hpp"

/**
 * @brief Engine entry point
 * 
 * Include this header in ONE compilation unit (typically the file with CreateApplication)
 * It defines the main() function that initializes and runs the application.
 * 
 * Usage:
 *   // In MyApp.cpp
 *   #include <GameEngine/Core/Application.hpp>
 *   #include <GameEngine/Core/EntryPoint.hpp>  // Must be last include
 *   
 *   class MyApp : public GameEngine::Application { ... };
 *   
 *   GameEngine::Application* GameEngine::CreateApplication() {
 *       return new MyApp();
 *   }
 */

#ifdef _WIN32

// Windows entry point
extern GameEngine::Application* GameEngine::CreateApplication();

int main(int argc, char** argv) {
    GameEngine::GraphicsOptions::Initialize(argc, argv);
    GameEngine::Application* app = GameEngine::CreateApplication();
    if (app) {
        app->Run();
        delete app;
    }
    return 0;
}

#else

// Linux/macOS entry point
extern GameEngine::Application* GameEngine::CreateApplication();

int main(int argc, char** argv) {
    GameEngine::GraphicsOptions::Initialize(argc, argv);
    GameEngine::Application* app = GameEngine::CreateApplication();
    if (app) {
        app->Run();
        delete app;
    }
    return 0;
}

#endif
