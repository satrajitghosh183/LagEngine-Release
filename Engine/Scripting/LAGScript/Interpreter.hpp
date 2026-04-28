#pragma once

#include "AST.hpp"
#include "Value.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace GameEngine {
namespace LAGScript {

    class Interpreter {
    public:
        Interpreter();

        // Register native C++ functions callable from script
        void RegisterNative(const std::string& name, NativeFn fn);

        // Run a parsed program in the global environment
        void Run(NodePtr program);

        // Call a method on a top-level identifier or instance
        Value Call(const std::string& name, const ValueList& args);

        // Access/update globals
        void SetGlobal(const std::string& name, const Value& v);
        Value GetGlobal(const std::string& name) const;

        const std::vector<std::string>& GetErrors() const { return m_Errors; }
        void ReportError(const std::string& msg, int line = 0);

        Ref<Environment> GlobalEnv() { return m_Globals; }

    private:
        struct ReturnException { Value Result; };
        struct BreakException {};
        struct ContinueException {};

        Value Eval(const NodePtr& node, Ref<Environment> env);
        Value EvalCall(const NodePtr& node, Ref<Environment> env);
        Value EvalBinary(const NodePtr& node, Ref<Environment> env);
        Value EvalMember(const NodePtr& node, Ref<Environment> env);
        Value EvalAssign(const NodePtr& node, Ref<Environment> env);
        void  ExecStmt(const NodePtr& node, Ref<Environment> env);
        Value CallClosure(Ref<Closure> fn, const ValueList& args);

        void InstallBuiltins();

        Ref<Environment> m_Globals;
        std::unordered_map<std::string, Ref<ClassInfo>> m_Classes;
        std::vector<std::string> m_Errors;
    };

}}
