#include "VisualScriptVM.hpp"
#include "VisualScriptGraph.hpp"
#include "VisualScriptNodes.hpp"

namespace GameEngine {

    void VisualScriptVM::initialize(Ref<VisualScriptGraph> graph) {
        m_Graph = graph;
        m_Variables.clear();
        m_Running = false;
        m_ExecutedNodeCount = 0;
    }

    void VisualScriptVM::reset() {
        m_Variables.clear();
        m_ExecutedNodeCount = 0;
        m_Running = false;
    }

    void VisualScriptVM::executeEvent(const std::string& eventName) {
        if (!m_Graph) return;
        m_Running = true;

        // Find all nodes with matching type name
        for (auto& node : m_Graph->getNodes()) {
            if (node->getNodeTypeName() == eventName) {
                node->execute(*this);
                // Follow the flow output
                for (auto& pin : node->getOutputPins()) {
                    if (pin.Type == PinType::Flow) {
                        executeFlowChain(pin.ID);
                    }
                }
            }
        }

        m_Running = false;
    }

    void VisualScriptVM::update(float dt) {
        m_DeltaTime = dt;
        executeEvent("EventUpdateNode");
    }

    void VisualScriptVM::setNextFlowPin(uint32_t outputPinID) {
        m_NextFlowPin = outputPinID;
    }

    void VisualScriptVM::resolveInputPins(Ref<VisualScriptNode> node) {
        if (!m_Graph) return;

        for (auto& pin : node->getInputPins()) {
            if (pin.Type == PinType::Flow) continue;

            // Find if there's a link feeding this input
            const VisualScriptLink* link = m_Graph->findLinkToInputPin(pin.ID);
            if (link) {
                auto srcNode = m_Graph->findNode(link->SourceNodeID);
                if (srcNode) {
                    // Execute the source node to compute its output (data flow)
                    resolveInputPins(srcNode);
                    srcNode->execute(*this);
                    // Copy value from source output pin to our input pin
                    PinValue val = srcNode->getPinValue(link->SourcePinID);
                    node->setPinValue(pin.ID, val);
                }
            }
        }
    }

    void VisualScriptVM::executeFlowChain(uint32_t startPinID) {
        if (!m_Graph) return;

        uint32_t currentPinID = startPinID;
        int steps = 0;

        while (currentPinID != 0 && steps < MAX_EXECUTION_STEPS) {
            steps++;
            m_ExecutedNodeCount++;

            // Find links from this output pin
            auto links = m_Graph->findLinksFromOutputPin(currentPinID);
            if (links.empty()) break;

            // Follow the first flow link
            const VisualScriptLink* flowLink = links[0];
            auto targetNode = m_Graph->findNode(flowLink->TargetNodeID);
            if (!targetNode) break;

            // Resolve data inputs for the target node
            resolveInputPins(targetNode);

            // Execute the node
            m_NextFlowPin = 0;
            targetNode->execute(*this);

            // Follow the flow output that the node set
            currentPinID = m_NextFlowPin;
        }
    }

    void VisualScriptVM::setVariable(const std::string& name, const PinValue& value) {
        m_Variables[name] = value;
    }

    PinValue VisualScriptVM::getVariable(const std::string& name) const {
        auto it = m_Variables.find(name);
        return it != m_Variables.end() ? it->second : PinValue{};
    }

    bool VisualScriptVM::hasVariable(const std::string& name) const {
        return m_Variables.count(name) > 0;
    }

    void VisualScriptVM::log(const std::string& message) {
        if (m_LogCallback) m_LogCallback(message);
    }

} // namespace GameEngine
