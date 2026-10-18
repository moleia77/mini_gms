#pragma once

// mini_gms document module
// Provides path-based access, traversal, and length queries
// Exposes the MgmsDoc class to the outside

#include <string>
#include <vector>
#include <stdexcept>
#include <variant>
#include "mini_gms_value.hpp"

namespace mol
{

    // Exception type; both parse errors and access errors throw it
    class Error : public std::runtime_error
    {
    public:
        // Constructs with the given message
        explicit Error(const std::string &msg) : std::runtime_error(msg) {}
    };

    // One segment of a path: either a name or an index
    struct PathToken
    {
        // Whether this token is an index
        bool isIndex = false;

        // Name; valid only when isIndex is false
        std::string name;

        // Index; valid only when isIndex is true
        size_t index = 0;
    };

    // Parses a path string into a list of path segments
    std::vector<PathToken> parsePath(const std::string &path);

    // Walks the AST from the given root, one path segment at a time
    ValuePtr walk(const ValuePtr &root, const std::vector<PathToken> &tokens);

    // Document class; holds the parsed top-level declarations and provides path access
    class MgmsDoc
    {
    public:
        // Constructs an empty document
        MgmsDoc() = default;

        // Constructs from already-parsed top-level declarations
        explicit MgmsDoc(Stu ast);

        // Parses source text and returns a document
        static MgmsDoc parse(const std::string &src);

        // Gets a scalar; returns the default value if the path is missing or the type does not match
        template <typename T>
        T get(const std::string &path, T def = T{}) const
        {
            auto v = node(path);
            if (!v || !v->isVar())
            {
                return def;
            }
            const auto &var = v->asVar();
            if constexpr (std::is_same_v<T, bool>)
            {
                if (std::holds_alternative<bool>(var))
                {
                    return std::get<bool>(var);
                }
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (std::holds_alternative<double>(var))
                {
                    return std::get<double>(var);
                }
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (std::holds_alternative<std::string>(var))
                {
                    return std::get<std::string>(var);
                }
            }
            return def;
        }

        // Gets a scalar; throws if the path is missing or the type does not match
        template <typename T>
        T require(const std::string &path) const
        {
            auto v = node(path);
            if (!v)
            {
                throw Error("path not found: " + path);
            }
            if (!v->isVar())
            {
                throw Error("not a scalar: " + path);
            }
            const auto &var = v->asVar();
            if constexpr (std::is_same_v<T, bool>)
            {
                if (std::holds_alternative<bool>(var))
                {
                    return std::get<bool>(var);
                }
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (std::holds_alternative<double>(var))
                {
                    return std::get<double>(var);
                }
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (std::holds_alternative<std::string>(var))
                {
                    return std::get<std::string>(var);
                }
            }
            throw Error("type mismatch at: " + path);
        }

        // Returns true if the path exists
        bool has(const std::string &path) const;

        // Gets a node; returns nullptr if not found
        ValuePtr node(const std::string &path) const;

        // Gets a vec node; returns an empty vec if the node is not a vec
        Vec getVec(const std::string &path) const;

        // Gets the length of a vec or stu; returns 0 for other types
        size_t size(const std::string &path) const;

        // Gets the i-th element of a vec; returns nullptr if out of range or not a vec
        ValuePtr at(const std::string &path, size_t i) const;

        // Iterates over the top-level declarations
        auto begin() const { return ast_.begin(); }

        // Iterates over the top-level declarations
        auto end() const { return ast_.end(); }

    private:
        // Top-level declarations
        Stu ast_;

        // Looks up a path starting from the top level
        ValuePtr walkFromTop(const std::vector<PathToken> &tokens) const;
    };

}