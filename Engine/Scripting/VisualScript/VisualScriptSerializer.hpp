#pragma once

#include "VisualScriptGraph.hpp"
#include <nlohmann/json.hpp>
#include <string>

namespace GameEngine {

    class VisualScriptSerializer {
    public:
        static nlohmann::json serialize(const VisualScriptGraph& graph);
        static Ref<VisualScriptGraph> deserialize(const nlohmann::json& j);

        static bool saveToFile(const VisualScriptGraph& graph, const std::string& path);
        static Ref<VisualScriptGraph> loadFromFile(const std::string& path);

    private:
        static nlohmann::json serializePin(const VisualScriptPin& pin);
        static nlohmann::json serializePinValue(const PinValue& value, PinType type);
        static PinValue deserializePinValue(const nlohmann::json& j, PinType type);
    };

} // namespace GameEngine
