#include "Lexer.hpp"
#include <cctype>
#include <unordered_map>

namespace GameEngine {
namespace LAGScript {

    static const std::unordered_map<std::string, TokenType> kKeywords = {
        {"var", TokenType::Var}, {"const", TokenType::Const}, {"func", TokenType::Func},
        {"if", TokenType::If}, {"else", TokenType::Else}, {"elif", TokenType::Elif},
        {"for", TokenType::For}, {"while", TokenType::While}, {"in", TokenType::In},
        {"return", TokenType::Return}, {"class", TokenType::Class}, {"extends", TokenType::Extends},
        {"signal", TokenType::Signal}, {"emit", TokenType::Emit}, {"connect", TokenType::Connect},
        {"self", TokenType::Self}, {"super", TokenType::Super},
        {"and", TokenType::And}, {"or", TokenType::Or}, {"not", TokenType::Not},
        {"break", TokenType::Break}, {"continue", TokenType::Continue},
        {"true", TokenType::True}, {"false", TokenType::False}, {"null", TokenType::Null},
    };

    TokenType Lexer::KeywordFor(const std::string& word) {
        auto it = kKeywords.find(word);
        return it == kKeywords.end() ? TokenType::Identifier : it->second;
    }

    Lexer::Lexer(std::string source) : m_Source(std::move(source)) {
        m_IndentStack.push_back(0);
    }

    char Lexer::Peek(int off) const {
        return (m_Pos + off < m_Source.size()) ? m_Source[m_Pos + off] : '\0';
    }

    char Lexer::Advance() {
        if (IsAtEnd()) return '\0';
        char c = m_Source[m_Pos++];
        if (c == '\n') { m_Line++; m_Column = 1; }
        else m_Column++;
        return c;
    }

    bool Lexer::Match(char c) {
        if (Peek() == c) { Advance(); return true; }
        return false;
    }

    std::vector<Token> Lexer::Tokenize() {
        std::vector<Token> tokens;

        while (!IsAtEnd()) {
            if (m_AtLineStart) HandleIndent(tokens);
            if (IsAtEnd()) break;

            char c = Peek();
            if (c == '#') {
                while (!IsAtEnd() && Peek() != '\n') Advance();
                continue;
            }
            if (c == '\n') {
                Token t; t.Type = TokenType::Newline; t.Line = m_Line;
                tokens.push_back(t);
                Advance();
                m_AtLineStart = true;
                continue;
            }
            if (c == ' ' || c == '\t' || c == '\r') { Advance(); continue; }

            tokens.push_back(NextToken());
        }

        // Emit any remaining dedents
        while (m_IndentStack.size() > 1) {
            Token t; t.Type = TokenType::Dedent; t.Line = m_Line;
            tokens.push_back(t);
            m_IndentStack.pop_back();
        }
        Token eof; eof.Type = TokenType::Eof; eof.Line = m_Line;
        tokens.push_back(eof);
        return tokens;
    }

    void Lexer::HandleIndent(std::vector<Token>& out) {
        m_AtLineStart = false;
        int spaces = 0;
        while (Peek() == ' ') { Advance(); spaces++; }
        while (Peek() == '\t') { Advance(); spaces += 4; }

        // Blank line or comment-only — don't emit indent
        if (Peek() == '\n' || Peek() == '#' || IsAtEnd()) return;

        int current = m_IndentStack.back();
        if (spaces > current) {
            m_IndentStack.push_back(spaces);
            Token t; t.Type = TokenType::Indent; t.Line = m_Line;
            out.push_back(t);
        } else while (spaces < current) {
            m_IndentStack.pop_back();
            Token t; t.Type = TokenType::Dedent; t.Line = m_Line;
            out.push_back(t);
            current = m_IndentStack.back();
        }
    }

    Token Lexer::NextToken() {
        char c = Peek();
        int startLine = m_Line, startCol = m_Column;

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') return ReadIdentifier();
        if (std::isdigit(static_cast<unsigned char>(c))) return ReadNumber();
        if (c == '"' || c == '\'') return ReadString(Advance());

        Advance();

        auto mk = [&](TokenType t, const std::string& lex = "") {
            Token tok; tok.Type = t; tok.Lexeme = lex;
            tok.Line = startLine; tok.Column = startCol;
            return tok;
        };

        switch (c) {
            case '+': return mk(TokenType::Plus, "+");
            case '-':
                if (Match('>')) return mk(TokenType::Arrow, "->");
                return mk(TokenType::Minus, "-");
            case '*': return mk(TokenType::Star, "*");
            case '/': return mk(TokenType::Slash, "/");
            case '%': return mk(TokenType::Percent, "%");
            case '=':
                if (Match('=')) return mk(TokenType::EqualsEquals, "==");
                return mk(TokenType::Equals, "=");
            case '!':
                if (Match('=')) return mk(TokenType::NotEquals, "!=");
                break;
            case '<':
                if (Match('=')) return mk(TokenType::LessEquals, "<=");
                return mk(TokenType::Less, "<");
            case '>':
                if (Match('=')) return mk(TokenType::GreaterEquals, ">=");
                return mk(TokenType::Greater, ">");
            case '(': return mk(TokenType::LParen, "(");
            case ')': return mk(TokenType::RParen, ")");
            case '{': return mk(TokenType::LBrace, "{");
            case '}': return mk(TokenType::RBrace, "}");
            case '[': return mk(TokenType::LBracket, "[");
            case ']': return mk(TokenType::RBracket, "]");
            case ',': return mk(TokenType::Comma, ",");
            case '.': return mk(TokenType::Dot, ".");
            case ':': return mk(TokenType::Colon, ":");
        }
        m_Errors.push_back("Unexpected character '" + std::string(1, c) + "' at line " + std::to_string(startLine));
        return mk(TokenType::Eof, "");
    }

    Token Lexer::ReadIdentifier() {
        int startLine = m_Line, startCol = m_Column;
        std::string word;
        while (std::isalnum(static_cast<unsigned char>(Peek())) || Peek() == '_') {
            word += Advance();
        }
        Token t;
        t.Type = KeywordFor(word);
        t.Lexeme = word;
        t.Line = startLine;
        t.Column = startCol;
        return t;
    }

    Token Lexer::ReadNumber() {
        int startLine = m_Line, startCol = m_Column;
        std::string num;
        while (std::isdigit(static_cast<unsigned char>(Peek()))) num += Advance();
        if (Peek() == '.' && std::isdigit(static_cast<unsigned char>(Peek(1)))) {
            num += Advance();
            while (std::isdigit(static_cast<unsigned char>(Peek()))) num += Advance();
        }
        Token t;
        t.Type = TokenType::Number;
        t.Lexeme = num;
        t.Number = std::stod(num);
        t.Line = startLine;
        t.Column = startCol;
        return t;
    }

    Token Lexer::ReadString(char quote) {
        int startLine = m_Line, startCol = m_Column;
        std::string str;
        while (!IsAtEnd() && Peek() != quote) {
            char c = Advance();
            if (c == '\\' && !IsAtEnd()) {
                char esc = Advance();
                switch (esc) {
                    case 'n': str += '\n'; break;
                    case 't': str += '\t'; break;
                    case 'r': str += '\r'; break;
                    case '\\': str += '\\'; break;
                    case '"': str += '"'; break;
                    case '\'': str += '\''; break;
                    default: str += esc;
                }
            } else str += c;
        }
        if (!IsAtEnd()) Advance(); // closing quote
        Token t;
        t.Type = TokenType::String;
        t.Lexeme = str;
        t.Line = startLine;
        t.Column = startCol;
        return t;
    }

}}
