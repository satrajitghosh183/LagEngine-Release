#include "Value.hpp"
#include <sstream>

namespace GameEngine {
namespace LAGScript {

    std::string Value::ToString() const {
        std::ostringstream os;
        switch (Type) {
            case Kind::Null:   os << "null"; break;
            case Kind::Bool:   os << (BoolValue ? "true" : "false"); break;
            case Kind::Int:    os << IntValue; break;
            case Kind::Float:  os << FloatValue; break;
            case Kind::String: os << StringValue; break;
            case Kind::Array: {
                os << "[";
                if (ArrayValue) {
                    for (size_t i = 0; i < ArrayValue->Items.size(); i++) {
                        if (i) os << ", ";
                        os << ArrayValue->Items[i].ToString();
                    }
                }
                os << "]";
                break;
            }
            case Kind::Dict: {
                os << "{";
                if (DictValue) {
                    size_t i = 0;
                    for (auto& [k, v] : DictValue->Items) {
                        if (i++) os << ", ";
                        os << k << ": " << v.ToString();
                    }
                }
                os << "}";
                break;
            }
            case Kind::Function:       os << "<function>"; break;
            case Kind::NativeFunction: os << "<native>"; break;
            case Kind::Instance:
                os << (InstanceValue && InstanceValue->Class
                       ? "<" + InstanceValue->Class->Name + ">"
                       : "<instance>");
                break;
        }
        return os.str();
    }

    bool Environment::Get(const std::string& name, Value& out) const {
        auto it = Vars.find(name);
        if (it != Vars.end()) { out = it->second; return true; }
        return Parent ? Parent->Get(name, out) : false;
    }

    bool Environment::Set(const std::string& name, const Value& val) {
        auto it = Vars.find(name);
        if (it != Vars.end()) { it->second = val; return true; }
        return Parent ? Parent->Set(name, val) : false;
    }

}}
