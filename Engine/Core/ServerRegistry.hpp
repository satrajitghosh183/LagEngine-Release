#pragma once

#include "Base.hpp"
#include "Runtime/IRenderServer.hpp"
#include "Runtime/IPhysicsServer.hpp"
#include "Runtime/IAudioServer.hpp"
#include "Runtime/IScriptServer.hpp"
#include "Runtime/ITelemetryServer.hpp"
#include <memory>

namespace GameEngine {

    /**
     * @brief Server registry (ServiceLocator pattern)
     * 
     * Central registry for accessing engine servers
     * Both runtime and editor can query servers through this interface
     */
    class ServerRegistry {
    public:
        /**
         * @brief Register render server
         */
        static void RegisterRenderServer(Ref<IRenderServer> server);

        /**
         * @brief Register physics server
         */
        static void RegisterPhysicsServer(Ref<IPhysicsServer> server);

        /**
         * @brief Register audio server
         */
        static void RegisterAudioServer(Ref<IAudioServer> server);

        /**
         * @brief Register script server
         */
        static void RegisterScriptServer(Ref<IScriptServer> server);

        /**
         * @brief Register telemetry server
         */
        static void RegisterTelemetryServer(Ref<ITelemetryServer> server);

        /**
         * @brief Get render server
         */
        static IRenderServer& GetRenderServer();

        /**
         * @brief Get physics server
         */
        static IPhysicsServer& GetPhysicsServer();

        /**
         * @brief Get audio server
         */
        static IAudioServer& GetAudioServer();

        /**
         * @brief Get script server
         */
        static IScriptServer& GetScriptServer();

        /**
         * @brief Get telemetry server
         */
        static ITelemetryServer& GetTelemetryServer();

        /**
         * @brief Check if server is registered
         */
        static bool HasRenderServer();
        static bool HasPhysicsServer();
        static bool HasAudioServer();
        static bool HasScriptServer();
        static bool HasTelemetryServer();

        /**
         * @brief Clear all registrations
         */
        static void Clear();

    private:
        static Ref<IRenderServer> s_RenderServer;
        static Ref<IPhysicsServer> s_PhysicsServer;
        static Ref<IAudioServer> s_AudioServer;
        static Ref<IScriptServer> s_ScriptServer;
        static Ref<ITelemetryServer> s_TelemetryServer;
    };

}

