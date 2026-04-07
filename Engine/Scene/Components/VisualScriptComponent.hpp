#pragma once

#include "Component.hpp"
#include "../../Scripting/VisualScript/VisualScriptGraph.hpp"
#include "../../Scripting/VisualScript/VisualScriptVM.hpp"

namespace GameEngine {

    class VisualScriptComponent : public Component {
    public:
        COMPONENT_TYPE(VisualScriptComponent)

        std::string GraphPath;
        Ref<VisualScriptGraph> Graph;
        Ref<VisualScriptVM> VM;

        void OnCreate() override {
            if (!GraphPath.empty() && !Graph) {
                // Load from file
                // Graph = VisualScriptSerializer::loadFromFile(GraphPath);
            }
            if (Graph && !VM) {
                VM = CreateRef<VisualScriptVM>();
                VM->initialize(Graph);
            }
        }

        void OnUpdate(float deltaTime) override {
            if (VM) {
                VM->update(deltaTime);
            }
        }

        nlohmann::json Serialize() const override {
            nlohmann::json j;
            j["graphPath"] = GraphPath;
            return j;
        }

        void Deserialize(const nlohmann::json& data) override {
            if (data.contains("graphPath")) GraphPath = data["graphPath"].get<std::string>();
        }
    };

} // namespace GameEngine
