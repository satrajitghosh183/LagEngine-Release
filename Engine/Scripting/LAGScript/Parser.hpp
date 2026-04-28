#pragma once

#include "Lexer.hpp"
#include "AST.hpp"
#include <string>
#include <vector>

namespace GameEngine {
namespace LAGScript {

    class Parser {
    public:
        explicit Parser(std::vector<Token> tokens);

        NodePtr Parse();
        const std::vector<std::string>& GetErrors() const { return m_Errors; }
        bool HasErrors() const { return !m_Errors.empty(); }

    private:
        const Token& Peek(int off = 0) const;
        const Token& Advance();
        bool Check(TokenType t) const { return Peek().Type == t; }
        bool Match(TokenType t) { if (Check(t)) { Advance(); return true; } return false; }
        void Expect(TokenType t, const std::string& msg);
        void Error(const std::string& msg);
        void SkipNewlines();

        NodePtr ParseStatement();
        NodePtr ParseFuncDecl();
        NodePtr ParseClassDecl();
        NodePtr ParseVarDecl(bool isConst);
        NodePtr ParseIfStmt();
        NodePtr ParseWhileStmt();
        NodePtr ParseForStmt();
        NodePtr ParseReturnStmt();
        NodePtr ParseSignalDecl();
        NodePtr ParseBlock();
        NodePtr ParseExprStmt();

        NodePtr ParseExpression();
        NodePtr ParseAssignment();
        NodePtr ParseLogicOr();
        NodePtr ParseLogicAnd();
        NodePtr ParseEquality();
        NodePtr ParseComparison();
        NodePtr ParseTerm();
        NodePtr ParseFactor();
        NodePtr ParseUnary();
        NodePtr ParseCall();
        NodePtr ParsePrimary();

        std::vector<Token> m_Tokens;
        size_t m_Pos = 0;
        std::vector<std::string> m_Errors;
    };

}}
