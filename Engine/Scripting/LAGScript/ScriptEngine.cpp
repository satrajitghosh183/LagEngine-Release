#include "ScriptEngine.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "../../Core/Logger.hpp"

#include <fstream>
#include <sstream>

namespace GameEngine {
namespace LAGScript {

    std::unordered_map<std::string, Ref<ScriptInstance>> LAGScriptEngine::s_Scripts;

    bool ScriptInstance::LoadFromSource(const std::string& source) {
        Lexer lex(source);
        auto tokens = lex.Tokenize();
        if (lex.HasErrors()) {
            for (auto& e : lex.GetErrors()) GE_CORE_WARN("LAGScript lex: {}", e);
            return false;
        }
        Parser p(std::move(tokens));
        auto ast = p.Parse();
        if (p.HasErrors()) {
            for (auto& e : p.GetErrors()) GE_CORE_WARN("LAGScript parse: {}", e);
            return false;
        }
        m_Interp.Run(ast);
        m_Loaded = true;
        return true;
    }

    bool ScriptInstance::LoadFromFile(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            GE_CORE_ERROR("LAGScript: failed to open {}", path);
            return false;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        return LoadFromSource(ss.str());
    }

    void ScriptInstance::CallMethod(const std::string& name, const ValueList& args) {
        if (!m_Loaded) return;
        if (HasMethod(name)) m_Interp.Call(name, args);
    }

    bool ScriptInstance::HasMethod(const std::string& name) const {
        Value v;
        return const_cast<ScriptInstance*>(this)->m_Interp.GlobalEnv()->Get(name, v)
               && (v.Type == Value::Kind::Function || v.Type == Value::Kind::NativeFunction);
    }

    void ScriptInstance::SetProperty(const std::string& name, const Value& v) {
        m_Interp.SetGlobal(name, v);
    }

    Value ScriptInstance::GetProperty(const std::string& name) const {
        return const_cast<ScriptInstance*>(this)->m_Interp.GetGlobal(name);
    }

    void LAGScriptEngine::Init()     { GE_CORE_INFO("LAGScriptEngine initialized"); }
    void LAGScriptEngine::Shutdown() { s_Scripts.clear(); }

    Ref<ScriptInstance> LAGScriptEngine::LoadScript(const std::string& path) {
        auto it = s_Scripts.find(path);
        if (it != s_Scripts.end()) return it->second;

        auto inst = CreateRef<ScriptInstance>();
        if (!inst->LoadFromFile(path)) return nullptr;
        s_Scripts[path] = inst;
        return inst;
    }

    void LAGScriptEngine::ReloadAll() {
        for (auto& [path, inst] : s_Scripts) inst->LoadFromFile(path);
    }

}}
