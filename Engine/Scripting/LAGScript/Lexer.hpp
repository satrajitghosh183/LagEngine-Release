#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace GameEngine {
namespace LAGScript {

    enum class TokenType {
        // Literals
        Number, String, Identifier,
        True, False, Null,

        // Keywords
        Var, Const, Func, If, Else, Elif, For, While, In, Return,
        Class, Extends, Signal, Emit, Connect, Self, Super,
        And, Or, Not, Break, Continue,

        // Operators
        Plus, Minus, Star, Slash, Percent,
        Equals, EqualsEquals, NotEquals,
        Less, Greater, LessEquals, GreaterEquals,
        LParen, RParen, LBrace, RBrace, LBracket, RBracket,
        Comma, Dot, Colon, Arrow,

        // Layout
        Newline, Indent, Dedent, Eof,
    };

    struct Token {
        TokenType Type = TokenType::Eof;
        std::string Lexeme;
        double Number = 0.0;
        int Line = 1;
        int Column = 1;
    };

    class Lexer {
    public:
        explicit Lexer(std::string source);
        std::vector<Token> Tokenize();

        const std::vector<std::string>& GetErrors() const { return m_Errors; }
        bool HasErrors() const { return !m_Errors.empty(); }

    private:
        Token NextToken();
        Token ReadIdentifier();
        Token ReadNumber();
        Token ReadString(char quote);
        void  SkipWhitespace();
        void  HandleIndent(std::vector<Token>& out);
        char  Peek(int off = 0) const;
        char  Advance();
        bool  Match(char c);
        bool  IsAtEnd() const { return m_Pos >= m_Source.size(); }

        static TokenType KeywordFor(const std::string& word);

        std::string m_Source;
        size_t m_Pos = 0;
        int m_Line = 1;
        int m_Column = 1;
        std::vector<int> m_IndentStack; // 0 = baseline
        bool m_AtLineStart = true;
        std::vector<std::string> m_Errors;
    };

}}
