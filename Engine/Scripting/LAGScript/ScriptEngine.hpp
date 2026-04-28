#pragma once

#include "Interpreter.hpp"
#include "../../Core/Base.hpp"
#include <string>
#include <unordered_map>

namespace GameEngine {
namespace LAGScript {

    // A loaded script instance bound to an entity.
    // Wraps an Interpreter with its own environment and instance value.
    class ScriptInstance {
    public:
        ScriptInstance() = default;

        bool LoadFromSource(const std::string& source);
        bool LoadFromFile(const std::string& path);

        void CallMethod(const std::string& name, const ValueList& args = {});
        bool HasMethod(const std::string& name) const;

        void SetProperty(const std::string& name, const Value& v);
        Value GetProperty(const std::string& name) const;

        Interpreter& GetInterpreter() { return m_Interp; }

    private:
        Interpreter m_Interp;
        bool m_Loaded = false;
    };

    // High-level engine that manages script loading + hot-reload
    class LAGScriptEngine {
    public:
        static void Init();
        static void Shutdown();

        // Load a .lag script from disk; returns a ScriptInstance ready to run.
        static Ref<ScriptInstance> LoadScript(const std::string& path);

        // Reload all scripts (e.g., for hot-reload)
        static void ReloadAll();

    private:
        static std::unordered_map<std::string, Ref<ScriptInstance>> s_Scripts;
    };

}}
