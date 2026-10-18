#pragma once

// mini_gms 文档模块
// 提供路径取值 遍历 长度 等访问接口
// 对外使用 MgmsDoc 类

#include <string>
#include <vector>
#include <stdexcept>
#include <variant>
#include "mini_gms_value.hpp"

namespace mol {

    // 异常类型, 解析错误和访问错误都抛它
    class Error : public std::runtime_error
    {
    public:
        // 用给定消息构造
        explicit Error(const std::string& msg) : std::runtime_error(msg) {}
    };

    // 路径中的一段, 要么是名字, 要么是下标
    struct PathToken
    {
        // 是否为下标
        bool isIndex = false;

        // 名字, 仅当 isIndex 为 false 时有效
        std::string name;

        // 下标, 仅当 isIndex 为 true 时有效
        size_t index = 0;
    };

    // 把路径字符串解析成路径段列表
    std::vector<PathToken> parsePath(const std::string& path);

    // 从给定根节点出发, 按路径段逐级查找
    ValuePtr walk(const ValuePtr& root, const std::vector<PathToken>& tokens);

    // 文档类, 持有解析后的顶层声明, 提供路径访问
    class MgmsDoc
    {
    public:
        // 构造空文档
        MgmsDoc() = default;

        // 用已解析的顶层声明构造
        explicit MgmsDoc(Stu ast);

        // 解析源码, 返回文档
        static MgmsDoc parse(const std::string& src);

        // 取标量, 路径不存在或类型不符时返回默认值
        template <typename T>
        T get(const std::string& path, T def = T{}) const
        {
            auto v = node(path);
            if (!v || !v->isVar()) {
                return def;
            }
            const auto& var = v->asVar();
            if constexpr (std::is_same_v<T, bool>) {
                if (std::holds_alternative<bool>(var)) {
                    return std::get<bool>(var);
                }
            }
            else if constexpr (std::is_same_v<T, double>) {
                if (std::holds_alternative<double>(var)) {
                    return std::get<double>(var);
                }
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                if (std::holds_alternative<std::string>(var)) {
                    return std::get<std::string>(var);
                }
            }
            return def;
        }

        // 取标量, 路径不存在或类型不符时抛出
        template <typename T>
        T require(const std::string& path) const
        {
            auto v = node(path);
            if (!v) {
                throw Error("path not found: " + path);
            }
            if (!v->isVar()) {
                throw Error("not a scalar: " + path);
            }
            const auto& var = v->asVar();
            if constexpr (std::is_same_v<T, bool>) {
                if (std::holds_alternative<bool>(var)) {
                    return std::get<bool>(var);
                }
            }
            else if constexpr (std::is_same_v<T, double>) {
                if (std::holds_alternative<double>(var)) {
                    return std::get<double>(var);
                }
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                if (std::holds_alternative<std::string>(var)) {
                    return std::get<std::string>(var);
                }
            }
            throw Error("type mismatch at: " + path);
        }

        // 路径是否存在
        bool has(const std::string& path) const;

        // 取节点, 找不到时返回 nullptr
        ValuePtr node(const std::string& path) const;

        // 取 vec 节点, 非 vec 时返回空 vec
        Vec getVec(const std::string& path) const;

        // 取 vec 或 stu 的长度, 其他类型返回 0
        size_t size(const std::string& path) const;

        // 取 vec 的第 i 个元素, 越界或非 vec 时返回 nullptr
        ValuePtr at(const std::string& path, size_t i) const;

        // 遍历顶层声明
        auto begin() const { return ast_.begin(); }

        // 遍历顶层声明
        auto end() const { return ast_.end(); }

    private:
        // 顶层声明
        Stu ast_;

        // 从顶层开始按路径查找
        ValuePtr walkFromTop(const std::vector<PathToken>& tokens) const;
    };

}