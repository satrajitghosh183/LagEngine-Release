#include "VisualScriptSerializer.hpp"
#include "VisualScriptNodes.hpp"
#include <fstream>

namespace GameEngine {

    nlohmann::json VisualScriptSerializer::serializePinValue(const PinValue& value, PinType type) {
        switch (type) {
            case PinType::Bool:   return std::get<bool>(value);
            case PinType::Int:    return std::get<int>(value);
            case PinType::Float:  return std::get<float>(value);
            case PinType::String: return std::get<std::string>(value);
            case PinType::Entity: return std::get<uint32_t>(value);
            case PinType::Vec2: {
                auto v = std::get<glm::vec2>(value);
                return nlohmann::json::array({v.x, v.y});
            }
            case PinType::Vec3: {
                auto v = std::get<glm::vec3>(value);
                return nlohmann::json::array({v.x, v.y, v.z});
            }
            default: return nullptr;
        }
    }

    PinValue VisualScriptSerializer::deserializePinValue(const nlohmann::json& j, PinType type) {
        try {
            switch (type) {
                case PinType::Bool:   return j.get<bool>();
                case PinType::Int:    return j.get<int>();
                case PinType::Float:  return j.get<float>();
                case PinType::String: return j.get<std::string>();
                case PinType::Entity: return j.get<uint32_t>();
                case PinType::Vec2:   return glm::vec2(j[0].get<float>(), j[1].get<float>());
                case PinType::Vec3:   return glm::vec3(j[0].get<float>(), j[1].get<float>(), j[2].get<float>());
                default: break;
            }
        } catch (...) {}
        return VisualScriptPin::defaultForType(type);
    }

    nlohmann::json VisualScriptSerializer::serializePin(const VisualScriptPin& pin) {
        nlohmann::json j;
        j["id"] = pin.ID;
        j["name"] = pin.Name;
        j["type"] = static_cast<int>(pin.Type);
        j["direction"] = static_cast<int>(pin.Direction);
        if (pin.Type != PinType::Flow) {
            j["default"] = serializePinValue(pin.DefaultValue, pin.Type);
        }
        return j;
    }

    nlohmann::json VisualScriptSerializer::serialize(const VisualScriptGraph& graph) {
        nlohmann::json j;
        j["version"] = 1;
        j["name"] = graph.getName();
        j["nextNodeID"] = graph.getNextNodeID();
        j["nextLinkID"] = graph.getNextLinkID();

        auto& jNodes = j["nodes"];
        jNodes = nlohmann::json::array();
        for (auto& node : graph.getNodes()) {
            nlohmann::json jn;
            jn["id"] = node->ID;
            jn["type"] = node->getNodeTypeName();
            jn["name"] = node->Name;
            jn["category"] = node->Category;
            jn["position"] = {node->Position.x, node->Position.y};
            jn["nextPinID"] = node->getNextPinID();

            auto& jInputs = jn["inputs"];
            jInputs = nlohmann::json::array();
            for (auto& pin : node->getInputPins()) {
                jInputs.push_back(serializePin(pin));
            }

            auto& jOutputs = jn["outputs"];
            jOutputs = nlohmann::json::array();
            for (auto& pin : node->getOutputPins()) {
                jOutputs.push_back(serializePin(pin));
            }

            jNodes.push_back(jn);
        }

        auto& jLinks = j["links"];
        jLinks = nlohmann::json::array();
        for (auto& link : graph.getLinks()) {
            nlohmann::json jl;
            jl["id"] = link.ID;
            jl["srcNode"] = link.SourceNodeID;
            jl["srcPin"] = link.SourcePinID;
            jl["dstNode"] = link.TargetNodeID;
            jl["dstPin"] = link.TargetPinID;
            jLinks.push_back(jl);
        }

        return j;
    }

    Ref<VisualScriptGraph> VisualScriptSerializer::deserialize(const nlohmann::json& j) {
        auto graph = CreateRef<VisualScriptGraph>();

        if (j.contains("name")) graph->setName(j["name"].get<std::string>());

        auto& reg = VisualScriptNodeRegistry::instance();

        if (j.contains("nodes")) {
            for (auto& jn : j["nodes"]) {
                std::string typeName = jn["type"].get<std::string>();
                auto node = reg.create(typeName);
                if (!node) continue;

                node->ID = jn["id"].get<uint32_t>();
                if (jn.contains("name")) node->Name = jn["name"].get<std::string>();
                if (jn.contains("category")) node->Category = jn["category"].get<std::string>();
                if (jn.contains("position")) {
                    node->Position = glm::vec2(jn["position"][0].get<float>(),
                                                jn["position"][1].get<float>());
                }
                if (jn.contains("nextPinID")) node->setNextPinID(jn["nextPinID"].get<uint32_t>());

                // Note: pins are created in the node constructor, we just restore IDs/defaults
                // For full fidelity, a more complex approach would be needed

                graph->addNode(node);
            }
        }

        // Restore IDs
        if (j.contains("nextNodeID")) graph->setNextNodeID(j["nextNodeID"].get<uint32_t>());
        if (j.contains("nextLinkID")) graph->setNextLinkID(j["nextLinkID"].get<uint32_t>());

        // Restore links
        if (j.contains("links")) {
            for (auto& jl : j["links"]) {
                graph->addLink(
                    jl["srcNode"].get<uint32_t>(),
                    jl["srcPin"].get<uint32_t>(),
                    jl["dstNode"].get<uint32_t>(),
                    jl["dstPin"].get<uint32_t>()
                );
            }
        }

        return graph;
    }

    bool VisualScriptSerializer::saveToFile(const VisualScriptGraph& graph, const std::string& path) {
        try {
            nlohmann::json j = serialize(graph);
            std::ofstream out(path);
            if (!out.is_open()) return false;
            out << j.dump(2);
            return true;
        } catch (...) {
            return false;
        }
    }

    Ref<VisualScriptGraph> VisualScriptSerializer::loadFromFile(const std::string& path) {
        try {
            std::ifstream in(path);
            if (!in.is_open()) return nullptr;
            nlohmann::json j = nlohmann::json::parse(in);
            return deserialize(j);
        } catch (...) {
            return nullptr;
        }
    }

} // namespace GameEngine
