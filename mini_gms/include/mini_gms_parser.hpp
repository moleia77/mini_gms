#pragma once

// mini_gms 解析器模块
// 声明词法分析器和语法分析器
// 属于内部模块, 外部不应直接使用

#include <string>
#include <vector>
#include <unordered_map>
#include "mini_gms_value.hpp"

namespace mol::mgms {

        // 词法单元类型
        enum class Tok {
            LBracket,   // [
            RBracket,   // ]
            LBrace,     // {
            RBrace,     // }
            Colon,      // :
            Tilde,      // ~
            Ident,      // 标识符,含 & 开头的宏名
            StringRaw,  // 双引号原始字符串
            StringEsc,  // 单引号转义字符串
            Number,     // 数字
            True,       // true
            False,      // false
            Null,       // null
            End         // 结束
        };

        // 一个词法单元, 含类型 原文 行号
        struct Token {
            Tok type;
            int line;
            std::string text;
        };

        // 词法分析器, 把源码文本转换成词法单元列表
        class Lexer {
        public:
            // 保存源码引用, 源码必须比 Lexer 活得久
            explicit Lexer(const std::string& src);

            // 扫描全部词法单元, 末尾追加 Tok::End
            // 遇到非法字符或未闭合字符串时抛出
            std::vector<Token> tokenize();

        private:
            // 跳过空白和 # 行注释
            void skipSpace();

            // 是否为标识符首字符, 含 &
            static bool isIdentStart(char c);

            // 是否为标识符后续字符
            static bool isIdentChar(char c);

            // 读取双引号原始字符串, 遇到 " 结束, 无转义
            Token readStringRaw();

            // 读取单引号转义字符串, 遇到 ' 结束, 可转义
            Token readStringEsc();

            // 读取数字, 支持可选负号和小数点
            Token readNumber();

            // 读取标识符, 并把关键字映射成专用类型
            Token readIdent();

            const std::string& src_;
            size_t pos_ = 0;
            int line_ = 1;
        };

        // 语法分析器, 把词法单元列表转换成 AST
        class Parser {
        public:
            // 接收词法单元列表
            explicit Parser(std::vector<Token> toks);

            // 解析全部顶层声明
            Stu parseTopLevel();

        private:
            // 查看当前词法单元, 不消费
            const Token& peek() const;

            // 消费并返回当前词法单元
            const Token& next();

            // 当前词法单元是否为指定类型
            bool check(Tok t) const;

            // 当前词法单元必须为指定类型, 否则抛出
            const Token& expect(Tok t, const std::string& msg);

            // 解析一条顶层声明
            StuMember parseTopDecl();

            // 解析一批无 tag 的声明
            void parseBatchDecl(Stu& out);

            // 解析 stu 主体
            ValuePtr parseStuBody();

            // 解析 stu 中的一个成员
            StuMember parseMember();

            // 解析 vec 主体, 含 ~ 范围展开
            ValuePtr parseVecBody();

            // 解析 vec 中的一个元素
            ValuePtr parseVecElement();

            // 解析标量值, 含 & 宏引用
            ValuePtr parseVarValue();

            // 模板补齐（vec 里的紧凑 stu）
            ValuePtr parseCompactStu(const Stu& header);

            std::vector<Token> toks_;
            size_t pos_ = 0;

            // 宏表, 记录 & 开头的声明
            std::unordered_map<std::string, ValuePtr> macros_;
        };

        // 解析 mini_gms 源码, 返回顶层声明列表
        Stu parse(const std::string& src);

}