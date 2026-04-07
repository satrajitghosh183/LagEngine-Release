#pragma once

#include "../../Core/Base.hpp"
#include "VisualScriptNode.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>

namespace GameEngine {

    class VisualScriptGraph;

    class VisualScriptVM {
    public:
        VisualScriptVM() = default;
        ~VisualScriptVM() = default;

        void initialize(Ref<VisualScriptGraph> graph);
        void reset();

        // Execute a specific event (e.g. "EventStart", "EventUpdate")
        void executeEvent(const std::string& eventName);

        // Called every frame — triggers EventUpdate nodes
        void update(float dt);

        // Flow control — called by flow nodes to schedule the next node
        void setNextFlowPin(uint32_t outputPinID);

        // Variable storage (global to this VM instance)
        void setVariable(const std::string& name, const PinValue& value);
        PinValue getVariable(const std::string& name) const;
        bool hasVariable(const std::string& name) const;

        // Delta time access for event nodes
        float getDeltaTime() const { return m_DeltaTime; }

        // Logging callback
        void setLogCallback(std::function<void(const std::string&)> cb) { m_LogCallback = std::move(cb); }
        void log(const std::string& message);

        // Execution state
        bool isRunning() const { return m_Running; }
        int getExecutedNodeCount() const { return m_ExecutedNodeCount; }

    private:
        void resolveInputPins(Ref<VisualScriptNode> node);
        void executeFlowChain(uint32_t startPinID);

        Ref<VisualScriptGraph> m_Graph;
        std::unordered_map<std::string, PinValue> m_Variables;
        float m_DeltaTime = 0.0f;
        bool m_Running = false;
        int m_ExecutedNodeCount = 0;
        uint32_t m_NextFlowPin = 0;

        static constexpr int MAX_EXECUTION_STEPS = 10000; // prevent infinite loops

        std::function<void(const std::string&)> m_LogCallback;
    };

} // namespace GameEngine
