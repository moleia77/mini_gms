#pragma once

// mini_gms parser module
// Declares the lexer and the parser
// Internal module; external code should not use it directly

#include <string>
#include <vector>
#include <unordered_map>
#include "mini_gms_value.hpp"

namespace mol::mgms
{

    // Token types
    enum class Tok
    {
        LBracket,  // [
        RBracket,  // ]
        LBrace,    // {
        RBrace,    // }
        Colon,     // :
        Tilde,     // ~
        Ident,     // Identifier, including macro names starting with &
        StringRaw, // Double-quoted raw string
        StringEsc, // Single-quoted escaped string
        Number,    // Number
        True,      // true
        False,     // false
        Null,      // null
        End        // End of input
    };

    // A single token, with type, source text, and line number
    struct Token
    {
        Tok type;
        int line;
        std::string text;
    };

    // Lexer; converts source text into a list of tokens
    class Lexer
    {
    public:
        // Stores a reference to the source; the source must outlive the Lexer
        explicit Lexer(const std::string &src);

        // Scans all tokens, appending Tok::End at the end
        // Throws on illegal characters or unterminated strings
        std::vector<Token> tokenize();

    private:
        // Skips whitespace and # line comments
        void skipSpace();

        // Returns true if c can start an identifier, including &
        static bool isIdentStart(char c);

        // Returns true if c can continue an identifier
        static bool isIdentChar(char c);

        // Reads a double-quoted raw string; ends at ", no escapes
        Token readStringRaw();

        // Reads a single-quoted escaped string; ends at ', may contain escapes
        Token readStringEsc();

        // Reads a number; supports an optional minus sign and decimal point
        Token readNumber();

        // Reads an identifier and maps keywords to dedicated token types
        Token readIdent();

        const std::string &src_;
        size_t pos_ = 0;
        int line_ = 1;
    };

    // Parser; converts a token list into an AST
    class Parser
    {
    public:
        // Receives the token list
        explicit Parser(std::vector<Token> toks);

        // Parses all top-level declarations
        Stu parseTopLevel();

    private:
        // Peeks at the current token without consuming it
        const Token &peek() const;

        // Consumes and returns the current token
        const Token &next();

        // Returns true if the current token is of the given type
        bool check(Tok t) const;

        // Requires the current token to be of the given type, otherwise throws
        const Token &expect(Tok t, const std::string &msg);

        // Parses one top-level declaration
        StuMember parseTopDecl();

        // Parses a batch of declarations without a tag
        void parseBatchDecl(Stu &out);

        // Parses a stu body
        ValuePtr parseStuBody();

        // Parses one member inside a stu
        StuMember parseMember();

        // Parses a vec body, including ~ range expansion
        ValuePtr parseVecBody();

        // Parses one element inside a vec
        ValuePtr parseVecElement();

        // Parses a scalar value, including & macro references
        ValuePtr parseVarValue();

        // Template completion (compact stu inside a vec)
        ValuePtr parseCompactStu(const Stu &header);

        std::vector<Token> toks_;
        size_t pos_ = 0;

        // Macro table; records declarations whose names start with &
        std::unordered_map<std::string, ValuePtr> macros_;
    };

    // Parses mini_gms source text and returns the list of top-level declarations
    Stu parse(const std::string &src);

}