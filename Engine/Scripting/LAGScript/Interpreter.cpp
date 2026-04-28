#include "Interpreter.hpp"
#include "../../Core/Logger.hpp"

#include <cmath>
#include <iostream>
#include <random>
#include <string>

namespace GameEngine {
namespace LAGScript {

    Interpreter::Interpreter() {
        m_Globals = CreateRef<Environment>();
        InstallBuiltins();
    }

    void Interpreter::RegisterNative(const std::string& name, NativeFn fn) {
        Value v;
        v.Type = Value::Kind::NativeFunction;
        v.NativeValue = std::move(fn);
        m_Globals->Declare(name, v);
    }

    void Interpreter::SetGlobal(const std::string& name, const Value& v) {
        m_Globals->Declare(name, v);
    }

    Value Interpreter::GetGlobal(const std::string& name) const {
        Value out;
        return m_Globals->Get(name, out) ? out : Value::Null();
    }

    void Interpreter::ReportError(const std::string& msg, int line) {
        std::string full = msg;
        if (line > 0) full += " (line " + std::to_string(line) + ")";
        m_Errors.push_back(full);
        GE_CORE_WARN("LAGScript: {}", full);
    }

    void Interpreter::Run(NodePtr program) {
        if (!program) return;
        try {
            for (auto& stmt : program->Children) ExecStmt(stmt, m_Globals);
        } catch (const ReturnException&) {
            // Return at top level — ignore
        } catch (const std::exception& e) {
            ReportError(std::string("runtime: ") + e.what());
        }
    }

    Value Interpreter::Call(const std::string& name, const ValueList& args) {
        Value v;
        if (!m_Globals->Get(name, v)) {
            ReportError("undefined function '" + name + "'");
            return Value::Null();
        }
        if (v.Type == Value::Kind::NativeFunction) return v.NativeValue(*this, args);
        if (v.Type == Value::Kind::Function) return CallClosure(v.FunctionValue, args);
        ReportError("'" + name + "' is not callable");
        return Value::Null();
    }

    Value Interpreter::CallClosure(Ref<Closure> fn, const ValueList& args) {
        auto env = CreateRef<Environment>();
        env->Parent = fn->CapturedEnv ? fn->CapturedEnv : m_Globals;
        for (size_t i = 0; i < fn->ParamNames.size(); i++) {
            env->Declare(fn->ParamNames[i], i < args.size() ? args[i] : Value::Null());
        }
        try {
            if (fn->Body) {
                for (auto& stmt : fn->Body->Children) ExecStmt(stmt, env);
            }
        } catch (const ReturnException& r) {
            return r.Result;
        }
        return Value::Null();
    }

    void Interpreter::ExecStmt(const NodePtr& node, Ref<Environment> env) {
        if (!node) return;
        switch (node->Kind) {
            case NodeKind::ExprStmt:
                Eval(node->Children[0], env);
                break;
            case NodeKind::VarDecl:
            case NodeKind::ConstDecl: {
                Value v = node->Children.empty() ? Value::Null() : Eval(node->Children[0], env);
                env->Declare(node->Text, v);
                break;
            }
            case NodeKind::FuncDecl: {
                auto closure = CreateRef<Closure>();
                for (size_t i = 0; i < node->Children.size() - 1; i++) {
                    closure->ParamNames.push_back(node->Children[i]->Text);
                }
                closure->Body = node->Children.back();
                closure->CapturedEnv = env;
                Value v;
                v.Type = Value::Kind::Function;
                v.FunctionValue = closure;
                env->Declare(node->Text, v);
                break;
            }
            case NodeKind::ClassDecl: {
                auto info = CreateRef<ClassInfo>();
                info->Name = node->Text;
                if (!node->Op.empty()) {
                    auto it = m_Classes.find(node->Op);
                    if (it != m_Classes.end()) info->BaseClass = it->second;
                }
                if (!node->Children.empty()) {
                    auto body = node->Children.back();
                    for (auto& bstmt : body->Children) {
                        if (bstmt->Kind == NodeKind::FuncDecl) {
                            auto m = CreateRef<Closure>();
                            for (size_t i = 0; i < bstmt->Children.size() - 1; i++) {
                                m->ParamNames.push_back(bstmt->Children[i]->Text);
                            }
                            m->Body = bstmt->Children.back();
                            m->CapturedEnv = env;
                            m->IsMethod = true;
                            info->Methods[bstmt->Text] = m;
                        } else if (bstmt->Kind == NodeKind::SignalDecl) {
                            info->SignalNames.push_back(bstmt->Text);
                        }
                    }
                }
                m_Classes[info->Name] = info;
                break;
            }
            case NodeKind::IfStmt: {
                size_t i = 0;
                Value cond = Eval(node->Children[i++], env);
                auto thenBody = node->Children[i++];
                if (cond.IsTruthy()) {
                    for (auto& s : thenBody->Children) ExecStmt(s, env);
                    return;
                }
                // elif branches are nested IfStmt children (cond, body)
                while (i < node->Children.size()) {
                    auto child = node->Children[i];
                    if (child->Kind == NodeKind::IfStmt && child->Children.size() == 2) {
                        Value ec = Eval(child->Children[0], env);
                        if (ec.IsTruthy()) {
                            for (auto& s : child->Children[1]->Children) ExecStmt(s, env);
                            return;
                        }
                        i++;
                    } else {
                        // else body
                        for (auto& s : child->Children) ExecStmt(s, env);
                        return;
                    }
                }
                break;
            }
            case NodeKind::WhileStmt: {
                while (true) {
                    Value cond = Eval(node->Children[0], env);
                    if (!cond.IsTruthy()) break;
                    try {
                        for (auto& s : node->Children[1]->Children) ExecStmt(s, env);
                    } catch (const BreakException&) { goto while_end; }
                    catch (const ContinueException&) { continue; }
                }
                while_end:;
                break;
            }
            case NodeKind::ForStmt: {
                Value iter = Eval(node->Children[0], env);
                if (iter.Type == Value::Kind::Array && iter.ArrayValue) {
                    for (auto& item : iter.ArrayValue->Items) {
                        auto loopEnv = CreateRef<Environment>();
                        loopEnv->Parent = env;
                        loopEnv->Declare(node->Text, item);
                        try {
                            for (auto& s : node->Children[1]->Children) ExecStmt(s, loopEnv);
                        } catch (const BreakException&) { goto for_end; }
                        catch (const ContinueException&) { continue; }
                    }
                    for_end:;
                } else if (iter.Type == Value::Kind::Int || iter.Type == Value::Kind::Float) {
                    long long n = iter.Type == Value::Kind::Int ? iter.IntValue : (long long)iter.FloatValue;
                    for (long long k = 0; k < n; k++) {
                        auto loopEnv = CreateRef<Environment>();
                        loopEnv->Parent = env;
                        loopEnv->Declare(node->Text, Value::Int(k));
                        try {
                            for (auto& s : node->Children[1]->Children) ExecStmt(s, loopEnv);
                        } catch (const BreakException&) { return; }
                        catch (const ContinueException&) { continue; }
                    }
                }
                break;
            }
            case NodeKind::ReturnStmt: {
                Value v = node->Children.empty() ? Value::Null() : Eval(node->Children[0], env);
                throw ReturnException{v};
            }
            case NodeKind::BreakStmt:    throw BreakException{};
            case NodeKind::ContinueStmt: throw ContinueException{};
            case NodeKind::SignalDecl: /* noop at runtime (info stored on class) */ break;
            case NodeKind::EmitStmt: {
                Value selfVal;
                if (env->Get("self", selfVal) && selfVal.Type == Value::Kind::Instance && selfVal.InstanceValue) {
                    auto it = selfVal.InstanceValue->Signals.find(node->Text);
                    if (it != selfVal.InstanceValue->Signals.end()) {
                        ValueList args;
                        for (auto& a : node->Children) args.push_back(Eval(a, env));
                        for (auto& cb : it->second) CallClosure(cb, args);
                    }
                }
                break;
            }
            default:
                Eval(node, env);
                break;
        }
    }

    Value Interpreter::Eval(const NodePtr& node, Ref<Environment> env) {
        if (!node) return Value::Null();
        switch (node->Kind) {
            case NodeKind::NumberLit: {
                if (node->Number == std::floor(node->Number)) return Value::Int((long long)node->Number);
                return Value::Float(node->Number);
            }
            case NodeKind::StringLit: return Value::String(node->Text);
            case NodeKind::BoolLit:   return Value::Bool(node->BoolValue);
            case NodeKind::NullLit:   return Value::Null();
            case NodeKind::Identifier: {
                Value out;
                if (env->Get(node->Text, out)) return out;
                // Maybe it's a class reference?
                auto it = m_Classes.find(node->Text);
                if (it != m_Classes.end()) {
                    Value v; v.Type = Value::Kind::NativeFunction;
                    auto cls = it->second;
                    v.NativeValue = [cls](Interpreter& interp, const ValueList& args) -> Value {
                        auto inst = CreateRef<Instance>();
                        inst->Class = cls;
                        Value iv; iv.Type = Value::Kind::Instance; iv.InstanceValue = inst;
                        // Call _init if exists
                        auto ctor = cls->Methods.find("_init");
                        if (ctor != cls->Methods.end()) {
                            auto env2 = CreateRef<Environment>();
                            env2->Parent = ctor->second->CapturedEnv;
                            env2->Declare("self", iv);
                            for (size_t i = 0; i < ctor->second->ParamNames.size(); i++) {
                                env2->Declare(ctor->second->ParamNames[i],
                                              i < args.size() ? args[i] : Value::Null());
                            }
                            try {
                                for (auto& s : ctor->second->Body->Children) interp.ExecStmt(s, env2);
                            } catch (const ReturnException&) {}
                        }
                        return iv;
                    };
                    return v;
                }
                ReportError("undefined '" + node->Text + "'", node->Line);
                return Value::Null();
            }
            case NodeKind::Self: {
                Value v;
                if (env->Get("self", v)) return v;
                return Value::Null();
            }
            case NodeKind::BinaryOp:    return EvalBinary(node, env);
            case NodeKind::UnaryOp: {
                Value v = Eval(node->Children[0], env);
                if (node->Op == "-") {
                    if (v.Type == Value::Kind::Int) return Value::Int(-v.IntValue);
                    return Value::Float(-v.AsNumber());
                }
                if (node->Op == "not") return Value::Bool(!v.IsTruthy());
                return v;
            }
            case NodeKind::Call:         return EvalCall(node, env);
            case NodeKind::MemberAccess: return EvalMember(node, env);
            case NodeKind::IndexAccess: {
                Value obj = Eval(node->Children[0], env);
                Value idx = Eval(node->Children[1], env);
                if (obj.Type == Value::Kind::Array && obj.ArrayValue) {
                    long long i = idx.Type == Value::Kind::Int ? idx.IntValue : (long long)idx.AsNumber();
                    if (i < 0 || i >= (long long)obj.ArrayValue->Items.size()) return Value::Null();
                    return obj.ArrayValue->Items[i];
                }
                if (obj.Type == Value::Kind::Dict && obj.DictValue) {
                    auto it = obj.DictValue->Items.find(idx.ToString());
                    if (it != obj.DictValue->Items.end()) return it->second;
                }
                return Value::Null();
            }
            case NodeKind::ArrayLit: {
                Value v; v.Type = Value::Kind::Array;
                v.ArrayValue = CreateRef<Array>();
                for (auto& c : node->Children) v.ArrayValue->Items.push_back(Eval(c, env));
                return v;
            }
            case NodeKind::DictLit: {
                Value v; v.Type = Value::Kind::Dict;
                v.DictValue = CreateRef<Dict>();
                for (size_t i = 0; i + 1 < node->Children.size(); i += 2) {
                    Value k = Eval(node->Children[i], env);
                    Value val = Eval(node->Children[i + 1], env);
                    v.DictValue->Items[k.ToString()] = val;
                }
                return v;
            }
            case NodeKind::Assign: return EvalAssign(node, env);
            default: return Value::Null();
        }
    }

    Value Interpreter::EvalBinary(const NodePtr& node, Ref<Environment> env) {
        Value a = Eval(node->Children[0], env);
        Value b = Eval(node->Children[1], env);

        if (node->Op == "and") return Value::Bool(a.IsTruthy() && b.IsTruthy());
        if (node->Op == "or")  return Value::Bool(a.IsTruthy() || b.IsTruthy());

        if (node->Op == "+") {
            if (a.Type == Value::Kind::String || b.Type == Value::Kind::String)
                return Value::String(a.ToString() + b.ToString());
        }

        double av = a.AsNumber(), bv = b.AsNumber();
        bool bothInt = (a.Type == Value::Kind::Int) && (b.Type == Value::Kind::Int);

        if (node->Op == "+") return bothInt ? Value::Int(a.IntValue + b.IntValue) : Value::Float(av + bv);
        if (node->Op == "-") return bothInt ? Value::Int(a.IntValue - b.IntValue) : Value::Float(av - bv);
        if (node->Op == "*") return bothInt ? Value::Int(a.IntValue * b.IntValue) : Value::Float(av * bv);
        if (node->Op == "/") return bv == 0.0 ? Value::Float(0.0) : Value::Float(av / bv);
        if (node->Op == "%") return bv == 0.0 ? Value::Float(0.0) : Value::Float(std::fmod(av, bv));
        if (node->Op == "==") return Value::Bool(av == bv);
        if (node->Op == "!=") return Value::Bool(av != bv);
        if (node->Op == "<")  return Value::Bool(av < bv);
        if (node->Op == ">")  return Value::Bool(av > bv);
        if (node->Op == "<=") return Value::Bool(av <= bv);
        if (node->Op == ">=") return Value::Bool(av >= bv);
        return Value::Null();
    }

    Value Interpreter::EvalCall(const NodePtr& node, Ref<Environment> env) {
        Value callee = Eval(node->Children[0], env);
        ValueList args;
        for (size_t i = 1; i < node->Children.size(); i++) {
            args.push_back(Eval(node->Children[i], env));
        }

        if (callee.Type == Value::Kind::NativeFunction) return callee.NativeValue(*this, args);
        if (callee.Type == Value::Kind::Function) {
            // Bind 'self' if this was a method access
            if (node->Children[0]->Kind == NodeKind::MemberAccess) {
                Value selfObj = Eval(node->Children[0]->Children[0], env);
                if (selfObj.Type == Value::Kind::Instance) {
                    auto env2 = CreateRef<Environment>();
                    env2->Parent = callee.FunctionValue->CapturedEnv;
                    env2->Declare("self", selfObj);
                    for (size_t i = 0; i < callee.FunctionValue->ParamNames.size(); i++) {
                        env2->Declare(callee.FunctionValue->ParamNames[i],
                                      i < args.size() ? args[i] : Value::Null());
                    }
                    try {
                        for (auto& s : callee.FunctionValue->Body->Children) ExecStmt(s, env2);
                    } catch (const ReturnException& r) { return r.Result; }
                    return Value::Null();
                }
            }
            return CallClosure(callee.FunctionValue, args);
        }
        ReportError("attempt to call non-function", node->Line);
        return Value::Null();
    }

    Value Interpreter::EvalMember(const NodePtr& node, Ref<Environment> env) {
        Value obj = Eval(node->Children[0], env);
        if (obj.Type == Value::Kind::Instance && obj.InstanceValue) {
            auto& fields = obj.InstanceValue->Fields;
            auto fit = fields.find(node->Text);
            if (fit != fields.end()) return fit->second;

            // Method lookup
            auto cls = obj.InstanceValue->Class;
            while (cls) {
                auto mit = cls->Methods.find(node->Text);
                if (mit != cls->Methods.end()) {
                    Value v; v.Type = Value::Kind::Function;
                    v.FunctionValue = mit->second;
                    return v;
                }
                cls = cls->BaseClass;
            }
        } else if (obj.Type == Value::Kind::Dict && obj.DictValue) {
            auto it = obj.DictValue->Items.find(node->Text);
            if (it != obj.DictValue->Items.end()) return it->second;
        }
        return Value::Null();
    }

    Value Interpreter::EvalAssign(const NodePtr& node, Ref<Environment> env) {
        Value value = Eval(node->Children[1], env);
        auto& target = node->Children[0];
        if (target->Kind == NodeKind::Identifier) {
            if (!env->Set(target->Text, value)) env->Declare(target->Text, value);
        } else if (target->Kind == NodeKind::MemberAccess) {
            Value obj = Eval(target->Children[0], env);
            if (obj.Type == Value::Kind::Instance && obj.InstanceValue) {
                obj.InstanceValue->Fields[target->Text] = value;
            } else if (obj.Type == Value::Kind::Dict && obj.DictValue) {
                obj.DictValue->Items[target->Text] = value;
            }
        } else if (target->Kind == NodeKind::IndexAccess) {
            Value obj = Eval(target->Children[0], env);
            Value idx = Eval(target->Children[1], env);
            if (obj.Type == Value::Kind::Array && obj.ArrayValue) {
                long long i = idx.Type == Value::Kind::Int ? idx.IntValue : (long long)idx.AsNumber();
                if (i >= 0 && i < (long long)obj.ArrayValue->Items.size())
                    obj.ArrayValue->Items[i] = value;
            } else if (obj.Type == Value::Kind::Dict && obj.DictValue) {
                obj.DictValue->Items[idx.ToString()] = value;
            }
        }
        return value;
    }

    void Interpreter::InstallBuiltins() {
        RegisterNative("print", [](Interpreter&, const ValueList& args) -> Value {
            for (size_t i = 0; i < args.size(); i++) {
                if (i) std::cout << " ";
                std::cout << args[i].ToString();
            }
            std::cout << std::endl;
            return Value::Null();
        });
        RegisterNative("len", [](Interpreter&, const ValueList& args) -> Value {
            if (args.empty()) return Value::Int(0);
            const Value& v = args[0];
            if (v.Type == Value::Kind::String) return Value::Int((long long)v.StringValue.size());
            if (v.Type == Value::Kind::Array && v.ArrayValue) return Value::Int((long long)v.ArrayValue->Items.size());
            if (v.Type == Value::Kind::Dict && v.DictValue) return Value::Int((long long)v.DictValue->Items.size());
            return Value::Int(0);
        });
        RegisterNative("range", [](Interpreter&, const ValueList& args) -> Value {
            Value v; v.Type = Value::Kind::Array; v.ArrayValue = CreateRef<Array>();
            long long start = 0, end = 0, step = 1;
            if (args.size() == 1) end = (long long)args[0].AsNumber();
            else if (args.size() >= 2) { start = (long long)args[0].AsNumber(); end = (long long)args[1].AsNumber(); }
            if (args.size() >= 3) step = (long long)args[2].AsNumber();
            if (step == 0) step = 1;
            if (step > 0) for (long long i = start; i < end; i += step) v.ArrayValue->Items.push_back(Value::Int(i));
            else          for (long long i = start; i > end; i += step) v.ArrayValue->Items.push_back(Value::Int(i));
            return v;
        });
        RegisterNative("str", [](Interpreter&, const ValueList& args) -> Value {
            return Value::String(args.empty() ? std::string() : args[0].ToString());
        });
        RegisterNative("int", [](Interpreter&, const ValueList& args) -> Value {
            return Value::Int(args.empty() ? 0 : (long long)args[0].AsNumber());
        });
        RegisterNative("float", [](Interpreter&, const ValueList& args) -> Value {
            return Value::Float(args.empty() ? 0.0 : args[0].AsNumber());
        });
        RegisterNative("abs", [](Interpreter&, const ValueList& args) -> Value {
            if (args.empty()) return Value::Int(0);
            return Value::Float(std::abs(args[0].AsNumber()));
        });
        RegisterNative("sin", [](Interpreter&, const ValueList& args) -> Value {
            return Value::Float(args.empty() ? 0.0 : std::sin(args[0].AsNumber()));
        });
        RegisterNative("cos", [](Interpreter&, const ValueList& args) -> Value {
            return Value::Float(args.empty() ? 0.0 : std::cos(args[0].AsNumber()));
        });
        RegisterNative("sqrt", [](Interpreter&, const ValueList& args) -> Value {
            if (args.empty()) return Value::Float(0.0);
            double v = args[0].AsNumber();
            return Value::Float(v < 0.0 ? 0.0 : std::sqrt(v));
        });
        RegisterNative("random", [](Interpreter&, const ValueList&) -> Value {
            static std::mt19937_64 gen{std::random_device{}()};
            static std::uniform_real_distribution<double> dist(0.0, 1.0);
            return Value::Float(dist(gen));
        });
        RegisterNative("append", [](Interpreter&, const ValueList& args) -> Value {
            if (args.size() >= 2 && args[0].Type == Value::Kind::Array && args[0].ArrayValue) {
                args[0].ArrayValue->Items.push_back(args[1]);
            }
            return Value::Null();
        });
    }

}}
