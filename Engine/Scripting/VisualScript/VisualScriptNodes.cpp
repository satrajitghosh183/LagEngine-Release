#include "VisualScriptNodes.hpp"
#include "VisualScriptVM.hpp"
#include "VisualScriptGraph.hpp"
#include <random>
#include <chrono>

namespace GameEngine {

    // =====================================================================
    // Flow Nodes
    // =====================================================================

    void EventStartNode::execute(VisualScriptVM& vm) {
        vm.setNextFlowPin(m_FlowOut);
    }

    void EventUpdateNode::execute(VisualScriptVM& vm) {
        setPinValue(m_DeltaTime, vm.getDeltaTime());
        vm.setNextFlowPin(m_FlowOut);
    }

    void IfElseNode::execute(VisualScriptVM& vm) {
        bool cond = getPinValueAs<bool>(m_Condition);
        vm.setNextFlowPin(cond ? m_TrueOut : m_FalseOut);
    }

    void ForLoopNode::execute(VisualScriptVM& vm) {
        int start = getPinValueAs<int>(m_Start);
        int end = getPinValueAs<int>(m_End);

        for (int i = start; i < end; i++) {
            setPinValue(m_Index, i);
            vm.setNextFlowPin(m_LoopBody);
        }
        vm.setNextFlowPin(m_Completed);
    }

    void SequenceNode::execute(VisualScriptVM& vm) {
        // Execute each output sequentially
        vm.setNextFlowPin(m_Out0);
    }

    // =====================================================================
    // Math Nodes (complex ones)
    // =====================================================================

    void RandomRangeNode::execute(VisualScriptVM& vm) {
        static std::mt19937 rng(static_cast<unsigned>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        float mn = getPinValueAs<float>(m_Min);
        float mx = getPinValueAs<float>(m_Max);
        std::uniform_real_distribution<float> dist(mn, mx);
        setPinValue(m_Result, dist(rng));
    }

    // =====================================================================
    // Entity Nodes
    // =====================================================================

    void GetPositionNode::execute(VisualScriptVM& vm) {
        // In a full implementation, this would query the scene
        // For now, read from VM variable
        std::string varName = "entity_" + std::to_string(getPinValueAs<uint32_t>(m_Entity)) + "_pos";
        PinValue val = vm.getVariable(varName);
        if (auto* v = std::get_if<glm::vec3>(&val)) {
            setPinValue(m_Position, *v);
        } else {
            setPinValue(m_Position, glm::vec3(0.0f));
        }
    }

    void SetPositionNode::execute(VisualScriptVM& vm) {
        uint32_t entityID = getPinValueAs<uint32_t>(m_Entity);
        glm::vec3 pos = getPinValueAs<glm::vec3>(m_Position);
        std::string varName = "entity_" + std::to_string(entityID) + "_pos";
        vm.setVariable(varName, pos);
        vm.setNextFlowPin(m_FlowOut);
    }

    void ApplyForceNode::execute(VisualScriptVM& vm) {
        uint32_t entityID = getPinValueAs<uint32_t>(m_Entity);
        glm::vec3 force = getPinValueAs<glm::vec3>(m_Force);
        std::string varName = "entity_" + std::to_string(entityID) + "_force";
        vm.setVariable(varName, force);
        vm.setNextFlowPin(m_FlowOut);
    }

    // =====================================================================
    // Input Nodes
    // =====================================================================

    void GetKeyNode::execute(VisualScriptVM& vm) {
        std::string key = getPinValueAs<std::string>(m_KeyName);
        // Query from VM variable (set by the host application)
        PinValue val = vm.getVariable("key_" + key);
        if (auto* b = std::get_if<bool>(&val)) {
            setPinValue(m_IsDown, *b);
        } else {
            setPinValue(m_IsDown, false);
        }
    }

    void GetMousePositionNode::execute(VisualScriptVM& vm) {
        PinValue val = vm.getVariable("mouse_position");
        if (auto* v = std::get_if<glm::vec2>(&val)) {
            setPinValue(m_Position, *v);
        } else {
            setPinValue(m_Position, glm::vec2(0.0f));
        }
    }

    // =====================================================================
    // Debug Nodes
    // =====================================================================

    void PrintNode::execute(VisualScriptVM& vm) {
        std::string msg = getPinValueAs<std::string>(m_Message);
        vm.log(msg);
        vm.setNextFlowPin(m_FlowOut);
    }

    // =====================================================================
    // Node Registration
    // =====================================================================

    void registerBuiltinNodes() {
        auto& reg = VisualScriptNodeRegistry::instance();

        // Flow
        reg.registerNode("EventStartNode", []() -> Ref<VisualScriptNode> { return CreateRef<EventStartNode>(); });
        reg.registerNode("EventUpdateNode", []() -> Ref<VisualScriptNode> { return CreateRef<EventUpdateNode>(); });
        reg.registerNode("IfElseNode", []() -> Ref<VisualScriptNode> { return CreateRef<IfElseNode>(); });
        reg.registerNode("ForLoopNode", []() -> Ref<VisualScriptNode> { return CreateRef<ForLoopNode>(); });
        reg.registerNode("SequenceNode", []() -> Ref<VisualScriptNode> { return CreateRef<SequenceNode>(); });

        // Math
        reg.registerNode("AddNode", []() -> Ref<VisualScriptNode> { return CreateRef<AddNode>(); });
        reg.registerNode("SubtractNode", []() -> Ref<VisualScriptNode> { return CreateRef<SubtractNode>(); });
        reg.registerNode("MultiplyNode", []() -> Ref<VisualScriptNode> { return CreateRef<MultiplyNode>(); });
        reg.registerNode("DivideNode", []() -> Ref<VisualScriptNode> { return CreateRef<DivideNode>(); });
        reg.registerNode("SinNode", []() -> Ref<VisualScriptNode> { return CreateRef<SinNode>(); });
        reg.registerNode("CosNode", []() -> Ref<VisualScriptNode> { return CreateRef<CosNode>(); });
        reg.registerNode("LerpNode", []() -> Ref<VisualScriptNode> { return CreateRef<LerpNode>(); });
        reg.registerNode("ClampNode", []() -> Ref<VisualScriptNode> { return CreateRef<ClampNode>(); });
        reg.registerNode("RandomRangeNode", []() -> Ref<VisualScriptNode> { return CreateRef<RandomRangeNode>(); });
        reg.registerNode("AbsNode", []() -> Ref<VisualScriptNode> { return CreateRef<AbsNode>(); });
        reg.registerNode("MakeVec3Node", []() -> Ref<VisualScriptNode> { return CreateRef<MakeVec3Node>(); });
        reg.registerNode("BreakVec3Node", []() -> Ref<VisualScriptNode> { return CreateRef<BreakVec3Node>(); });

        // Logic
        reg.registerNode("CompareNode", []() -> Ref<VisualScriptNode> { return CreateRef<CompareNode>(); });
        reg.registerNode("BoolNotNode", []() -> Ref<VisualScriptNode> { return CreateRef<BoolNotNode>(); });
        reg.registerNode("BoolAndNode", []() -> Ref<VisualScriptNode> { return CreateRef<BoolAndNode>(); });

        // Entity
        reg.registerNode("GetPositionNode", []() -> Ref<VisualScriptNode> { return CreateRef<GetPositionNode>(); });
        reg.registerNode("SetPositionNode", []() -> Ref<VisualScriptNode> { return CreateRef<SetPositionNode>(); });
        reg.registerNode("ApplyForceNode", []() -> Ref<VisualScriptNode> { return CreateRef<ApplyForceNode>(); });

        // Input
        reg.registerNode("GetKeyNode", []() -> Ref<VisualScriptNode> { return CreateRef<GetKeyNode>(); });
        reg.registerNode("GetMousePositionNode", []() -> Ref<VisualScriptNode> { return CreateRef<GetMousePositionNode>(); });

        // Debug
        reg.registerNode("PrintNode", []() -> Ref<VisualScriptNode> { return CreateRef<PrintNode>(); });
    }

} // namespace GameEngine
