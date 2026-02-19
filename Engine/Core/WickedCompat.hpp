#pragma once

#include "Base.hpp"
#include "Application.hpp"
#include "../Graphics/RenderPath.hpp"
#include "../Scene/Scene.hpp"
#include "../Scene/SceneHelpers.hpp"
#include "../Audio/AudioHelpers.hpp"
#include "../Platform/InputHelpers.hpp"
#include <string>

// Wicked Engine-style compatibility namespace
namespace wi {

    /**
     * @brief Wicked Engine-style Application wrapper
     * 
     * Provides a thin adapter over GameEngine::Application
     * to match Wicked Engine's API style
     */
    class Application {
    public:
        Application() = default;
        ~Application() = default;

        /**
         * @brief Set the native window handle
         * @param hWnd Platform-specific window handle (GLFWwindow* on GLFW)
         */
        void SetWindow(void* hWnd);

        /**
         * @brief Initialize the application
         * Equivalent to Application::Init()
         */
        void Initialize();

        /**
         * @brief Run the application main loop
         * Equivalent to Application::Run()
         */
        void Run();

        /**
         * @brief Activate a render path
         * @param path Render path to activate (can be nullptr to deactivate)
         * @param fadeSeconds Fade transition time (0 = instant)
         */
        void ActivatePath(GameEngine::Ref<GameEngine::RenderPath> path, float fadeSeconds = 0.0f);

        /**
         * @brief Get the underlying GameEngine application
         */
        static GameEngine::Application& Get() {
            return GameEngine::Application::Get();
        }
    };

    /**
     * @brief Scene helper functions (Wicked Engine style)
     */
    namespace scene {

        /**
         * @brief Get the current active scene (global scene)
         */
        GameEngine::Ref<GameEngine::Scene> GetScene();

        /**
         * @brief Load a model/scene file into the global scene
         * @param filepath Path to model or scene file
         * @return Root entity of loaded model, or invalid entity if failed
         */
        GameEngine::Entity LoadModel(const std::string& filepath);

        /**
         * @brief Load a model/scene file into a specific scene
         * @param scene Target scene
         * @param filepath Path to model or scene file
         * @return Root entity of loaded model, or invalid entity if failed
         */
        GameEngine::Entity LoadModel(GameEngine::Ref<GameEngine::Scene> scene, const std::string& filepath);
    }

    /**
     * @brief Audio helper functions (Wicked Engine style)
     * 
     * Uses GameEngine::AudioHelpers namespace
     */
    namespace audio {
        using Sound = GameEngine::AudioHelpers::Sound;
        using SoundInstance = GameEngine::AudioHelpers::SoundInstance;

        /**
         * @brief Create a sound from file
         */
        bool CreateSound(const std::string& filepath, Sound& out);

        /**
         * @brief Create a sound instance from a sound
         */
        bool CreateSoundInstance(const Sound& sound, SoundInstance& out);

        /**
         * @brief Play a sound instance
         */
        void Play(SoundInstance& instance);

        /**
         * @brief Stop a sound instance
         */
        void Stop(SoundInstance& instance);

        /**
         * @brief Set volume for a sound instance or master volume
         */
        void SetVolume(float volume, SoundInstance* instance = nullptr);
    }

    /**
     * @brief Input helper functions (Wicked Engine style)
     * 
     * Uses GameEngine::InputHelpers namespace
     */
    namespace input {
        using KeyCode = GameEngine::KeyCode;

        /**
         * @brief Check if key was just pressed this frame
         */
        bool Press(KeyCode key);

        /**
         * @brief Check if key is currently held down
         */
        bool Down(KeyCode key);

        /**
         * @brief Key constants matching Wicked Engine style
         */
        namespace Keys {
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_SPACE;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_LEFT;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_RIGHT;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_UP;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_DOWN;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_ESCAPE;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_ENTER;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_TAB;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_BACKSPACE;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_DELETE;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_HOME;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_END;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_PAGEUP;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_PAGEDOWN;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F1;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F2;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F3;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F4;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F5;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F6;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F7;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F8;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F9;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F10;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F11;
            using GameEngine::InputHelpers::Keys::KEYBOARD_BUTTON_F12;
            using GameEngine::InputHelpers::Keys::MOUSE_BUTTON_LEFT;
            using GameEngine::InputHelpers::Keys::MOUSE_BUTTON_RIGHT;
            using GameEngine::InputHelpers::Keys::MOUSE_BUTTON_MIDDLE;
        }
    }

}
