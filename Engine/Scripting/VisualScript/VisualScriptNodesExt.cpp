#include "VisualScriptNodesExt.hpp"
#include "VisualScriptNodes.hpp"
#include "VisualScriptVM.hpp"

namespace GameEngine {

    void DeltaTimeNode::execute(VisualScriptVM& vm) {
        setPinValue(PinOut, vm.getDeltaTime());
    }

    void GameTimeNode::execute(VisualScriptVM& vm) {
        // VisualScriptVM doesn't expose total game time directly; surface the
        // current frame's delta as a placeholder so the node still produces
        // a value. (A future pass can plumb a global time accumulator through
        // the VM if needed.)
        setPinValue(PinOut, vm.getDeltaTime());
    }

    void WhileLoopNode::execute(VisualScriptVM& vm) {
        bool cond = getPinValueAs<bool>(PinCond);
        if (cond) vm.setNextFlowPin(PinBody);
        else vm.setNextFlowPin(PinDone);
    }

    void SwitchNode::execute(VisualScriptVM& vm) {
        int v = getPinValueAs<int>(PinValue);
        switch (v) {
            case 1: vm.setNextFlowPin(PinCase1); break;
            case 2: vm.setNextFlowPin(PinCase2); break;
            case 3: vm.setNextFlowPin(PinCase3); break;
            default: vm.setNextFlowPin(PinDefault); break;
        }
    }

    void registerExtendedNodes(VisualScriptNodeRegistry& reg) {
        // Math
        reg.registerNode("Modulo",        []() { return std::make_shared<ModNode>(); });
        reg.registerNode("Negate",        []() { return std::make_shared<NegateNode>(); });
        reg.registerNode("Min",           []() { return std::make_shared<MinNode>(); });
        reg.registerNode("Max",           []() { return std::make_shared<MaxNode>(); });
        reg.registerNode("Tan",           []() { return std::make_shared<TanNode>(); });
        reg.registerNode("Sqrt",          []() { return std::make_shared<SqrtNode>(); });
        reg.registerNode("Pow",           []() { return std::make_shared<PowNode>(); });
        reg.registerNode("Floor",         []() { return std::make_shared<FloorNode>(); });
        reg.registerNode("Ceil",          []() { return std::make_shared<CeilNode>(); });
        reg.registerNode("Round",         []() { return std::make_shared<RoundNode>(); });

        // Vector
        reg.registerNode("Vec3Add",       []() { return std::make_shared<Vec3AddNode>(); });
        reg.registerNode("Vec3Sub",       []() { return std::make_shared<Vec3SubNode>(); });
        reg.registerNode("Vec3Scale",     []() { return std::make_shared<Vec3ScaleNode>(); });
        reg.registerNode("Vec3Normalize", []() { return std::make_shared<Vec3NormalizeNode>(); });
        reg.registerNode("Vec3Dot",       []() { return std::make_shared<Vec3DotNode>(); });
        reg.registerNode("Vec3Cross",     []() { return std::make_shared<Vec3CrossNode>(); });
        reg.registerNode("Vec3Length",    []() { return std::make_shared<Vec3LengthNode>(); });
        reg.registerNode("Vec3Distance",  []() { return std::make_shared<Vec3DistanceNode>(); });

        // Logic
        reg.registerNode("Or",            []() { return std::make_shared<OrNode>(); });
        reg.registerNode("Equals",        []() { return std::make_shared<EqualsNode>(); });
        reg.registerNode("NotEquals",     []() { return std::make_shared<NotEqualsNode>(); });
        reg.registerNode("LessThan",      []() { return std::make_shared<LessThanNode>(); });
        reg.registerNode("GreaterThan",   []() { return std::make_shared<GreaterThanNode>(); });

        // Time
        reg.registerNode("DeltaTime",     []() { return std::make_shared<DeltaTimeNode>(); });
        reg.registerNode("GameTime",      []() { return std::make_shared<GameTimeNode>(); });

        // String
        reg.registerNode("StringConcat",  []() { return std::make_shared<StringConcatNode>(); });
        reg.registerNode("StringLength",  []() { return std::make_shared<StringLengthNode>(); });
        reg.registerNode("ToString",      []() { return std::make_shared<ToStringNode>(); });
        reg.registerNode("ParseFloat",    []() { return std::make_shared<ParseFloatNode>(); });

        // Flow
        reg.registerNode("While",         []() { return std::make_shared<WhileLoopNode>(); });
        reg.registerNode("Switch",        []() { return std::make_shared<SwitchNode>(); });
    }

}
