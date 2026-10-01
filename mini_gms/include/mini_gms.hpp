// mini_gms.hpp - public C++ API for the mini_gms configuration language.
//
// This header defines the core data model, the parser entry point, and the
// convenience Document wrapper used by C++ callers. It is the main header
// that C++ users include to parse mini_gms text and read values from it.
//
// Data model:
//   - stu : an ordered list of key/value pairs (struct)
//   - vec : an ordered list of values (array)
//   - var : a scalar (null, bool, number, or string)
//
// Two layers are exposed:
//   - mol::parse        : low-level parser that returns the raw AST
//   - mol::Document     : high-level wrapper with path-based access

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace mol
{

    // Forward declaration of Value so ValuePtr can be defined first.
    struct Value;

    // Shared ownership pointer to a Value node in the AST.
    using ValuePtr = std::shared_ptr<Value>;

    // Stu: an ordered list of named members (struct).
    using Stu = std::vector<std::pair<std::string, ValuePtr>>;

    // Vec: an ordered list of values (array).
    using Vec = std::vector<ValuePtr>;

    // Var: a scalar value, either null, bool, double, or string.
    using Var = std::variant<std::monostate, bool, double, std::string>;

    // A single node in the AST. Holds exactly one of Stu, Vec, or Var.
    struct Value
    {
        // The underlying data of this node. The active alternative decides
        // whether this node is a stu, a vec, or a var.
        std::variant<Stu, Vec, Var> data;

        // Returns true if this node holds a stu.
        bool isStu() const { return std::holds_alternative<Stu>(data); }

        // Returns true if this node holds a vec.
        bool isVec() const { return std::holds_alternative<Vec>(data); }

        // Returns true if this node holds a var (scalar).
        bool isVar() const { return std::holds_alternative<Var>(data); }

        // Read-only access to the underlying stu. Throws if not a stu.
        const Stu &asStu() const { return std::get<Stu>(data); }

        // Read-only access to the underlying vec. Throws if not a vec.
        const Vec &asVec() const { return std::get<Vec>(data); }

        // Read-only access to the underlying var. Throws if not a var.
        const Var &asVar() const { return std::get<Var>(data); }

        // Mutable access to the underlying stu. Throws if not a stu.
        Stu &asStu() { return std::get<Stu>(data); }

        // Mutable access to the underlying vec. Throws if not a vec.
        Vec &asVec() { return std::get<Vec>(data); }

        // Mutable access to the underlying var. Throws if not a var.
        Var &asVar() { return std::get<Var>(data); }

        // Looks up a member by key. Returns nullptr if this is not a stu
        // or the key is not present.
        ValuePtr get(const std::string &key) const;
    };

    // Parses mini_gms source text into a top-level list of named declarations.
    // Throws mol::Error on syntax errors.
    std::vector<std::pair<std::string, ValuePtr>> parse(const std::string &src);

}

#include <stdexcept>

namespace mol
{

    // Type aliases for scalar values inside a var.
    using null = std::monostate;
    using boolean = bool;
    using number = double;
    using string = std::string;

    // Exception type thrown by the parser and by Document accessors.
    class Error : public std::runtime_error
    {
    public:
        // Constructs an Error with the given message.
        explicit Error(const std::string &msg) : std::runtime_error(msg) {}
    };

    // A single segment of a dotted path, either a key name or an array index.
    struct PathToken
    {
        // True if this token is an array index, false if it is a key name.
        bool isIndex = false;

        // Key name. Only meaningful when isIndex is false.
        std::string name;

        // Array index. Only meaningful when isIndex is true.
        size_t index = 0;
    };

    // Parses a dotted path such as "app.window.width" or "app.servers[0].host"
    // into a list of PathToken. Throws Error on malformed index syntax.
    inline std::vector<PathToken> parsePath(const std::string &path)
    {
        std::vector<PathToken> out;
        size_t i = 0;
        const size_t n = path.size();

        while (i < n)
        {
            if (path[i] == '.')
            {
                ++i;
                continue;
            }

            if (path[i] == '[')
            {
                ++i;
                size_t start = i;
                while (i < n && std::isdigit(static_cast<unsigned char>(path[i])))
                    ++i;
                if (i >= n || path[i] != ']' || i == start)
                {
                    throw Error("invalid array index in path: " + path);
                }
                PathToken t;
                t.isIndex = true;
                t.index = static_cast<size_t>(std::stoul(path.substr(start, i - start)));
                out.push_back(t);
                ++i;
                continue;
            }

            size_t start = i;
            while (i < n && path[i] != '.' && path[i] != '[')
                ++i;
            if (i > start)
            {
                PathToken t;
                t.isIndex = false;
                t.name = path.substr(start, i - start);
                out.push_back(t);
            }
        }
        return out;
    }

    // Walks a list of PathToken starting from the given root node.
    // Returns the node the path points to, or nullptr if any step fails.
    inline mol::ValuePtr walk(const mol::ValuePtr &root,
                              const std::vector<PathToken> &tokens)
    {
        mol::ValuePtr cur = root;
        for (const auto &t : tokens)
        {
            if (!cur)
                return nullptr;

            if (t.isIndex)
            {
                if (!cur->isVec())
                    return nullptr;
                const auto &vec = cur->asVec();
                if (t.index >= vec.size())
                    return nullptr;
                cur = vec[t.index];
            }
            else
            {
                if (!cur->isStu())
                    return nullptr;
                cur = cur->get(t.name);
            }
        }
        return cur;
    }

    // High-level wrapper around a parsed mini_gms document.
    // Provides path-based lookup of scalar values and nodes.
    class Document
    {
        // Top-level declarations of the parsed document.
        std::vector<std::pair<std::string, mol::ValuePtr>> ast_;

        // Resolves a path whose first token is a top-level declaration name.
        // Returns nullptr if the first token is an index or no match is found.
        mol::ValuePtr walkFromTop(const std::vector<PathToken> &tokens) const
        {
            if (tokens.empty())
                return nullptr;

            const auto &first = tokens[0];
            if (first.isIndex)
                return nullptr;

            for (const auto &kv : ast_)
            {
                if (kv.first == first.name)
                {
                    std::vector<PathToken> rest(tokens.begin() + 1, tokens.end());
                    return rest.empty() ? kv.second : walk(kv.second, rest);
                }
            }
            return nullptr;
        }

    public:
        // Constructs an empty document.
        Document() = default;

        // Constructs a document from a pre-parsed top-level AST.
        explicit Document(std::vector<std::pair<std::string, mol::ValuePtr>> ast)
            : ast_(std::move(ast))
        {
        }

        // Parses mini_gms source text and returns a Document.
        // Throws mol::Error on syntax errors.
        static Document parse(const std::string &src)
        {
            return Document(mol::parse(src));
        }

        // Returns the scalar at the given path converted to T.
        // Returns def if the path is missing or the type does not match.
        template <typename T>
        T get(const std::string &path, T def = T{}) const
        {
            auto v = node(path);
            if (!v || !v->isVar())
                return def;
            const auto &var = v->asVar();
            if constexpr (std::is_same_v<T, bool>)
            {
                if (std::holds_alternative<bool>(var))
                    return std::get<bool>(var);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (std::holds_alternative<double>(var))
                    return std::get<double>(var);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (std::holds_alternative<std::string>(var))
                    return std::get<std::string>(var);
            }
            return def;
        }

        // Returns the scalar at the given path converted to T.
        // Throws Error if the path is missing or the type does not match.
        template <typename T>
        T require(const std::string &path) const
        {
            auto v = node(path);
            if (!v)
                throw Error("path not found: " + path);
            if (!v->isVar())
                throw Error("not a scalar: " + path);
            const auto &var = v->asVar();
            if constexpr (std::is_same_v<T, bool>)
            {
                if (std::holds_alternative<bool>(var))
                    return std::get<bool>(var);
            }
            else if constexpr (std::is_same_v<T, double>)
            {
                if (std::holds_alternative<double>(var))
                    return std::get<double>(var);
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                if (std::holds_alternative<std::string>(var))
                    return std::get<std::string>(var);
            }
            throw Error("type mismatch at: " + path);
        }

        // Returns true if a node exists at the given path.
        bool has(const std::string &path) const
        {
            return node(path) != nullptr;
        }

        // Returns the node at the given path, or nullptr if not found.
        // Use this for manual traversal of stu and vec nodes.
        mol::ValuePtr node(const std::string &path) const
        {
            return walkFromTop(parsePath(path));
        }

        // Iterates over the top-level declarations.
        auto begin() const { return ast_.begin(); }

        // Iterates over the top-level declarations.
        auto end() const { return ast_.end(); }
    };

}