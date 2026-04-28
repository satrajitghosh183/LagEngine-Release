#include "LagsceneSerializer.hpp"
#include "../Core/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace GameEngine {

    // ---- Property::ToText --------------------------------------------------

    std::string LagsceneSerializer::Property::ToText() const {
        std::ostringstream os;
        switch (Type) {
            case Kind::Null:   os << "null"; break;
            case Kind::Bool:   os << (BoolValue ? "true" : "false"); break;
            case Kind::Int:    os << IntValue; break;
            case Kind::Float:  os << FloatValue; break;
            case Kind::String: os << '"' << StringValue << '"'; break;
            case Kind::Vector2:
                os << "Vector2(" << VectorValue[0] << ", " << VectorValue[1] << ")";
                break;
            case Kind::Vector3:
                os << "Vector3(" << VectorValue[0] << ", " << VectorValue[1] << ", " << VectorValue[2] << ")";
                break;
            case Kind::Vector4:
                os << "Vector4(" << VectorValue[0] << ", " << VectorValue[1] << ", " << VectorValue[2] << ", " << VectorValue[3] << ")";
                break;
            case Kind::Quat:
                os << "Quat(" << VectorValue[0] << ", " << VectorValue[1] << ", " << VectorValue[2] << ", " << VectorValue[3] << ")";
                break;
            case Kind::Transform: {
                os << "Transform(";
                for (size_t i = 0; i < TransformValue.size(); i++) {
                    if (i) os << ", ";
                    os << TransformValue[i];
                }
                os << ")";
                break;
            }
            case Kind::ExtRef: os << "ExtResource(\"" << RefValue << "\")"; break;
            case Kind::SubRef: os << "SubResource(\"" << RefValue << "\")"; break;
            case Kind::UIDRef: os << "UID(\"" << RefValue << "\")"; break;
            case Kind::Array: {
                os << "[";
                for (size_t i = 0; i < ArrayItems.size(); i++) {
                    if (i) os << ", ";
                    os << ArrayItems[i].ToText();
                }
                os << "]";
                break;
            }
            case Kind::Dict: {
                os << "{";
                size_t i = 0;
                for (const auto& [k, v] : DictItems) {
                    if (i++) os << ", ";
                    os << '"' << k << "\": " << v.ToText();
                }
                os << "}";
                break;
            }
        }
        return os.str();
    }

    // ---- Property::FromText ------------------------------------------------

    static void SkipWS(const std::string& s, size_t& pos) {
        while (pos < s.size() && std::isspace(static_cast<unsigned char>(s[pos]))) pos++;
    }

    static std::string ReadIdent(const std::string& s, size_t& pos) {
        SkipWS(s, pos);
        std::string out;
        while (pos < s.size() && (std::isalnum(static_cast<unsigned char>(s[pos])) || s[pos] == '_')) {
            out += s[pos++];
        }
        return out;
    }

    static std::string ReadQuoted(const std::string& s, size_t& pos) {
        SkipWS(s, pos);
        std::string out;
        if (pos >= s.size() || s[pos] != '"') return out;
        pos++;
        while (pos < s.size() && s[pos] != '"') {
            if (s[pos] == '\\' && pos + 1 < s.size()) { out += s[++pos]; pos++; }
            else out += s[pos++];
        }
        if (pos < s.size()) pos++; // closing quote
        return out;
    }

    static LagsceneSerializer::Property ParseValue(const std::string& s, size_t& pos);

    static std::vector<float> ParseFloats(const std::string& s, size_t& pos) {
        std::vector<float> out;
        SkipWS(s, pos);
        if (pos < s.size() && s[pos] == '(') pos++;
        while (pos < s.size() && s[pos] != ')') {
            SkipWS(s, pos);
            size_t start = pos;
            while (pos < s.size() && s[pos] != ',' && s[pos] != ')') pos++;
            try { out.push_back(std::stof(s.substr(start, pos - start))); } catch (...) {}
            if (pos < s.size() && s[pos] == ',') pos++;
        }
        if (pos < s.size() && s[pos] == ')') pos++;
        return out;
    }

    static LagsceneSerializer::Property ParseValue(const std::string& s, size_t& pos) {
        using P = LagsceneSerializer::Property;
        P result;
        SkipWS(s, pos);
        if (pos >= s.size()) return result;

        char c = s[pos];

        if (c == '"') {
            result.Type = P::Kind::String;
            result.StringValue = ReadQuoted(s, pos);
            return result;
        }
        if (c == '[') {
            pos++;
            result.Type = P::Kind::Array;
            while (pos < s.size() && s[pos] != ']') {
                result.ArrayItems.push_back(ParseValue(s, pos));
                SkipWS(s, pos);
                if (pos < s.size() && s[pos] == ',') pos++;
                SkipWS(s, pos);
            }
            if (pos < s.size()) pos++;
            return result;
        }
        if (c == '{') {
            pos++;
            result.Type = P::Kind::Dict;
            while (pos < s.size() && s[pos] != '}') {
                SkipWS(s, pos);
                std::string key = ReadQuoted(s, pos);
                SkipWS(s, pos);
                if (pos < s.size() && s[pos] == ':') pos++;
                result.DictItems.emplace_back(key, ParseValue(s, pos));
                SkipWS(s, pos);
                if (pos < s.size() && s[pos] == ',') pos++;
                SkipWS(s, pos);
            }
            if (pos < s.size()) pos++;
            return result;
        }
        if (std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '+') {
            size_t start = pos;
            bool hasDot = false;
            if (c == '-' || c == '+') pos++;
            while (pos < s.size() && (std::isdigit(static_cast<unsigned char>(s[pos])) || s[pos] == '.')) {
                if (s[pos] == '.') hasDot = true;
                pos++;
            }
            try {
                if (hasDot) { result.Type = P::Kind::Float; result.FloatValue = std::stod(s.substr(start, pos - start)); }
                else        { result.Type = P::Kind::Int;   result.IntValue  = std::stoll(s.substr(start, pos - start)); }
            } catch (...) {}
            return result;
        }

        std::string ident = ReadIdent(s, pos);
        if (ident == "true")  { result.Type = P::Kind::Bool; result.BoolValue = true; return result; }
        if (ident == "false") { result.Type = P::Kind::Bool; result.BoolValue = false; return result; }
        if (ident == "null")  { result.Type = P::Kind::Null; return result; }

        if (ident == "Vector2")   { result.Type = P::Kind::Vector2; result.VectorValue = ParseFloats(s, pos); return result; }
        if (ident == "Vector3")   { result.Type = P::Kind::Vector3; result.VectorValue = ParseFloats(s, pos); return result; }
        if (ident == "Vector4")   { result.Type = P::Kind::Vector4; result.VectorValue = ParseFloats(s, pos); return result; }
        if (ident == "Quat")      { result.Type = P::Kind::Quat; result.VectorValue = ParseFloats(s, pos); return result; }
        if (ident == "Transform") { result.Type = P::Kind::Transform; result.TransformValue = ParseFloats(s, pos); return result; }

        if (ident == "ExtResource") {
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == '(') pos++;
            result.Type = P::Kind::ExtRef;
            result.RefValue = ReadQuoted(s, pos);
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == ')') pos++;
            return result;
        }
        if (ident == "SubResource") {
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == '(') pos++;
            result.Type = P::Kind::SubRef;
            result.RefValue = ReadQuoted(s, pos);
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == ')') pos++;
            return result;
        }
        if (ident == "UID") {
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == '(') pos++;
            result.Type = P::Kind::UIDRef;
            result.RefValue = ReadQuoted(s, pos);
            SkipWS(s, pos);
            if (pos < s.size() && s[pos] == ')') pos++;
            return result;
        }

        // Unknown — treat as string
        result.Type = P::Kind::String;
        result.StringValue = ident;
        return result;
    }

    LagsceneSerializer::Property
    LagsceneSerializer::Property::FromText(const std::string& s, int& outErrors) {
        size_t pos = 0;
        Property p = ParseValue(s, pos);
        if (p.Type == Kind::Null && !s.empty() && s[0] != 'n') outErrors++;
        return p;
    }

    // ---- Section parsing ---------------------------------------------------

    struct Section {
        std::string Header;
        std::unordered_map<std::string, std::string> HeaderAttrs;
        std::vector<std::pair<std::string, std::string>> Props;
    };

    static std::vector<Section> ParseSections(const std::string& source) {
        std::vector<Section> sections;
        std::istringstream ss(source);
        std::string line;
        Section* current = nullptr;

        while (std::getline(ss, line)) {
            // Trim
            size_t start = 0; while (start < line.size() && std::isspace((unsigned char)line[start])) start++;
            if (start >= line.size()) continue;
            if (line[start] == ';' || line[start] == '#') continue;
            std::string trimmed = line.substr(start);

            if (!trimmed.empty() && trimmed[0] == '[') {
                size_t closeBracket = trimmed.find(']');
                if (closeBracket == std::string::npos) continue;
                sections.emplace_back();
                current = &sections.back();
                std::string body = trimmed.substr(1, closeBracket - 1);

                // First token is the header type
                size_t sp = body.find(' ');
                if (sp == std::string::npos) { current->Header = body; continue; }
                current->Header = body.substr(0, sp);

                // Parse remaining key=value attributes
                size_t p = sp;
                while (p < body.size()) {
                    while (p < body.size() && std::isspace((unsigned char)body[p])) p++;
                    if (p >= body.size()) break;
                    size_t keyStart = p;
                    while (p < body.size() && body[p] != '=' && !std::isspace((unsigned char)body[p])) p++;
                    std::string key = body.substr(keyStart, p - keyStart);
                    if (p < body.size() && body[p] == '=') p++;
                    std::string value;
                    if (p < body.size() && body[p] == '"') {
                        p++;
                        while (p < body.size() && body[p] != '"') value += body[p++];
                        if (p < body.size()) p++;
                    } else {
                        while (p < body.size() && !std::isspace((unsigned char)body[p])) value += body[p++];
                    }
                    current->HeaderAttrs[key] = value;
                }
            } else if (current) {
                size_t eq = trimmed.find('=');
                if (eq == std::string::npos) continue;
                std::string key = trimmed.substr(0, eq);
                while (!key.empty() && std::isspace((unsigned char)key.back())) key.pop_back();
                std::string val = trimmed.substr(eq + 1);
                size_t vs = 0; while (vs < val.size() && std::isspace((unsigned char)val[vs])) vs++;
                val = val.substr(vs);
                current->Props.emplace_back(key, val);
            }
        }
        return sections;
    }

    bool LagsceneSerializer::Parse(const std::string& source, Document& out, std::string* error) {
        auto sections = ParseSections(source);
        if (sections.empty()) {
            if (error) *error = "empty or invalid file";
            return false;
        }

        for (const auto& sec : sections) {
            if (sec.Header == "lagscene") {
                auto fv = sec.HeaderAttrs.find("format");
                if (fv != sec.HeaderAttrs.end()) {
                    try { out.FormatVersion = std::stoi(fv->second); } catch (...) {}
                }
                auto uid = sec.HeaderAttrs.find("uid");
                if (uid != sec.HeaderAttrs.end()) out.UID = uid->second;
            } else if (sec.Header == "ext_resource") {
                ExtResource r;
                if (auto t = sec.HeaderAttrs.find("type"); t != sec.HeaderAttrs.end()) r.Type = t->second;
                if (auto p = sec.HeaderAttrs.find("path"); p != sec.HeaderAttrs.end()) r.Path = p->second;
                if (auto u = sec.HeaderAttrs.find("uid");  u != sec.HeaderAttrs.end()) r.UID  = u->second;
                if (auto i = sec.HeaderAttrs.find("id");   i != sec.HeaderAttrs.end()) r.ID   = i->second;
                out.ExtResources.push_back(r);
            } else if (sec.Header == "sub_resource") {
                SubResource r;
                if (auto t = sec.HeaderAttrs.find("type"); t != sec.HeaderAttrs.end()) r.Type = t->second;
                if (auto i = sec.HeaderAttrs.find("id");   i != sec.HeaderAttrs.end()) r.ID   = i->second;
                int errs = 0;
                for (const auto& [k, v] : sec.Props) r.Properties[k] = Property::FromText(v, errs);
                out.SubResources.push_back(r);
            } else if (sec.Header == "node") {
                NodeEntry n;
                if (auto na = sec.HeaderAttrs.find("name");     na != sec.HeaderAttrs.end()) n.Name = na->second;
                if (auto ty = sec.HeaderAttrs.find("type");     ty != sec.HeaderAttrs.end()) n.Type = ty->second;
                if (auto pa = sec.HeaderAttrs.find("parent");   pa != sec.HeaderAttrs.end()) n.Parent = pa->second;
                if (auto in = sec.HeaderAttrs.find("instance"); in != sec.HeaderAttrs.end()) n.InstancePath = in->second;
                int errs = 0;
                for (const auto& [k, v] : sec.Props) n.Properties[k] = Property::FromText(v, errs);
                out.Nodes.push_back(n);
            }
        }

        return true;
    }

    std::string LagsceneSerializer::Write(const Document& doc) {
        std::ostringstream os;
        os << "[lagscene format=" << doc.FormatVersion;
        if (!doc.UID.empty()) os << " uid=\"" << doc.UID << "\"";
        os << "]\n\n";

        for (const auto& r : doc.ExtResources) {
            os << "[ext_resource type=\"" << r.Type << "\"";
            if (!r.Path.empty()) os << " path=\"" << r.Path << "\"";
            if (!r.UID.empty())  os << " uid=\"" << r.UID << "\"";
            if (!r.ID.empty())   os << " id=\"" << r.ID << "\"";
            os << "]\n";
        }
        if (!doc.ExtResources.empty()) os << "\n";

        for (const auto& r : doc.SubResources) {
            os << "[sub_resource type=\"" << r.Type << "\" id=\"" << r.ID << "\"]\n";
            for (const auto& [k, v] : r.Properties) os << k << " = " << v.ToText() << "\n";
            os << "\n";
        }

        for (const auto& n : doc.Nodes) {
            os << "[node name=\"" << n.Name << "\" type=\"" << n.Type << "\"";
            if (!n.Parent.empty()) os << " parent=\"" << n.Parent << "\"";
            if (!n.InstancePath.empty()) os << " instance=\"" << n.InstancePath << "\"";
            os << "]\n";
            for (const auto& [k, v] : n.Properties) os << k << " = " << v.ToText() << "\n";
            os << "\n";
        }

        return os.str();
    }

    bool LagsceneSerializer::LoadFile(const std::string& path, Document& out, std::string* error) {
        std::ifstream f(path);
        if (!f.is_open()) {
            if (error) *error = "failed to open " + path;
            return false;
        }
        std::ostringstream ss; ss << f.rdbuf();
        return Parse(ss.str(), out, error);
    }

    bool LagsceneSerializer::SaveFile(const std::string& path, const Document& doc) {
        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << Write(doc);
        return true;
    }

    // Stubs for Scene bridge — full integration handled by Scene::LoadLagscene
    bool LagsceneSerializer::BuildSceneFromDoc(const Document&, Scene&) {
        GE_CORE_INFO("Lagscene: scene reconstruction requires Scene API integration");
        return true;
    }

    LagsceneSerializer::Document LagsceneSerializer::BuildDocFromScene(const Scene&) {
        Document doc;
        doc.FormatVersion = 1;
        return doc;
    }

}
