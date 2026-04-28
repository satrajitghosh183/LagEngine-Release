#include "Parser.hpp"

namespace GameEngine {
namespace LAGScript {

    Parser::Parser(std::vector<Token> tokens) : m_Tokens(std::move(tokens)) {}

    const Token& Parser::Peek(int off) const {
        size_t idx = m_Pos + off;
        if (idx >= m_Tokens.size()) return m_Tokens.back();
        return m_Tokens[idx];
    }

    const Token& Parser::Advance() {
        if (m_Pos < m_Tokens.size()) return m_Tokens[m_Pos++];
        return m_Tokens.back();
    }

    void Parser::Expect(TokenType t, const std::string& msg) {
        if (Check(t)) { Advance(); return; }
        Error(msg + " (got '" + Peek().Lexeme + "' at line " + std::to_string(Peek().Line) + ")");
    }

    void Parser::Error(const std::string& msg) {
        m_Errors.push_back(msg);
        // Panic-mode recovery — skip to next newline/dedent
        while (!Check(TokenType::Eof) && !Check(TokenType::Newline) && !Check(TokenType::Dedent)) {
            Advance();
        }
    }

    void Parser::SkipNewlines() {
        while (Match(TokenType::Newline)) {}
    }

    NodePtr Parser::Parse() {
        auto prog = MakeNode(NodeKind::Program);
        SkipNewlines();
        while (!Check(TokenType::Eof)) {
            auto stmt = ParseStatement();
            if (stmt) prog->Children.push_back(stmt);
            SkipNewlines();
        }
        return prog;
    }

    NodePtr Parser::ParseStatement() {
        switch (Peek().Type) {
            case TokenType::Func:     return ParseFuncDecl();
            case TokenType::Class:    return ParseClassDecl();
            case TokenType::Var:      Advance(); return ParseVarDecl(false);
            case TokenType::Const:    Advance(); return ParseVarDecl(true);
            case TokenType::If:       return ParseIfStmt();
            case TokenType::While:    return ParseWhileStmt();
            case TokenType::For:      return ParseForStmt();
            case TokenType::Return:   return ParseReturnStmt();
            case TokenType::Signal:   return ParseSignalDecl();
            case TokenType::Break: {
                int line = Advance().Line;
                return MakeNode(NodeKind::BreakStmt, line);
            }
            case TokenType::Continue: {
                int line = Advance().Line;
                return MakeNode(NodeKind::ContinueStmt, line);
            }
            case TokenType::Emit: {
                int line = Advance().Line;
                auto node = MakeNode(NodeKind::EmitStmt, line);
                node->Text = Advance().Lexeme; // signal name
                if (Match(TokenType::LParen)) {
                    while (!Check(TokenType::RParen) && !Check(TokenType::Eof)) {
                        node->Children.push_back(ParseExpression());
                        if (!Match(TokenType::Comma)) break;
                    }
                    Expect(TokenType::RParen, "expected ')' after emit arguments");
                }
                return node;
            }
            default:
                return ParseExprStmt();
        }
    }

    NodePtr Parser::ParseFuncDecl() {
        int line = Advance().Line; // consume 'func'
        auto node = MakeNode(NodeKind::FuncDecl, line);
        node->Text = Advance().Lexeme; // function name
        Expect(TokenType::LParen, "expected '(' after function name");
        while (!Check(TokenType::RParen) && !Check(TokenType::Eof)) {
            auto param = MakeNode(NodeKind::Identifier);
            param->Text = Advance().Lexeme;
            // optional ": Type"
            if (Match(TokenType::Colon)) Advance(); // skip type
            node->Children.push_back(param);
            if (!Match(TokenType::Comma)) break;
        }
        Expect(TokenType::RParen, "expected ')' after parameters");
        // Optional return type
        if (Match(TokenType::Arrow)) Advance();
        Expect(TokenType::Colon, "expected ':' before function body");
        node->Children.push_back(ParseBlock());
        return node;
    }

    NodePtr Parser::ParseClassDecl() {
        int line = Advance().Line; // 'class'
        auto node = MakeNode(NodeKind::ClassDecl, line);
        node->Text = Advance().Lexeme; // class name
        if (Match(TokenType::Extends)) {
            node->Op = Advance().Lexeme;
        }
        Expect(TokenType::Colon, "expected ':' after class declaration");
        node->Children.push_back(ParseBlock());
        return node;
    }

    NodePtr Parser::ParseVarDecl(bool isConst) {
        auto node = MakeNode(isConst ? NodeKind::ConstDecl : NodeKind::VarDecl, Peek().Line);
        node->Text = Advance().Lexeme; // name
        // Optional type annotation
        if (Match(TokenType::Colon)) Advance();
        if (Match(TokenType::Equals)) {
            node->Children.push_back(ParseExpression());
        }
        return node;
    }

    NodePtr Parser::ParseIfStmt() {
        int line = Advance().Line; // 'if'
        auto node = MakeNode(NodeKind::IfStmt, line);
        node->Children.push_back(ParseExpression()); // cond
        Expect(TokenType::Colon, "expected ':' after if condition");
        node->Children.push_back(ParseBlock()); // then

        while (Match(TokenType::Elif)) {
            auto cond = ParseExpression();
            Expect(TokenType::Colon, "expected ':' after elif");
            auto body = ParseBlock();
            auto pair = MakeNode(NodeKind::IfStmt); // encoded as nested if
            pair->Children.push_back(cond);
            pair->Children.push_back(body);
            node->Children.push_back(pair);
        }

        if (Match(TokenType::Else)) {
            Expect(TokenType::Colon, "expected ':' after else");
            node->Children.push_back(ParseBlock()); // else body
        }
        return node;
    }

    NodePtr Parser::ParseWhileStmt() {
        int line = Advance().Line;
        auto node = MakeNode(NodeKind::WhileStmt, line);
        node->Children.push_back(ParseExpression());
        Expect(TokenType::Colon, "expected ':' after while");
        node->Children.push_back(ParseBlock());
        return node;
    }

    NodePtr Parser::ParseForStmt() {
        int line = Advance().Line;
        auto node = MakeNode(NodeKind::ForStmt, line);
        node->Text = Advance().Lexeme; // loop var
        Expect(TokenType::In, "expected 'in' after for variable");
        node->Children.push_back(ParseExpression()); // iterable
        Expect(TokenType::Colon, "expected ':' after for");
        node->Children.push_back(ParseBlock());
        return node;
    }

    NodePtr Parser::ParseReturnStmt() {
        int line = Advance().Line;
        auto node = MakeNode(NodeKind::ReturnStmt, line);
        if (!Check(TokenType::Newline) && !Check(TokenType::Eof) && !Check(TokenType::Dedent)) {
            node->Children.push_back(ParseExpression());
        }
        return node;
    }

    NodePtr Parser::ParseSignalDecl() {
        int line = Advance().Line;
        auto node = MakeNode(NodeKind::SignalDecl, line);
        node->Text = Advance().Lexeme;
        // Optional param list
        if (Match(TokenType::LParen)) {
            while (!Check(TokenType::RParen) && !Check(TokenType::Eof)) {
                Advance();
                if (Match(TokenType::Colon)) Advance();
                if (!Match(TokenType::Comma)) break;
            }
            Expect(TokenType::RParen, "expected ')' after signal params");
        }
        return node;
    }

    NodePtr Parser::ParseBlock() {
        auto node = MakeNode(NodeKind::Program);
        SkipNewlines();
        Expect(TokenType::Indent, "expected indented block");
        while (!Check(TokenType::Dedent) && !Check(TokenType::Eof)) {
            auto stmt = ParseStatement();
            if (stmt) node->Children.push_back(stmt);
            SkipNewlines();
        }
        Match(TokenType::Dedent);
        return node;
    }

    NodePtr Parser::ParseExprStmt() {
        auto node = MakeNode(NodeKind::ExprStmt, Peek().Line);
        node->Children.push_back(ParseExpression());
        return node;
    }

    NodePtr Parser::ParseExpression() { return ParseAssignment(); }

    NodePtr Parser::ParseAssignment() {
        auto left = ParseLogicOr();
        if (Check(TokenType::Equals)) {
            Advance();
            auto value = ParseAssignment();
            auto n = MakeNode(NodeKind::Assign);
            n->Children.push_back(left);
            n->Children.push_back(value);
            return n;
        }
        return left;
    }

    NodePtr Parser::ParseLogicOr() {
        auto left = ParseLogicAnd();
        while (Match(TokenType::Or)) {
            auto right = ParseLogicAnd();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = "or";
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseLogicAnd() {
        auto left = ParseEquality();
        while (Match(TokenType::And)) {
            auto right = ParseEquality();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = "and";
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseEquality() {
        auto left = ParseComparison();
        while (Check(TokenType::EqualsEquals) || Check(TokenType::NotEquals)) {
            std::string op = Advance().Lexeme;
            auto right = ParseComparison();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = op;
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseComparison() {
        auto left = ParseTerm();
        while (Check(TokenType::Less) || Check(TokenType::Greater) ||
               Check(TokenType::LessEquals) || Check(TokenType::GreaterEquals)) {
            std::string op = Advance().Lexeme;
            auto right = ParseTerm();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = op;
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseTerm() {
        auto left = ParseFactor();
        while (Check(TokenType::Plus) || Check(TokenType::Minus)) {
            std::string op = Advance().Lexeme;
            auto right = ParseFactor();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = op;
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseFactor() {
        auto left = ParseUnary();
        while (Check(TokenType::Star) || Check(TokenType::Slash) || Check(TokenType::Percent)) {
            std::string op = Advance().Lexeme;
            auto right = ParseUnary();
            auto n = MakeNode(NodeKind::BinaryOp);
            n->Op = op;
            n->Children = { left, right };
            left = n;
        }
        return left;
    }

    NodePtr Parser::ParseUnary() {
        if (Check(TokenType::Minus) || Check(TokenType::Not)) {
            std::string op = Advance().Lexeme;
            auto operand = ParseUnary();
            auto n = MakeNode(NodeKind::UnaryOp);
            n->Op = op;
            n->Children = { operand };
            return n;
        }
        return ParseCall();
    }

    NodePtr Parser::ParseCall() {
        auto expr = ParsePrimary();
        while (true) {
            if (Match(TokenType::LParen)) {
                auto n = MakeNode(NodeKind::Call);
                n->Children.push_back(expr);
                while (!Check(TokenType::RParen) && !Check(TokenType::Eof)) {
                    n->Children.push_back(ParseExpression());
                    if (!Match(TokenType::Comma)) break;
                }
                Expect(TokenType::RParen, "expected ')'");
                expr = n;
            } else if (Match(TokenType::Dot)) {
                auto n = MakeNode(NodeKind::MemberAccess);
                n->Text = Advance().Lexeme;
                n->Children.push_back(expr);
                expr = n;
            } else if (Match(TokenType::LBracket)) {
                auto n = MakeNode(NodeKind::IndexAccess);
                n->Children.push_back(expr);
                n->Children.push_back(ParseExpression());
                Expect(TokenType::RBracket, "expected ']'");
                expr = n;
            } else break;
        }
        return expr;
    }

    NodePtr Parser::ParsePrimary() {
        const Token& t = Peek();
        if (t.Type == TokenType::Number) {
            Advance();
            auto n = MakeNode(NodeKind::NumberLit, t.Line);
            n->Number = t.Number;
            return n;
        }
        if (t.Type == TokenType::String) {
            Advance();
            auto n = MakeNode(NodeKind::StringLit, t.Line);
            n->Text = t.Lexeme;
            return n;
        }
        if (t.Type == TokenType::True)  { Advance(); auto n = MakeNode(NodeKind::BoolLit); n->BoolValue = true; return n; }
        if (t.Type == TokenType::False) { Advance(); auto n = MakeNode(NodeKind::BoolLit); n->BoolValue = false; return n; }
        if (t.Type == TokenType::Null)  { Advance(); return MakeNode(NodeKind::NullLit); }
        if (t.Type == TokenType::Self)  { Advance(); return MakeNode(NodeKind::Self); }
        if (t.Type == TokenType::Super) { Advance(); return MakeNode(NodeKind::Super); }
        if (t.Type == TokenType::Identifier) {
            Advance();
            auto n = MakeNode(NodeKind::Identifier);
            n->Text = t.Lexeme;
            return n;
        }
        if (Match(TokenType::LParen)) {
            auto e = ParseExpression();
            Expect(TokenType::RParen, "expected ')'");
            return e;
        }
        if (Match(TokenType::LBracket)) {
            auto n = MakeNode(NodeKind::ArrayLit);
            while (!Check(TokenType::RBracket) && !Check(TokenType::Eof)) {
                n->Children.push_back(ParseExpression());
                if (!Match(TokenType::Comma)) break;
            }
            Expect(TokenType::RBracket, "expected ']'");
            return n;
        }
        if (Match(TokenType::LBrace)) {
            auto n = MakeNode(NodeKind::DictLit);
            while (!Check(TokenType::RBrace) && !Check(TokenType::Eof)) {
                auto key = ParseExpression();
                Expect(TokenType::Colon, "expected ':' in dict");
                auto val = ParseExpression();
                n->Children.push_back(key);
                n->Children.push_back(val);
                if (!Match(TokenType::Comma)) break;
            }
            Expect(TokenType::RBrace, "expected '}'");
            return n;
        }
        Error("unexpected token '" + t.Lexeme + "'");
        Advance();
        return MakeNode(NodeKind::NullLit);
    }

}}
