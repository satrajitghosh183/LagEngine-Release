#pragma once

#include "VisualScriptNode.hpp"
#include "VisualScriptVM.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <cstdlib>
#include <random>
#include <string>

namespace GameEngine {

    // Forward decl — registerExtendedNodes is defined in VisualScriptNodesExt.cpp
    class VisualScriptNodeRegistry;
    void registerExtendedNodes(VisualScriptNodeRegistry& reg);

    // =====================================================================
    // Math (extended)
    // =====================================================================

    class ModNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        ModNode() {
            Name = "Modulo"; Category = "Math";
            PinA = addInputPin("A", PinType::Float, 0.0f);
            PinB = addInputPin("B", PinType::Float, 1.0f);
            PinOut = addOutputPin("Result", PinType::Float, 0.0f);
        }
        void execute(VisualScriptVM&) override {
            float a = getPinValueAs<float>(PinA), b = getPinValueAs<float>(PinB);
            setPinValue(PinOut, b == 0.0f ? 0.0f : std::fmod(a, b));
        }
        std::string getNodeTypeName() const override { return "Modulo"; }
    };

    class NegateNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        NegateNode() {
            Name = "Negate"; Category = "Math";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, -getPinValueAs<float>(PinIn));
        }
        std::string getNodeTypeName() const override { return "Negate"; }
    };

    class MinNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        MinNode() {
            Name = "Min"; Category = "Math";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::min(getPinValueAs<float>(PinA), getPinValueAs<float>(PinB)));
        }
        std::string getNodeTypeName() const override { return "Min"; }
    };

    class MaxNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        MaxNode() {
            Name = "Max"; Category = "Math";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::max(getPinValueAs<float>(PinA), getPinValueAs<float>(PinB)));
        }
        std::string getNodeTypeName() const override { return "Max"; }
    };

    class TanNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        TanNode() {
            Name = "Tan"; Category = "Math";
            PinIn = addInputPin("Angle", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::tan(getPinValueAs<float>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "Tan"; }
    };

    class SqrtNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        SqrtNode() {
            Name = "Sqrt"; Category = "Math";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            float v = getPinValueAs<float>(PinIn);
            setPinValue(PinOut, v < 0.0f ? 0.0f : std::sqrt(v));
        }
        std::string getNodeTypeName() const override { return "Sqrt"; }
    };

    class PowNode : public VisualScriptNode {
    public:
        uint32_t PinBase, PinExp, PinOut;
        PowNode() {
            Name = "Pow"; Category = "Math";
            PinBase = addInputPin("Base", PinType::Float, 1.0f);
            PinExp = addInputPin("Exponent", PinType::Float, 2.0f);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::pow(getPinValueAs<float>(PinBase), getPinValueAs<float>(PinExp)));
        }
        std::string getNodeTypeName() const override { return "Pow"; }
    };

    class FloorNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        FloorNode() {
            Name = "Floor"; Category = "Math";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::floor(getPinValueAs<float>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "Floor"; }
    };

    class CeilNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        CeilNode() {
            Name = "Ceil"; Category = "Math";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::ceil(getPinValueAs<float>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "Ceil"; }
    };

    class RoundNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        RoundNode() {
            Name = "Round"; Category = "Math";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::round(getPinValueAs<float>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "Round"; }
    };

    // =====================================================================
    // Vector math
    // =====================================================================

    class Vec3AddNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        Vec3AddNode() {
            Name = "Vec3 Add"; Category = "Vector";
            PinA = addInputPin("A", PinType::Vec3);
            PinB = addInputPin("B", PinType::Vec3);
            PinOut = addOutputPin("Result", PinType::Vec3);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<glm::vec3>(PinA) + getPinValueAs<glm::vec3>(PinB));
        }
        std::string getNodeTypeName() const override { return "Vec3Add"; }
    };

    class Vec3SubNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        Vec3SubNode() {
            Name = "Vec3 Sub"; Category = "Vector";
            PinA = addInputPin("A", PinType::Vec3);
            PinB = addInputPin("B", PinType::Vec3);
            PinOut = addOutputPin("Result", PinType::Vec3);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<glm::vec3>(PinA) - getPinValueAs<glm::vec3>(PinB));
        }
        std::string getNodeTypeName() const override { return "Vec3Sub"; }
    };

    class Vec3ScaleNode : public VisualScriptNode {
    public:
        uint32_t PinV, PinS, PinOut;
        Vec3ScaleNode() {
            Name = "Vec3 Scale"; Category = "Vector";
            PinV = addInputPin("Vector", PinType::Vec3);
            PinS = addInputPin("Scale", PinType::Float, 1.0f);
            PinOut = addOutputPin("Result", PinType::Vec3);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<glm::vec3>(PinV) * getPinValueAs<float>(PinS));
        }
        std::string getNodeTypeName() const override { return "Vec3Scale"; }
    };

    class Vec3NormalizeNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        Vec3NormalizeNode() {
            Name = "Vec3 Normalize"; Category = "Vector";
            PinIn = addInputPin("Vector", PinType::Vec3);
            PinOut = addOutputPin("Result", PinType::Vec3);
        }
        void execute(VisualScriptVM&) override {
            glm::vec3 v = getPinValueAs<glm::vec3>(PinIn);
            float len = glm::length(v);
            setPinValue(PinOut, len > 1e-6f ? v / len : glm::vec3(0.0f));
        }
        std::string getNodeTypeName() const override { return "Vec3Normalize"; }
    };

    class Vec3DotNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        Vec3DotNode() {
            Name = "Vec3 Dot"; Category = "Vector";
            PinA = addInputPin("A", PinType::Vec3);
            PinB = addInputPin("B", PinType::Vec3);
            PinOut = addOutputPin("Result", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, glm::dot(getPinValueAs<glm::vec3>(PinA), getPinValueAs<glm::vec3>(PinB)));
        }
        std::string getNodeTypeName() const override { return "Vec3Dot"; }
    };

    class Vec3CrossNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        Vec3CrossNode() {
            Name = "Vec3 Cross"; Category = "Vector";
            PinA = addInputPin("A", PinType::Vec3);
            PinB = addInputPin("B", PinType::Vec3);
            PinOut = addOutputPin("Result", PinType::Vec3);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, glm::cross(getPinValueAs<glm::vec3>(PinA), getPinValueAs<glm::vec3>(PinB)));
        }
        std::string getNodeTypeName() const override { return "Vec3Cross"; }
    };

    class Vec3LengthNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        Vec3LengthNode() {
            Name = "Vec3 Length"; Category = "Vector";
            PinIn = addInputPin("Vector", PinType::Vec3);
            PinOut = addOutputPin("Length", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, glm::length(getPinValueAs<glm::vec3>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "Vec3Length"; }
    };

    class Vec3DistanceNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        Vec3DistanceNode() {
            Name = "Vec3 Distance"; Category = "Vector";
            PinA = addInputPin("A", PinType::Vec3);
            PinB = addInputPin("B", PinType::Vec3);
            PinOut = addOutputPin("Distance", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, glm::distance(getPinValueAs<glm::vec3>(PinA), getPinValueAs<glm::vec3>(PinB)));
        }
        std::string getNodeTypeName() const override { return "Vec3Distance"; }
    };

    // =====================================================================
    // Logic / comparison
    // =====================================================================

    class OrNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        OrNode() {
            Name = "Or"; Category = "Logic";
            PinA = addInputPin("A", PinType::Bool);
            PinB = addInputPin("B", PinType::Bool);
            PinOut = addOutputPin("Result", PinType::Bool);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<bool>(PinA) || getPinValueAs<bool>(PinB));
        }
        std::string getNodeTypeName() const override { return "Or"; }
    };

    class EqualsNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        EqualsNode() {
            Name = "Equals"; Category = "Logic";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("Equal", PinType::Bool);
        }
        void execute(VisualScriptVM&) override {
            float a = getPinValueAs<float>(PinA), b = getPinValueAs<float>(PinB);
            setPinValue(PinOut, std::abs(a - b) < 1e-6f);
        }
        std::string getNodeTypeName() const override { return "Equals"; }
    };

    class NotEqualsNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        NotEqualsNode() {
            Name = "NotEquals"; Category = "Logic";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("NotEqual", PinType::Bool);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::abs(getPinValueAs<float>(PinA) - getPinValueAs<float>(PinB)) >= 1e-6f);
        }
        std::string getNodeTypeName() const override { return "NotEquals"; }
    };

    class LessThanNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        LessThanNode() {
            Name = "Less"; Category = "Logic";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("Less", PinType::Bool);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<float>(PinA) < getPinValueAs<float>(PinB));
        }
        std::string getNodeTypeName() const override { return "LessThan"; }
    };

    class GreaterThanNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        GreaterThanNode() {
            Name = "Greater"; Category = "Logic";
            PinA = addInputPin("A", PinType::Float);
            PinB = addInputPin("B", PinType::Float);
            PinOut = addOutputPin("Greater", PinType::Bool);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<float>(PinA) > getPinValueAs<float>(PinB));
        }
        std::string getNodeTypeName() const override { return "GreaterThan"; }
    };

    // =====================================================================
    // Time
    // =====================================================================

    class DeltaTimeNode : public VisualScriptNode {
    public:
        uint32_t PinOut;
        DeltaTimeNode() {
            Name = "DeltaTime"; Category = "Time";
            PinOut = addOutputPin("Delta", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "DeltaTime"; }
    };

    class GameTimeNode : public VisualScriptNode {
    public:
        uint32_t PinOut;
        GameTimeNode() {
            Name = "GameTime"; Category = "Time";
            PinOut = addOutputPin("Time", PinType::Float);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "GameTime"; }
    };

    // =====================================================================
    // String
    // =====================================================================

    class StringConcatNode : public VisualScriptNode {
    public:
        uint32_t PinA, PinB, PinOut;
        StringConcatNode() {
            Name = "Concat"; Category = "String";
            PinA = addInputPin("A", PinType::String);
            PinB = addInputPin("B", PinType::String);
            PinOut = addOutputPin("Result", PinType::String);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, getPinValueAs<std::string>(PinA) + getPinValueAs<std::string>(PinB));
        }
        std::string getNodeTypeName() const override { return "StringConcat"; }
    };

    class StringLengthNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        StringLengthNode() {
            Name = "Length"; Category = "String";
            PinIn = addInputPin("String", PinType::String);
            PinOut = addOutputPin("Length", PinType::Int);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, static_cast<int>(getPinValueAs<std::string>(PinIn).size()));
        }
        std::string getNodeTypeName() const override { return "StringLength"; }
    };

    class ToStringNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        ToStringNode() {
            Name = "ToString"; Category = "String";
            PinIn = addInputPin("Value", PinType::Float);
            PinOut = addOutputPin("String", PinType::String);
        }
        void execute(VisualScriptVM&) override {
            setPinValue(PinOut, std::to_string(getPinValueAs<float>(PinIn)));
        }
        std::string getNodeTypeName() const override { return "ToString"; }
    };

    class ParseFloatNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinOut;
        ParseFloatNode() {
            Name = "ParseFloat"; Category = "String";
            PinIn = addInputPin("String", PinType::String);
            PinOut = addOutputPin("Value", PinType::Float);
        }
        void execute(VisualScriptVM&) override {
            try {
                setPinValue(PinOut, std::stof(getPinValueAs<std::string>(PinIn)));
            } catch (...) {
                setPinValue(PinOut, 0.0f);
            }
        }
        std::string getNodeTypeName() const override { return "ParseFloat"; }
    };

    // =====================================================================
    // Flow control (additional)
    // =====================================================================

    class WhileLoopNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinCond, PinBody, PinDone;
        WhileLoopNode() {
            Name = "While"; Category = "Flow";
            PinIn = addInputPin("In", PinType::Flow);
            PinCond = addInputPin("Condition", PinType::Bool);
            PinBody = addOutputPin("Body", PinType::Flow);
            PinDone = addOutputPin("Done", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "While"; }
    };

    class SwitchNode : public VisualScriptNode {
    public:
        uint32_t PinIn, PinValue, PinCase1, PinCase2, PinCase3, PinDefault;
        SwitchNode() {
            Name = "Switch"; Category = "Flow";
            PinIn = addInputPin("In", PinType::Flow);
            PinValue = addInputPin("Value", PinType::Int);
            PinCase1 = addOutputPin("Case 1", PinType::Flow);
            PinCase2 = addOutputPin("Case 2", PinType::Flow);
            PinCase3 = addOutputPin("Case 3", PinType::Flow);
            PinDefault = addOutputPin("Default", PinType::Flow);
        }
        void execute(VisualScriptVM& vm) override;
        std::string getNodeTypeName() const override { return "Switch"; }
    };

} // namespace GameEngine
