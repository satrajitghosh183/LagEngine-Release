#include "ServerRegistry.hpp"
#include "Logger.hpp"

namespace GameEngine {

    Ref<IRenderServer> ServerRegistry::s_RenderServer = nullptr;
    Ref<IPhysicsServer> ServerRegistry::s_PhysicsServer = nullptr;
    Ref<IAudioServer> ServerRegistry::s_AudioServer = nullptr;
    Ref<IScriptServer> ServerRegistry::s_ScriptServer = nullptr;
    Ref<ITelemetryServer> ServerRegistry::s_TelemetryServer = nullptr;

    void ServerRegistry::RegisterRenderServer(Ref<IRenderServer> server) {
        s_RenderServer = server;
        GE_CORE_INFO("Render server registered");
    }

    void ServerRegistry::RegisterPhysicsServer(Ref<IPhysicsServer> server) {
        s_PhysicsServer = server;
        GE_CORE_INFO("Physics server registered");
    }

    void ServerRegistry::RegisterAudioServer(Ref<IAudioServer> server) {
        s_AudioServer = server;
        GE_CORE_INFO("Audio server registered");
    }

    void ServerRegistry::RegisterScriptServer(Ref<IScriptServer> server) {
        s_ScriptServer = server;
        GE_CORE_INFO("Script server registered");
    }

    void ServerRegistry::RegisterTelemetryServer(Ref<ITelemetryServer> server) {
        s_TelemetryServer = server;
        GE_CORE_INFO("Telemetry server registered");
    }

    IRenderServer& ServerRegistry::GetRenderServer() {
        GE_CORE_ASSERT(s_RenderServer, "Render server not registered!");
        return *s_RenderServer;
    }

    IPhysicsServer& ServerRegistry::GetPhysicsServer() {
        GE_CORE_ASSERT(s_PhysicsServer, "Physics server not registered!");
        return *s_PhysicsServer;
    }

    IAudioServer& ServerRegistry::GetAudioServer() {
        GE_CORE_ASSERT(s_AudioServer, "Audio server not registered!");
        return *s_AudioServer;
    }

    IScriptServer& ServerRegistry::GetScriptServer() {
        GE_CORE_ASSERT(s_ScriptServer, "Script server not registered!");
        return *s_ScriptServer;
    }

    ITelemetryServer& ServerRegistry::GetTelemetryServer() {
        GE_CORE_ASSERT(s_TelemetryServer, "Telemetry server not registered!");
        return *s_TelemetryServer;
    }

    bool ServerRegistry::HasRenderServer() {
        return s_RenderServer != nullptr;
    }

    bool ServerRegistry::HasPhysicsServer() {
        return s_PhysicsServer != nullptr;
    }

    bool ServerRegistry::HasAudioServer() {
        return s_AudioServer != nullptr;
    }

    bool ServerRegistry::HasScriptServer() {
        return s_ScriptServer != nullptr;
    }

    bool ServerRegistry::HasTelemetryServer() {
        return s_TelemetryServer != nullptr;
    }

    void ServerRegistry::Clear() {
        s_RenderServer.reset();
        s_PhysicsServer.reset();
        s_AudioServer.reset();
        s_ScriptServer.reset();
        s_TelemetryServer.reset();
        GE_CORE_INFO("Server registry cleared");
    }

}

