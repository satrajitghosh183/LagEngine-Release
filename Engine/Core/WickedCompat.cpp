#include "WickedCompat.hpp"
#include "Logger.hpp"
#include "../Platform/Window.hpp"

namespace wi {

    void Application::SetWindow(void* hWnd) {
        // Note: In GLFW, the window is already created by Application::Init()
        // This function is mainly for compatibility with Wicked Engine's API
        // The window handle should match what Application already has
        auto& app = GameEngine::Application::Get();
        void* currentHandle = app.GetWindow().GetNativeWindow();
        
        if (currentHandle != hWnd) {
            GE_CORE_WARN("WickedCompat::Application::SetWindow: Window handle mismatch. "
                         "Window was already created by Application::Init()");
        }
    }

    void Application::Initialize() {
        auto& app = GameEngine::Application::Get();
        // Application::Init() is called automatically in Run() if not already initialized
        // This is mainly for explicit initialization if needed before Run()
        // For now, we'll rely on the automatic initialization in Run()
    }

    void Application::Run() {
        auto& app = GameEngine::Application::Get();
        app.Run();
    }

    void Application::ActivatePath(GameEngine::Ref<GameEngine::RenderPath> path, float fadeSeconds) {
        auto& app = GameEngine::Application::Get();
        app.ActivatePath(path, fadeSeconds);
    }

    namespace scene {

        GameEngine::Ref<GameEngine::Scene> GetScene() {
            return GameEngine::SceneHelpers::GetScene();
        }

        GameEngine::Entity LoadModel(const std::string& filepath) {
            return GameEngine::SceneHelpers::LoadModel(filepath);
        }

        GameEngine::Entity LoadModel(GameEngine::Ref<GameEngine::Scene> scene, const std::string& filepath) {
            return GameEngine::SceneHelpers::LoadModel(scene, filepath);
        }

    }

    namespace audio {

        bool CreateSound(const std::string& filepath, Sound& out) {
            return GameEngine::AudioHelpers::CreateSound(filepath, out);
        }

        bool CreateSoundInstance(const Sound& sound, SoundInstance& out) {
            return GameEngine::AudioHelpers::CreateSoundInstance(sound, out);
        }

        void Play(SoundInstance& instance) {
            GameEngine::AudioHelpers::Play(instance);
        }

        void Stop(SoundInstance& instance) {
            GameEngine::AudioHelpers::Stop(instance);
        }

        void SetVolume(float volume, SoundInstance* instance) {
            GameEngine::AudioHelpers::SetVolume(volume, instance);
        }

    }

    namespace input {

        bool Press(KeyCode key) {
            return GameEngine::InputHelpers::Press(key);
        }

        bool Down(KeyCode key) {
            return GameEngine::InputHelpers::Down(key);
        }

    }

}
