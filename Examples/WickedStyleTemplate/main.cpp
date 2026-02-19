#include "WickedStyleTemplateApp.hpp"
#include "../../Engine/Core/EntryPoint.hpp"

GameEngine::Application* GameEngine::CreateApplication() {
    return new GameEngine::WickedStyleTemplateApp();
}
