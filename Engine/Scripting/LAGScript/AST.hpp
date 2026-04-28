#pragma once

#include "../../Core/Base.hpp"
#include "Value.hpp"
#include <memory>
#include <string>
#include <vector>

namespace GameEngine {
namespace LAGScript {

    struct ASTNode;
    using NodePtr = Ref<ASTNode>;

    enum class NodeKind {
        Program, FuncDecl, ClassDecl, VarDecl, ConstDecl,
        IfStmt, WhileStmt, ForStmt, ReturnStmt, BreakStmt, ContinueStmt,
        ExprStmt, SignalDecl, EmitStmt, ConnectStmt,

        // Expressions
        NumberLit, StringLit, BoolLit, NullLit, Identifier,
        BinaryOp, UnaryOp, Call, MemberAccess, IndexAccess,
        ArrayLit, DictLit, Assign, Self, Super,
    };

    struct ASTNode {
        NodeKind Kind;
        int Line = 1;

        // Data union via variants
        std::string Text;
        double Number = 0.0;
        bool BoolValue = false;
        std::string Op;

        std::vector<NodePtr> Children;
        // For FuncDecl: name + params + body
        // For VarDecl: name + typeAnnotation + initializer
        // For IfStmt: cond + thenBody + elifClauses... + elseBody
        // For Call: target + args
        // For MemberAccess: object + ".name" via Text
        // For ClassDecl: name (Text) + extendsName + body
    };

    inline NodePtr MakeNode(NodeKind k, int line = 0) {
        auto n = CreateRef<ASTNode>();
        n->Kind = k;
        n->Line = line;
        return n;
    }

}}
