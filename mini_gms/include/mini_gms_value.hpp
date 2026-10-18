#pragma once

// mini_gms value model module
// Defines the three data nodes: stu, vec, var,
// plus the shared Value node type and smart pointer alias.

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace mol
{

    // Scalar type aliases
    using mgms_null = std::monostate;
    using mgms_bool = bool;
    using mgms_num = double;
    using mgms_str = std::string;

    // Forward declaration
    struct Value;

    // Shared pointer to a Value node; every node in the AST is held through it
    using ValuePtr = std::shared_ptr<Value>;

    using StuMember = std::pair<mgms_str, ValuePtr>;
    // stu: an ordered list of key-value pairs; keys are strings, values are Value nodes
    using Stu = std::vector<StuMember>;

    // vec: an ordered list of values; elements are Value nodes
    using Vec = std::vector<ValuePtr>;

    // var: a scalar, which can only be null, bool, double, or string
    using Var = std::variant<mgms_null, mgms_bool, mgms_num, mgms_str>;

    // A single node in the AST, holding one of stu, vec, or var
    struct Value
    {
        // The actual data of the node; the active variant branch determines the node type
        std::variant<Stu, Vec, Var> data;

        // Returns true if the node is a stu
        bool isStu() const { return std::holds_alternative<Stu>(data); }

        // Returns true if the node is a vec
        bool isVec() const { return std::holds_alternative<Vec>(data); }

        // Returns true if the node is a var
        bool isVar() const { return std::holds_alternative<Var>(data); }

        // Returns the stu by const reference; throws if the node is not a stu
        const Stu &asStu() const { return std::get<Stu>(data); }

        // Returns the vec by const reference; throws if the node is not a vec
        const Vec &asVec() const { return std::get<Vec>(data); }

        // Returns the var by const reference; throws if the node is not a var
        const Var &asVar() const { return std::get<Var>(data); }

        // Returns the stu by mutable reference; throws if the node is not a stu
        Stu &asStu() { return std::get<Stu>(data); }

        // Returns the vec by mutable reference; throws if the node is not a vec
        Vec &asVec() { return std::get<Vec>(data); }

        // Returns the var by mutable reference; throws if the node is not a var
        Var &asVar() { return std::get<Var>(data); }

        // Looks up a stu member by key; returns nullptr if not a stu or the key is missing
        ValuePtr get(const std::string &key) const;
    };

}