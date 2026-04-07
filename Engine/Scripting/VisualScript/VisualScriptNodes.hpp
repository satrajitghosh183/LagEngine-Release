#pragma once

#include "VisualScriptNode.hpp"
#include "VisualScriptVM.hpp"
#include <cmath>

namespace GameEngine {

    // =====================================================================
    // Node Registry
    // =====================================================================

    class VisualScriptNodeRegistry {
    public:
        using FactoryFn = std::function<Ref<VisualScriptNode>()>;

        static VisualScriptNodeRegistry& instance() {
            static VisualScriptNodeRegistry s;
            return s;
        }

        void registerNode(const std::string& typeName, FactoryFn factory) {
            m_Factories[typeName] = std::move(factory);
        }

        Ref<VisualScriptNode> create(const std::string& typeName) const {
            auto it = m_Factories.find(typeName);
            if (it != m_Factories.end()) return it->second();
            return nullptr;
        }

        std::vector<std::string> getRegisteredTypes() const {
            std::vector<std::string> types;
            for (auto& [k, v] : m_Factories) types.push_back(k);
            return types;
        }

    private:
        std::unordered_map<std::string, FactoryFn> m_Factories;
    };

    // Helper macro for registering nodes
    #define REGISTER_VS_NODE(Class) \
        static bool _reg_##Class = []() { \
            VisualScriptNodeRegistry::instance().registerNode(#Class, []() -> Ref<VisualScriptNode> { \
                return CreateRef<Class>(); \
            }); \
            return true; \
        }()

    // =====================================================================
    // Flow Nodes
    // =====================================================================

    class EventStartNode : public VisualScriptNode {
    public:
        EventStartNode() {
            Name = "Event Start"; Category = "Flow";
            m_FlowOut = addOutputPin("Start", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "EventStartNode"; }
        uint32_t m_FlowOut = 0;
    };

    class EventUpdateNode : public VisualScriptNode {
    public:
        EventUpdateNode() {
            Name = "Event Update"; Category = "Flow";
            m_FlowOut = addOutputPin("Update", PinType::Flow);
            m_DeltaTime = addOutputPin("DeltaTime", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "EventUpdateNode"; }
        uint32_t m_FlowOut = 0;
        uint32_t m_DeltaTime = 0;
    };

    class IfElseNode : public VisualScriptNode {
    public:
        IfElseNode() {
            Name = "If / Else"; Category = "Flow";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Condition = addInputPin("Condition", PinType::Bool);
            m_TrueOut = addOutputPin("True", PinType::Flow);
            m_FalseOut = addOutputPin("False", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "IfElseNode"; }
        uint32_t m_FlowIn = 0, m_Condition = 0, m_TrueOut = 0, m_FalseOut = 0;
    };

    class ForLoopNode : public VisualScriptNode {
    public:
        ForLoopNode() {
            Name = "For Loop"; Category = "Flow";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Start = addInputPin("Start", PinType::Int, int(0));
            m_End = addInputPin("End", PinType::Int, int(10));
            m_LoopBody = addOutputPin("Body", PinType::Flow);
            m_Index = addOutputPin("Index", PinType::Int);
            m_Completed = addOutputPin("Completed", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "ForLoopNode"; }
        uint32_t m_FlowIn = 0, m_Start = 0, m_End = 0;
        uint32_t m_LoopBody = 0, m_Index = 0, m_Completed = 0;
    };

    class SequenceNode : public VisualScriptNode {
    public:
        SequenceNode() {
            Name = "Sequence"; Category = "Flow";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Out0 = addOutputPin("Then 0", PinType::Flow);
            m_Out1 = addOutputPin("Then 1", PinType::Flow);
            m_Out2 = addOutputPin("Then 2", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "SequenceNode"; }
        uint32_t m_FlowIn = 0, m_Out0 = 0, m_Out1 = 0, m_Out2 = 0;
    };

    // =====================================================================
    // Math Nodes
    // =====================================================================

    class AddNode : public VisualScriptNode {
    public:
        AddNode() {
            Name = "Add"; Category = "Math";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 0.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            float a = getPinValueAs<float>(m_A);
            float b = getPinValueAs<float>(m_B);
            setPinValue(m_Result, a + b);
        }
        std::string getNodeTypeName() const override { return "AddNode"; }
        uint32_t m_A = 0, m_B = 0, m_Result = 0;
    };

    class SubtractNode : public VisualScriptNode {
    public:
        SubtractNode() {
            Name = "Subtract"; Category = "Math";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 0.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, getPinValueAs<float>(m_A) - getPinValueAs<float>(m_B));
        }
        std::string getNodeTypeName() const override { return "SubtractNode"; }
        uint32_t m_A = 0, m_B = 0, m_Result = 0;
    };

    class MultiplyNode : public VisualScriptNode {
    public:
        MultiplyNode() {
            Name = "Multiply"; Category = "Math";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 1.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, getPinValueAs<float>(m_A) * getPinValueAs<float>(m_B));
        }
        std::string getNodeTypeName() const override { return "MultiplyNode"; }
        uint32_t m_A = 0, m_B = 0, m_Result = 0;
    };

    class DivideNode : public VisualScriptNode {
    public:
        DivideNode() {
            Name = "Divide"; Category = "Math";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 1.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            float b = getPinValueAs<float>(m_B);
            setPinValue(m_Result, b != 0.0f ? getPinValueAs<float>(m_A) / b : 0.0f);
        }
        std::string getNodeTypeName() const override { return "DivideNode"; }
        uint32_t m_A = 0, m_B = 0, m_Result = 0;
    };

    class SinNode : public VisualScriptNode {
    public:
        SinNode() {
            Name = "Sin"; Category = "Math";
            m_Angle = addInputPin("Radians", PinType::Float, 0.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, std::sin(getPinValueAs<float>(m_Angle)));
        }
        std::string getNodeTypeName() const override { return "SinNode"; }
        uint32_t m_Angle = 0, m_Result = 0;
    };

    class CosNode : public VisualScriptNode {
    public:
        CosNode() {
            Name = "Cos"; Category = "Math";
            m_Angle = addInputPin("Radians", PinType::Float, 0.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, std::cos(getPinValueAs<float>(m_Angle)));
        }
        std::string getNodeTypeName() const override { return "CosNode"; }
        uint32_t m_Angle = 0, m_Result = 0;
    };

    class LerpNode : public VisualScriptNode {
    public:
        LerpNode() {
            Name = "Lerp"; Category = "Math";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 1.0f);
            m_T = addInputPin("T", PinType::Float, 0.5f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            float a = getPinValueAs<float>(m_A);
            float b = getPinValueAs<float>(m_B);
            float t = getPinValueAs<float>(m_T);
            setPinValue(m_Result, a + (b - a) * t);
        }
        std::string getNodeTypeName() const override { return "LerpNode"; }
        uint32_t m_A = 0, m_B = 0, m_T = 0, m_Result = 0;
    };

    class ClampNode : public VisualScriptNode {
    public:
        ClampNode() {
            Name = "Clamp"; Category = "Math";
            m_Value = addInputPin("Value", PinType::Float, 0.0f);
            m_Min = addInputPin("Min", PinType::Float, 0.0f);
            m_Max = addInputPin("Max", PinType::Float, 1.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            float v = getPinValueAs<float>(m_Value);
            float mn = getPinValueAs<float>(m_Min);
            float mx = getPinValueAs<float>(m_Max);
            setPinValue(m_Result, std::max(mn, std::min(mx, v)));
        }
        std::string getNodeTypeName() const override { return "ClampNode"; }
        uint32_t m_Value = 0, m_Min = 0, m_Max = 0, m_Result = 0;
    };

    class RandomRangeNode : public VisualScriptNode {
    public:
        RandomRangeNode() {
            Name = "Random Range"; Category = "Math";
            m_Min = addInputPin("Min", PinType::Float, 0.0f);
            m_Max = addInputPin("Max", PinType::Float, 1.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "RandomRangeNode"; }
        uint32_t m_Min = 0, m_Max = 0, m_Result = 0;
    };

    class AbsNode : public VisualScriptNode {
    public:
        AbsNode() {
            Name = "Abs"; Category = "Math";
            m_Value = addInputPin("Value", PinType::Float, 0.0f);
            m_Result = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, std::abs(getPinValueAs<float>(m_Value)));
        }
        std::string getNodeTypeName() const override { return "AbsNode"; }
        uint32_t m_Value = 0, m_Result = 0;
    };

    // =====================================================================
    // Entity Nodes
    // =====================================================================

    class GetPositionNode : public VisualScriptNode {
    public:
        GetPositionNode() {
            Name = "Get Position"; Category = "Entity";
            m_Entity = addInputPin("Entity", PinType::Entity);
            m_Position = addOutputPin("Position", PinType::Vec3);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "GetPositionNode"; }
        uint32_t m_Entity = 0, m_Position = 0;
    };

    class SetPositionNode : public VisualScriptNode {
    public:
        SetPositionNode() {
            Name = "Set Position"; Category = "Entity";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Entity = addInputPin("Entity", PinType::Entity);
            m_Position = addInputPin("Position", PinType::Vec3);
            m_FlowOut = addOutputPin("Done", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "SetPositionNode"; }
        uint32_t m_FlowIn = 0, m_Entity = 0, m_Position = 0, m_FlowOut = 0;
    };

    class ApplyForceNode : public VisualScriptNode {
    public:
        ApplyForceNode() {
            Name = "Apply Force"; Category = "Entity";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Entity = addInputPin("Entity", PinType::Entity);
            m_Force = addInputPin("Force", PinType::Vec3);
            m_FlowOut = addOutputPin("Done", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "ApplyForceNode"; }
        uint32_t m_FlowIn = 0, m_Entity = 0, m_Force = 0, m_FlowOut = 0;
    };

    // =====================================================================
    // Input Nodes
    // =====================================================================

    class GetKeyNode : public VisualScriptNode {
    public:
        GetKeyNode() {
            Name = "Get Key"; Category = "Input";
            m_KeyName = addInputPin("Key", PinType::String, std::string("Space"));
            m_IsDown = addOutputPin("IsDown", PinType::Bool);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "GetKeyNode"; }
        uint32_t m_KeyName = 0, m_IsDown = 0;
    };

    class GetMousePositionNode : public VisualScriptNode {
    public:
        GetMousePositionNode() {
            Name = "Get Mouse Position"; Category = "Input";
            m_Position = addOutputPin("Position", PinType::Vec2);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "GetMousePositionNode"; }
        uint32_t m_Position = 0;
    };

    // =====================================================================
    // Debug Nodes
    // =====================================================================

    class PrintNode : public VisualScriptNode {
    public:
        PrintNode() {
            Name = "Print"; Category = "Debug";
            m_FlowIn = addInputPin("Exec", PinType::Flow);
            m_Message = addInputPin("Message", PinType::String, std::string("Hello"));
            m_FlowOut = addOutputPin("Done", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "PrintNode"; }
        uint32_t m_FlowIn = 0, m_Message = 0, m_FlowOut = 0;
    };

    class CompareNode : public VisualScriptNode {
    public:
        CompareNode() {
            Name = "Compare"; Category = "Logic";
            m_A = addInputPin("A", PinType::Float, 0.0f);
            m_B = addInputPin("B", PinType::Float, 0.0f);
            m_Greater = addOutputPin(">", PinType::Bool);
            m_Equal = addOutputPin("==", PinType::Bool);
            m_Less = addOutputPin("<", PinType::Bool);
        }
        void execute(VisualScriptVM& vm) override {
            float a = getPinValueAs<float>(m_A);
            float b = getPinValueAs<float>(m_B);
            setPinValue(m_Greater, a > b);
            setPinValue(m_Equal, std::abs(a - b) < 0.0001f);
            setPinValue(m_Less, a < b);
        }
        std::string getNodeTypeName() const override { return "CompareNode"; }
        uint32_t m_A = 0, m_B = 0, m_Greater = 0, m_Equal = 0, m_Less = 0;
    };

    class MakeVec3Node : public VisualScriptNode {
    public:
        MakeVec3Node() {
            Name = "Make Vec3"; Category = "Math";
            m_X = addInputPin("X", PinType::Float, 0.0f);
            m_Y = addInputPin("Y", PinType::Float, 0.0f);
            m_Z = addInputPin("Z", PinType::Float, 0.0f);
            m_Result = addOutputPin("Vector", PinType::Vec3);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, glm::vec3(
                getPinValueAs<float>(m_X),
                getPinValueAs<float>(m_Y),
                getPinValueAs<float>(m_Z)));
        }
        std::string getNodeTypeName() const override { return "MakeVec3Node"; }
        uint32_t m_X = 0, m_Y = 0, m_Z = 0, m_Result = 0;
    };

    class BreakVec3Node : public VisualScriptNode {
    public:
        BreakVec3Node() {
            Name = "Break Vec3"; Category = "Math";
            m_Vec = addInputPin("Vector", PinType::Vec3);
            m_X = addOutputPin("X", PinType::Float);
            m_Y = addOutputPin("Y", PinType::Float);
            m_Z = addOutputPin("Z", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override {
            glm::vec3 v = getPinValueAs<glm::vec3>(m_Vec);
            setPinValue(m_X, v.x);
            setPinValue(m_Y, v.y);
            setPinValue(m_Z, v.z);
        }
        std::string getNodeTypeName() const override { return "BreakVec3Node"; }
        uint32_t m_Vec = 0, m_X = 0, m_Y = 0, m_Z = 0;
    };

    class BoolNotNode : public VisualScriptNode {
    public:
        BoolNotNode() {
            Name = "Not"; Category = "Logic";
            m_Input = addInputPin("Value", PinType::Bool, false);
            m_Output = addOutputPin("Result", PinType::Bool);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Output, !getPinValueAs<bool>(m_Input));
        }
        std::string getNodeTypeName() const override { return "BoolNotNode"; }
        uint32_t m_Input = 0, m_Output = 0;
    };

    class BoolAndNode : public VisualScriptNode {
    public:
        BoolAndNode() {
            Name = "And"; Category = "Logic";
            m_A = addInputPin("A", PinType::Bool, false);
            m_B = addInputPin("B", PinType::Bool, false);
            m_Result = addOutputPin("Result", PinType::Bool);
        }
        void execute(VisualScriptVM& vm) override {
            setPinValue(m_Result, getPinValueAs<bool>(m_A) && getPinValueAs<bool>(m_B));
        }
        std::string getNodeTypeName() const override { return "BoolAndNode"; }
        uint32_t m_A = 0, m_B = 0, m_Result = 0;
    };

    // Register all built-in node types
    void registerBuiltinNodes();

} // namespace GameEngine
