// mini_gms.cpp - implementation of the mini_gms C++ parser and Document API.
//
// This file implements everything declared in mini_gms.hpp:
//   - Value::get       : member lookup inside a stu
//   - mol::parse       : the lexer + parser that turns source text into an AST
//   - Document helpers : path parsing and node walking (inline in the header)
//
// The parser is a hand-written two-stage design:
//   1. Lexer  : turns raw text into a flat list of tokens
//   2. Parser : consumes tokens and builds the AST of stu / vec / var nodes
//
// Type rules enforced here:
//   name { ... }    -> stu
//   name : { ... }  -> vec
//   name : value    -> var (scalar)

#include "mini_gms.hpp"

#include <stdexcept>

namespace mol
{

    // Looks up a member by key inside a stu.
    // Returns the first matching value, or nullptr if this is not a stu
    // or the key is not present. Duplicate keys resolve to the first one.
    ValuePtr Value::get(const std::string &key) const
    {
        if (!isStu())
            return nullptr;
        for (auto &kv : asStu())
            if (kv.first == key)
                return kv.second;
        return nullptr;
    }

    namespace
    {

        // Builds an error message with a line number appended.
        std::string errAt(const std::string &msg, int line)
        {
            return msg + " at line " + std::to_string(line);
        }

        // Token kinds produced by the lexer.
        enum class Tok
        {
            LBracket,
            RBracket,
            LBrace,
            RBrace,
            Colon,
            Ident,
            String,
            Number,
            True,
            False,
            Null,
            End
        };

        // A single token with its kind, raw text, and source line.
        struct Token
        {
            Tok type;
            std::string text;
            int line;
        };

        // Lexer: converts mini_gms source text into a flat token list.
        // Handles whitespace, '#' line comments, punctuation, strings,
        // numbers, identifiers, and the keywords true / false / null.
        class Lexer
        {
        public:
            // Stores a reference to the source text. The source must
            // outlive the Lexer.
            explicit Lexer(const std::string &src) : src_(src) {}

            // Runs the lexer and returns all tokens, ending with Tok::End.
            // Throws std::runtime_error on unexpected characters or
            // unterminated strings.
            std::vector<Token> tokenize()
            {

                std::vector<Token> out;
                while (true)
                {
                    skipSpace();
                    if (pos_ >= src_.size())
                    {
                        out.push_back({Tok::End, "", line_});
                        break;
                    }
                    char c = src_[pos_];
                    int ln = line_;

                    if (c == '[')
                    {
                        out.push_back({Tok::LBracket, "[", ln});
                        pos_++;
                    }
                    else if (c == ']')
                    {
                        out.push_back({Tok::RBracket, "]", ln});
                        pos_++;
                    }
                    else if (c == '{')
                    {
                        out.push_back({Tok::LBrace, "{", ln});
                        pos_++;
                    }
                    else if (c == '}')
                    {
                        out.push_back({Tok::RBrace, "}", ln});
                        pos_++;
                    }
                    else if (c == ':')
                    {
                        out.push_back({Tok::Colon, ":", ln});
                        pos_++;
                    }
                    else if (c == '"')
                    {
                        out.push_back(readString());
                    }
                    else if (c == '-' || (c >= '0' && c <= '9'))
                    {
                        out.push_back(readNumber());
                    }
                    else if (isIdentStart(c))
                    {
                        out.push_back(readIdent());
                    }
                    else
                    {
                        throw std::runtime_error(
                            errAt("unexpected character '" + std::string(1, c) + "'", line_));
                    }
                }
                return out;
            }

        private:
            const std::string &src_;
            size_t pos_ = 0;
            int line_ = 1;

            // Skips spaces, tabs, carriage returns, newlines, and '#' line
            // comments. The newline that ends a comment is handled by the
            // next call so that line numbers stay correct.
            void skipSpace()
            {
                while (pos_ < src_.size())
                {
                    char c = src_[pos_];
                    if (c == '\n')
                    {
                        line_++;
                        pos_++;
                    }
                    else if (c == ' ' || c == '\t' || c == '\r')
                        pos_++;
                    else if (c == '#')
                    {
                        // Skip to the end of the line without consuming '\n'.
                        while (pos_ < src_.size() && src_[pos_] != '\n')
                            pos_++;
                    }
                    else
                        break;
                }
            }

            // Returns true if c can start an identifier.
            static bool isIdentStart(char c)
            {
                return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
            }

            // Returns true if c can appear after the first character of an
            // identifier.
            static bool isIdentChar(char c)
            {
                return isIdentStart(c) || (c >= '0' && c <= '9');
            }

            // Reads a double-quoted string. Supports the escapes
            // \n \t \" \\ and treats any other escape as the literal
            // escaped character. Throws on an unterminated string.
            Token readString()
            {
                int ln = line_;
                pos_++;
                std::string s;
                while (pos_ < src_.size() && src_[pos_] != '"')
                {
                    char c = src_[pos_];
                    if (c == '\\' && pos_ + 1 < src_.size())
                    {
                        char n = src_[pos_ + 1];
                        switch (n)
                        {
                        case 'n':
                            s += '\n';
                            break;
                        case 't':
                            s += '\t';
                            break;
                        case '"':
                            s += '"';
                            break;
                        case '\\':
                            s += '\\';
                            break;
                        default:
                            s += n;
                            break;
                        }
                        pos_ += 2;
                    }
                    else
                    {
                        if (c == '\n')
                            line_++;
                        s += c;
                        pos_++;
                    }
                }
                if (pos_ >= src_.size())
                    throw std::runtime_error(errAt("unterminated string", ln));
                pos_++;
                return {Tok::String, s, ln};
            }

            // Reads a number token. Accepts an optional leading '-', digits,
            // and at most any number of '.' characters. Throws if no digit
            // is found. The parser converts the text to double later.
            Token readNumber()
            {
                int ln = line_;
                size_t start = pos_;
                if (src_[pos_] == '-')
                    pos_++;
                bool hasDigit = false;
                while (pos_ < src_.size() &&
                       ((src_[pos_] >= '0' && src_[pos_] <= '9') || src_[pos_] == '.'))
                {
                    if (src_[pos_] >= '0' && src_[pos_] <= '9')
                        hasDigit = true;
                    pos_++;
                }
                if (!hasDigit)
                    throw std::runtime_error(errAt("malformed number", ln));
                return {Tok::Number, src_.substr(start, pos_ - start), ln};
            }

            // Reads an identifier and maps the keywords true / false / null
            // to their dedicated token kinds. Any other word becomes Ident.
            Token readIdent()
            {
                int ln = line_;
                size_t start = pos_;
                while (pos_ < src_.size() && isIdentChar(src_[pos_]))
                    pos_++;
                std::string s = src_.substr(start, pos_ - start);
                if (s == "true")
                    return {Tok::True, s, ln};
                if (s == "false")
                    return {Tok::False, s, ln};
                if (s == "null")
                    return {Tok::Null, s, ln};
                return {Tok::Ident, s, ln};
            }
        };

        // Parser: consumes the token list and builds the AST.
        // Top-level declarations must carry a tag such as [stu], [vec],
        // or [var]. Nested declarations may omit the tag because the form
        // already implies the type.
        class Parser
        {
        public:
            // Takes ownership of the token list produced by the lexer.
            explicit Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}

            // Parses the whole input as a sequence of top-level declarations.
            std::vector<std::pair<std::string, ValuePtr>> parseTopLevel()
            {
                std::vector<std::pair<std::string, ValuePtr>> out;
                while (peek().type != Tok::End)
                {
                    out.push_back(parseTopDecl());
                }
                return out;
            }

        private:
            std::vector<Token> toks_;
            size_t pos_ = 0;

            // Returns the current token without consuming it.
            const Token &peek() const { return toks_[pos_]; }

            // Consumes and returns the current token.
            const Token &next() { return toks_[pos_++]; }

            // Returns true if the current token has the given kind.
            bool check(Tok t) const { return peek().type == t; }

            // Consumes the current token if it has the given kind, otherwise
            // throws an error with the given message.
            const Token &expect(Tok t, const std::string &msg)
            {
                if (!check(t))
                    throw std::runtime_error(errAt(msg, peek().line));
                return next();
            }

            // Parses one top-level declaration of the form
            //   [tag] name ...
            // and dispatches to the matching body parser.
            std::pair<std::string, ValuePtr> parseTopDecl()
            {
                if (!check(Tok::LBracket))
                    throw std::runtime_error(errAt(
                        "top-level declaration must start with '['", peek().line));
                next();
                std::string tag = expect(Tok::Ident, "expected tag name").text;
                expect(Tok::RBracket, "expected ']'");
                std::string name = expect(Tok::Ident, "expected declaration name").text;

                ValuePtr v;
                if (tag == "stu")
                {
                    v = parseStuBody();
                }
                else if (tag == "vec")
                {
                    expect(Tok::Colon, "expected ':' after vec declaration");
                    v = parseVecBody();
                }
                else if (tag == "var")
                {
                    expect(Tok::Colon, "expected ':' after var declaration");
                    v = parseVarValue();
                }
                else
                {
                    throw std::runtime_error(errAt(
                        "unknown tag [" + tag + "]", peek().line));
                }
                return {name, v};
            }

            // Parses a stu body: '{' followed by members until '}'.
            ValuePtr parseStuBody()
            {
                expect(Tok::LBrace, "expected '{' for stu");
                Stu s;
                while (!check(Tok::RBrace))
                {
                    if (check(Tok::End))
                        throw std::runtime_error(errAt("unterminated stu", peek().line));
                    s.push_back(parseMember());
                }
                expect(Tok::RBrace, "expected '}' for stu");

                auto v = std::make_shared<Value>();
                v->data = std::move(s);
                return v;
            }

            // Parses one member inside a stu. An optional [tag] prefix is
            // accepted and ignored, because the form already determines the
            // type. The value is a nested stu, a vec, or a scalar.
            std::pair<std::string, ValuePtr> parseMember()
            {
                if (check(Tok::LBracket))
                {
                    next();
                    expect(Tok::Ident, "expected tag name");
                    expect(Tok::RBracket, "expected ']'");
                }
                std::string name = expect(Tok::Ident, "expected member name").text;

                ValuePtr v;
                if (check(Tok::LBrace))
                {
                    v = parseStuBody();
                }
                else if (check(Tok::Colon))
                {
                    next();
                    if (check(Tok::LBrace))
                        v = parseVecBody();
                    else
                        v = parseVarValue();
                }
                else
                {
                    throw std::runtime_error(errAt(
                        "malformed member '" + name + "'", peek().line));
                }
                return {name, v};
            }

            // Parses a vec body: '{' followed by elements until '}'.
            // Elements may be of any type and may be mixed.
            ValuePtr parseVecBody()
            {
                expect(Tok::LBrace, "expected '{' for vec");
                Vec vec;
                while (!check(Tok::RBrace))
                {
                    if (check(Tok::End))
                        throw std::runtime_error(errAt("unterminated vec", peek().line));
                    vec.push_back(parseVecElement());
                }
                expect(Tok::RBrace, "expected '}' for vec");

                auto v = std::make_shared<Value>();
                v->data = std::move(vec);
                return v;
            }

            // Parses one element inside a vec. An anonymous stu is written
            // as '{ ... }' and an anonymous sub-vec as ':' followed by
            // '{ ... }'. Anything else is treated as a scalar value.
            ValuePtr parseVecElement()
            {
                if (check(Tok::LBrace))
                    return parseStuBody();
                if (check(Tok::Colon))
                {
                    next();
                    return parseVecBody();
                }
                return parseVarValue();
            }

            // Parses a scalar value into a Var node. Accepts string, number,
            // true, false, and null. Throws on any other token.
            ValuePtr parseVarValue()
            {
                Var var;
                const Token &t = peek();
                switch (t.type)
                {
                case Tok::String:
                    var = next().text;
                    break;
                case Tok::Number:
                    var = std::stod(next().text);
                    break;
                case Tok::True:
                    var = true;
                    next();
                    break;
                case Tok::False:
                    var = false;
                    next();
                    break;
                case Tok::Null:
                    var = std::monostate{};
                    next();
                    break;
                default:
                    throw std::runtime_error(errAt("expected scalar value", t.line));
                }
                auto v = std::make_shared<Value>();
                v->data = std::move(var);
                return v;
            }
        };

    }

    // Public parser entry point. Runs the lexer, then the parser, and
    // returns the top-level AST. Throws mol::Error on syntax errors.
    std::vector<std::pair<std::string, ValuePtr>> parse(const std::string &src)
    {
        Lexer lex(src);
        Parser p(lex.tokenize());
        return p.parseTopLevel();
    }

}