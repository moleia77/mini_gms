#pragma once

// mini_gms 值模型模块
// 定义三种数据节点 stu vec var,
// 以及共用的 Value 节点类型和智能指针别名

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace mol {

    // 标量类型别名
    using mgms_null = std::monostate;
    using mgms_bool = bool;
    using mgms_num = double;
    using mgms_str = std::string;

    // 前置声明
    struct Value;

    // Value 节点的共享指针, AST 中所有节点都用它持有
    using ValuePtr = std::shared_ptr<Value>;

    using StuMember = std::pair<mgms_str, ValuePtr>;
    // stu: 有序的键值对列表, 键为字符串, 值为 Value 节点
    using Stu = std::vector<StuMember>;

    // vec: 有序的值列表, 元素为 Value 节点
    using Vec = std::vector<ValuePtr>;

    // var: 标量, 只能是 null bool double string 之一
    using Var = std::variant<mgms_null, mgms_bool, mgms_num, mgms_str>;

    

    // AST 中的一个节点, 持有 stu vec var 三者之一
    struct Value {
        // 节点的实际数据, 当前激活的 variant 分支决定节点类型
        std::variant<Stu, Vec, Var> data;

        // 是否为 stu 节点
        bool isStu() const { return std::holds_alternative<Stu>(data); }

        // 是否为 vec 节点
        bool isVec() const { return std::holds_alternative<Vec>(data); }

        // 是否为 var 节点
        bool isVar() const { return std::holds_alternative<Var>(data); }

        // 只读方式取 stu, 非 stu 时抛出
        const Stu& asStu() const { return std::get<Stu>(data); }

        // 只读方式取 vec, 非 vec 时抛出
        const Vec& asVec() const { return std::get<Vec>(data); }

        // 只读方式取 var, 非 var 时抛出
        const Var& asVar() const { return std::get<Var>(data); }

        // 可写方式取 stu, 非 stu 时抛出
        Stu& asStu() { return std::get<Stu>(data); }

        // 可写方式取 vec, 非 vec 时抛出
        Vec& asVec() { return std::get<Vec>(data); }

        // 可写方式取 var, 非 var 时抛出
        Var& asVar() { return std::get<Var>(data); }

        // 按 key 查找 stu 成员, 非 stu 或找不到时返回 nullptr
        ValuePtr get(const std::string& key) const;
    };

}