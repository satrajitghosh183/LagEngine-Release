#pragma once

#include "../../Core/Base.hpp"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace GameEngine {
namespace LAGScript {

    struct Value;
    struct Interpreter;
    using ValueList = std::vector<Value>;
    using NativeFn = std::function<Value(Interpreter&, const ValueList&)>;

    // Forward decl
    struct ASTNode;
    using NodePtr = Ref<ASTNode>;

    struct Closure;
    struct Instance;
    struct Array;
    struct Dict;

    struct Value {
        enum class Kind {
            Null, Bool, Int, Float, String,
            Array, Dict, Function, NativeFunction, Instance
        };

        Kind Type = Kind::Null;

        bool BoolValue = false;
        long long IntValue = 0;
        double FloatValue = 0.0;
        std::string StringValue;
        Ref<Array> ArrayValue;
        Ref<Dict> DictValue;
        Ref<Closure> FunctionValue;
        NativeFn NativeValue;
        Ref<Instance> InstanceValue;

        static Value Null() { return {}; }
        static Value Bool(bool b)   { Value v; v.Type = Kind::Bool;  v.BoolValue = b; return v; }
        static Value Int(long long i) { Value v; v.Type = Kind::Int; v.IntValue = i; return v; }
        static Value Float(double f){ Value v; v.Type = Kind::Float; v.FloatValue = f; return v; }
        static Value String(std::string s) { Value v; v.Type = Kind::String; v.StringValue = std::move(s); return v; }

        double AsNumber() const {
            if (Type == Kind::Int) return static_cast<double>(IntValue);
            if (Type == Kind::Float) return FloatValue;
            if (Type == Kind::Bool) return BoolValue ? 1.0 : 0.0;
            return 0.0;
        }

        bool IsTruthy() const {
            switch (Type) {
                case Kind::Null: return false;
                case Kind::Bool: return BoolValue;
                case Kind::Int: return IntValue != 0;
                case Kind::Float: return FloatValue != 0.0;
                case Kind::String: return !StringValue.empty();
                default: return true;
            }
        }

        std::string ToString() const;
    };

    struct Array {
        std::vector<Value> Items;
    };

    struct Dict {
        std::unordered_map<std::string, Value> Items;
    };

    struct Environment;

    struct Closure {
        std::vector<std::string> ParamNames;
        NodePtr Body;
        Ref<Environment> CapturedEnv;
        bool IsMethod = false;
    };

    struct ClassInfo {
        std::string Name;
        Ref<ClassInfo> BaseClass;
        std::unordered_map<std::string, Ref<Closure>> Methods;
        std::unordered_map<std::string, Value> StaticMembers;
        std::vector<std::string> SignalNames;
    };

    struct Instance {
        Ref<ClassInfo> Class;
        std::unordered_map<std::string, Value> Fields;
        // Signal name → connected callbacks
        std::unordered_map<std::string, std::vector<Ref<Closure>>> Signals;
    };

    struct Environment {
        Ref<Environment> Parent;
        std::unordered_map<std::string, Value> Vars;

        bool Get(const std::string& name, Value& out) const;
        bool Set(const std::string& name, const Value& val); // true if found & set
        void Declare(const std::string& name, const Value& val) { Vars[name] = val; }
    };

}}
