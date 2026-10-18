#include "mini_gms_parser.hpp"
#include <stdexcept>

namespace mol::mgms {

    static std::string errAt(const std::string& msg, int line) {
        return msg + " at line " + std::to_string(line);
    }

    Lexer::Lexer(const std::string& src) : src_(src) {}

    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> out;

        while (true) {
            skipSpace();
            if (pos_ >= src_.size()) {
                out.push_back({ Tok::End, line_, "" });
                break;
            }
            char c = src_[pos_];
            int  ln = line_;

            if (c == '[') {
                out.push_back({ Tok::LBracket, ln, "[" });
                pos_++;
            }
            else if (c == ']') {
                out.push_back({ Tok::RBracket, ln, "]" });
                pos_++;
            }
            else if (c == '{') {
                out.push_back({ Tok::LBrace, ln, "{" });
                pos_++;
            }
            else if (c == '}') {
                out.push_back({ Tok::RBrace, ln, "}" });
                pos_++;
            }
            else if (c == ':') {
                out.push_back({ Tok::Colon, ln, ":" });
                pos_++;
            }
            else if (c == '~') {
                out.push_back({ Tok::Tilde, ln, "~" });
                pos_++;
            }
            else if (c == '"') {
                out.push_back(readStringRaw());
            }
            else if (c == '\'') {
                out.push_back(readStringEsc());
            }
            else if (c == '-' || (c >= '0' && c <= '9')) {
                out.push_back(readNumber());
            }
            else if (isIdentStart(c)) {
                out.push_back(readIdent());
            }
            else {
                throw std::runtime_error(
                    errAt("unexpected character '" + std::string(1, c) + "'", line_));
            }
        }

        return out;
    }

    void Lexer::skipSpace() {
        while (pos_ < src_.size()) {
            char c = src_[pos_];

            if (c == '\n') {
                line_++;
                pos_++;
            }
            else if (c == ' ' || c == '\t' || c == '\r') {
                pos_++;
            }
            else if (c == '#') {
                while (pos_ < src_.size() && src_[pos_] != '\n') {
                    pos_++;
                }
            }
            else {
                break;
            }
        }
    }

    bool Lexer::isIdentStart(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || c == '_' || c == '&';
    }

    bool Lexer::isIdentChar(char c) {
        return isIdentStart(c) || (c >= '0' && c <= '9');
    }

    Token Lexer::readStringRaw() {
        int ln = line_;
        pos_++;
        std::string s;

        while (pos_ < src_.size() && src_[pos_] != '"') {
            if (src_[pos_] == '\n') {
                throw std::runtime_error(errAt("string cannot span lines", ln));
            }
            s += src_[pos_];
            pos_++;
        }
        if (pos_ >= src_.size()) {
            throw std::runtime_error(errAt("unterminated string", ln));
        }
        pos_++;
        return { Tok::StringRaw, ln, s };
    }

    Token Lexer::readStringEsc() {
        int ln = line_;
        pos_++;
        std::string s;

        while (pos_ < src_.size() && src_[pos_] != '\'') {
            char c = src_[pos_];

            if (c == '\\' && pos_ + 1 < src_.size()) {
                char n = src_[pos_ + 1];
                switch (n) {
                case 'n':  s += '\n'; break;
                case 't':  s += '\t'; break;
                case 'r':  s += '\r'; break;
                case '\'': s += '\''; break;
                case '\\': s += '\\'; break;
                default:   s += n;    break;
                }
                pos_ += 2;
            }
            else {
                if (c == '\n') {
                    line_++;
                }
                s += c;
                pos_++;
            }
        }
        if (pos_ >= src_.size()) {
            throw std::runtime_error(errAt("unterminated string", ln));
        }
        pos_++;
        return { Tok::StringEsc, ln, s };
    }

    Token Lexer::readNumber() {
        int    ln = line_;
        size_t start = pos_;
        bool hasDigit = false;

        if (src_[pos_] == '-') {
            pos_++;
        }
        while (pos_ < src_.size()
            && ((src_[pos_] >= '0' && src_[pos_] <= '9')
                || src_[pos_] == '.')) {
            if (src_[pos_] >= '0' && src_[pos_] <= '9') {
                hasDigit = true;
            }
            pos_++;
        }
        if (!hasDigit) {
            throw std::runtime_error(errAt("malformed number", ln));
        }
        return { Tok::Number, ln, src_.substr(start, pos_ - start) };
    }

    Token Lexer::readIdent() {
        int    ln = line_;
        size_t start = pos_;

        while (pos_ < src_.size() && isIdentChar(src_[pos_])) {
            pos_++;
        }

        std::string s = src_.substr(start, pos_ - start);
        if (s == "true") {
            return { Tok::True, ln, s };
        }
        if (s == "false") {
            return { Tok::False, ln, s };
        }
        if (s == "null") {
            return { Tok::Null, ln, s };
        }
        return { Tok::Ident, ln, s };
    }

    Parser::Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}

    const Token& Parser::peek() const {
        return toks_[pos_];
    }

    const Token& Parser::next() {
        return toks_[pos_++];
    }

    bool Parser::check(Tok t) const {
        return peek().type == t;
    }

    const Token& Parser::expect(Tok t, const std::string& msg) {
        if (!check(t)) {
            throw std::runtime_error(errAt(msg, peek().line));
        }
        return next();
    }

    Stu Parser::parseTopLevel() {

        Stu out;
        while (peek().type != Tok::End) {
            if (check(Tok::LBracket)) {
                if (pos_ + 3 < toks_.size()
                    && toks_[pos_ + 1].type == Tok::Ident
                    && toks_[pos_ + 2].type == Tok::RBracket
                    && toks_[pos_ + 3].type == Tok::LBrace) {
                    next();

                    std::string tag = expect(Tok::Ident, "expected tag").text;
                    expect(Tok::RBracket, "]");

                    if (tag != "stu") {
                        throw std::runtime_error(errAt("only [stu] can be used as a batch declaration", peek().line));
                    }

                    parseBatchDecl(out);
                    continue;
                }
                out.push_back(parseTopDecl());
            }
            else {
                throw std::runtime_error(errAt("expected '[' for top-level declaration", peek().line));
            }
        }
        return out;
    }
    

    StuMember Parser::parseTopDecl() {
        expect(Tok::LBracket, "expected '['");
        std::string tag = expect(Tok::Ident, "expected tag name").text;
        expect(Tok::RBracket, "expected ']'");

        std::string name = expect(Tok::Ident, "expected declaration name").text;

        ValuePtr v;
        if (tag == "stu") {
            v = parseStuBody();
        }
        else if (tag == "vec") {
            expect(Tok::Colon, "expected ':' after vec declaration");
            v = parseVecBody();
        }
        else if (tag == "var") {
            expect(Tok::Colon, "expected ':' after var declaration");
            v = parseVarValue();
        }
        else {
            throw std::runtime_error(errAt("unknown tag [" + tag + "]", peek().line));
        }

        if (!name.empty() && name[0] == '&') {
            macros_[name] = v;
        }

        return { name, v };
    }

    void Parser::parseBatchDecl(Stu& out) {
        expect(Tok::LBrace, "expected '{'");
        while (!check(Tok::RBrace)) {
            if (check(Tok::End)) {
                throw std::runtime_error(errAt("unterminated batch declaration", peek().line));
            }
            out.push_back(parseMember());
        }
        expect(Tok::RBrace, "expected '}'");
    }

    ValuePtr Parser::parseStuBody() {
        expect(Tok::LBrace, "expected '{' for stu");
        Stu s;

        while (!check(Tok::RBrace)) {
            if (check(Tok::End)) {
                throw std::runtime_error(errAt("unterminated stu", peek().line));
            }
            if (check(Tok::LBracket)) {
                next();
                std::string tag = expect(Tok::Ident, "expected tag").text;
                expect(Tok::RBracket, "]");

                if (tag == "stu" && check(Tok::LBrace)) {
                    parseBatchDecl(s);
                }
                else {
                    throw std::runtime_error(errAt("only [stu] {} allowed here", peek().line));
                }
            }
            else {
                s.push_back(parseMember());
            }
        }
        expect(Tok::RBrace, "expected '}'");

        auto v = std::make_shared<Value>();
        v->data = std::move(s);
        return v;
    }

    StuMember Parser::parseMember() {
        if (check(Tok::LBracket)) {
            next();
            expect(Tok::Ident, "expected tag name");
            expect(Tok::RBracket, "expected ']'");
        }
        std::string name = expect(Tok::Ident, "expected member name").text;

        ValuePtr v;
        if (check(Tok::LBrace)) {
            v = parseStuBody();
        }
        else if (check(Tok::Colon)) {
            next();
            if (check(Tok::LBrace)) {
                v = parseVecBody();
            }
            else {
                v = parseVarValue();
            }
        }
        else {
            throw std::runtime_error(errAt(
                "malformed member '" + name + "'", peek().line));
        }

        if (!name.empty() && name[0] == '&') {
            macros_[name] = v;
        }

        return { name, v };
    }

    ValuePtr Parser::parseVecBody() {
        expect(Tok::LBrace, "expected '{' for vec");
        Vec vec;
        Stu header_template;
        bool has_template = false;

        while (!check(Tok::RBrace)) {
            if (check(Tok::End)) {
                throw std::runtime_error(errAt("unterminated vec", peek().line));
            }

            // 1~5 范围
            if (check(Tok::Number) && pos_ + 1 < toks_.size()
                && toks_[pos_ + 1].type == Tok::Tilde) {
                double start = std::stod(next().text);
                next();
                double end = std::stod(next().text);
                double step = (start <= end) ? 1.0 : -1.0;
                for (double i = start; step > 0 ? i <= end : i >= end; i += step) {
                    auto v = std::make_shared<Value>();
                    v->data = Var(i);
                    vec.push_back(v);
                }
                continue;
            }

            if (check(Tok::LBrace)) {
                size_t lookahead = pos_ + 1;
                bool is_compact = false;
                if (lookahead < toks_.size()) {
                    Tok t = toks_[lookahead].type;
                    if (t == Tok::StringRaw || t == Tok::StringEsc || t == Tok::Number
                        || t == Tok::True || t == Tok::False || t == Tok::Null) {
                        is_compact = true;
                    }
                }

                if (is_compact) {
                    if (!has_template) {
                        throw std::runtime_error(errAt("compact stu requires a template", peek().line));
                    }
                    vec.push_back(parseCompactStu(header_template));
                }
                else {
                    auto v = parseStuBody();
                    if (!has_template) {
                        header_template = v->asStu();
                        has_template = true;
                    }
                    vec.push_back(v);
                }
            }
            else if (check(Tok::Colon)) {
                next();
                vec.push_back(parseVecBody());
            }
            else {
                vec.push_back(parseVarValue());
            }
        }
        expect(Tok::RBrace, "expected '}' for vec");

        auto v = std::make_shared<Value>();
        v->data = std::move(vec);
        return v;
    }

    ValuePtr Parser::parseVecElement() {
        if (check(Tok::LBrace)) {
            return parseStuBody();
        }
        if (check(Tok::Colon)) {
            next();
            return parseVecBody();
        }
        return parseVarValue();
    }

    ValuePtr Parser::parseVarValue() {
        if (check(Tok::Ident)) {
            std::string name = peek().text;
            int ln = peek().line;
            if (!name.empty() && name[0] == '&') {
                next();
                auto it = macros_.find(name);
                if (it == macros_.end()) {
                    throw std::runtime_error(errAt(
                        "undefined macro '" + name + "'", ln));
                }
                return it->second;
            }
        }

        Var var;
        const Token& t = peek();
        switch (t.type) {
        case Tok::StringRaw:
        case Tok::StringEsc:
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

    ValuePtr Parser::parseCompactStu(const Stu& header) {
        expect(Tok::LBrace, "expected '{' for compact stu");
        Vec values;

        while (!check(Tok::RBrace)) {
            if (check(Tok::End)) {
                throw std::runtime_error(errAt("unterminated compact stu", peek().line));
            }
            values.push_back(parseVarValue());
        }
        expect(Tok::RBrace, "expected '}'");

        if (values.size() != header.size()) {
            throw std::runtime_error(errAt("compact stu size does not match header", peek().line));
        }

        Stu s;
        for (size_t i = 0; i < values.size(); ++i) {
            s.push_back({ header[i].first, values[i] });
        }

        auto v = std::make_shared<Value>();
        v->data = std::move(s);
        return v;
    }

    Stu parse(const std::string& src) {
        Lexer lex(src);
        Parser p(lex.tokenize());
        return p.parseTopLevel();
    }

}