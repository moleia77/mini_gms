// mini_gms_c.cpp - implementation of the mini_gms C ABI.
//
// This file implements everything declared in mini_gms_c.h on top of the
// C++ core in mini_gms.hpp. It is a thin translation layer:
//   - mgms_doc   wraps std::vector<std::pair<std::string, mol::ValuePtr>>
//   - mgms_value is a reinterpret_cast of mol::Value*
//   - every accessor casts back and forwards to the C++ API
//
// All mgms_value* pointers returned by accessors are borrowed from the
// owning document and must not be freed by the caller.

#include "mini_gms_c.h"
#include "mini_gms.hpp"

#include <cstring>
#include <string>
#include <vector>
#include <variant>

// Concrete definition of the opaque mgms_doc handle.
// Holds the top-level declarations of a parsed document.
struct mgms_doc
{
    std::vector<std::pair<std::string, mol::ValuePtr>> decls;
};

// Casts an opaque C value handle back to the underlying C++ Value.
static inline const mol::Value *V(const mgms_value *v)
{
    return reinterpret_cast<const mol::Value *>(v);
}

// Casts a C++ ValuePtr to the opaque C value handle.
static inline const mgms_value *C(const mol::ValuePtr &v)
{
    return reinterpret_cast<const mgms_value *>(v.get());
}

// Returns the Var payload of a value if it is a scalar, otherwise nullptr.
static const mol::Var *asVar(const mgms_value *v)
{
    if (!v)
        return nullptr;
    const auto *val = V(v);
    if (!val->isVar())
        return nullptr;
    return &val->asVar();
}

extern "C"
{

    // Parses mini_gms source text into a new document.
    // On success returns a non-null handle owned by the caller.
    // On failure returns NULL and writes the error message into errbuf
    // when errbuf and errbuf_size are both valid.
    mgms_doc *mgms_parse(const char *src, size_t len,
                         char *errbuf, size_t errbuf_size)
    {
        try
        {
            auto *doc = new mgms_doc();
            doc->decls = mol::parse(std::string(src, len));
            return doc;
        }
        catch (const std::exception &e)
        {
            if (errbuf && errbuf_size > 0)
            {
                std::strncpy(errbuf, e.what(), errbuf_size - 1);
                errbuf[errbuf_size - 1] = '\0';
            }
            return nullptr;
        }
        catch (...)
        {
            if (errbuf && errbuf_size > 0)
            {
                std::strncpy(errbuf, "unknown error", errbuf_size - 1);
                errbuf[errbuf_size - 1] = '\0';
            }
            return nullptr;
        }
    }

    // Releases a document. Passing NULL is safe.
    void mgms_free(mgms_doc *doc)
    {
        delete doc;
    }

    // Returns the number of top-level declarations, or 0 for NULL.
    size_t mgms_top_count(const mgms_doc *doc)
    {
        return doc ? doc->decls.size() : 0;
    }

    // Returns the name of the i-th top-level declaration.
    // Returns NULL if doc is NULL or the index is out of range.
    const char *mgms_top_name(const mgms_doc *doc, size_t i)
    {
        if (!doc || i >= doc->decls.size())
            return nullptr;
        return doc->decls[i].first.c_str();
    }

    // Returns the value node of the i-th top-level declaration.
    // Returns NULL if doc is NULL or the index is out of range.
    const mgms_value *mgms_top_value(const mgms_doc *doc, size_t i)
    {
        if (!doc || i >= doc->decls.size())
            return nullptr;
        return C(doc->decls[i].second);
    }

    // Returns non-zero if the value is a stu.
    int mgms_is_stu(const mgms_value *v) { return v && V(v)->isStu(); }

    // Returns non-zero if the value is a vec.
    int mgms_is_vec(const mgms_value *v) { return v && V(v)->isVec(); }

    // Returns non-zero if the value is a var (scalar).
    int mgms_is_var(const mgms_value *v) { return v && V(v)->isVar(); }

    // Returns the number of members in a stu, or 0 if not a stu.
    size_t mgms_stu_count(const mgms_value *v)
    {
        if (!v || !V(v)->isStu())
            return 0;
        return V(v)->asStu().size();
    }

    // Returns the key of the i-th stu member.
    // Returns NULL if the value is not a stu or the index is out of range.
    const char *mgms_stu_key(const mgms_value *v, size_t i)
    {
        if (!v || !V(v)->isStu())
            return nullptr;
        const auto &s = V(v)->asStu();
        if (i >= s.size())
            return nullptr;
        return s[i].first.c_str();
    }

    // Returns the value of the i-th stu member.
    // Returns NULL if the value is not a stu or the index is out of range.
    const mgms_value *mgms_stu_value(const mgms_value *v, size_t i)
    {
        if (!v || !V(v)->isStu())
            return nullptr;
        const auto &s = V(v)->asStu();
        if (i >= s.size())
            return nullptr;
        return C(s[i].second);
    }

    // Looks up a stu member by key. Returns the first match, or NULL
    // if the value is not a stu or the key is missing.
    const mgms_value *mgms_stu_get(const mgms_value *v, const char *key)
    {
        if (!v || !key)
            return nullptr;
        return C(V(v)->get(key));
    }

    // Returns the number of elements in a vec, or 0 if not a vec.
    size_t mgms_vec_count(const mgms_value *v)
    {
        if (!v || !V(v)->isVec())
            return 0;
        return V(v)->asVec().size();
    }

    // Returns the i-th vec element.
    // Returns NULL if the value is not a vec or the index is out of range.
    const mgms_value *mgms_vec_at(const mgms_value *v, size_t i)
    {
        if (!v || !V(v)->isVec())
            return nullptr;
        const auto &vec = V(v)->asVec();
        if (i >= vec.size())
            return nullptr;
        return C(vec[i]);
    }

    // Returns non-zero if the var holds null.
    int mgms_var_is_null(const mgms_value *v)
    {
        auto *var = asVar(v);
        return var && std::holds_alternative<std::monostate>(*var);
    }

    // Returns non-zero if the var holds a boolean.
    int mgms_var_is_bool(const mgms_value *v)
    {
        auto *var = asVar(v);
        return var && std::holds_alternative<bool>(*var);
    }

    // Returns non-zero if the var holds a number.
    int mgms_var_is_number(const mgms_value *v)
    {
        auto *var = asVar(v);
        return var && std::holds_alternative<double>(*var);
    }

    // Returns non-zero if the var holds a string.
    int mgms_var_is_string(const mgms_value *v)
    {
        auto *var = asVar(v);
        return var && std::holds_alternative<std::string>(*var);
    }

    // Returns the boolean payload of a var as 0 or 1.
    // Returns 0 if the value is not a boolean var.
    int mgms_var_bool(const mgms_value *v)
    {
        auto *var = asVar(v);
        if (!var || !std::holds_alternative<bool>(*var))
            return 0;
        return std::get<bool>(*var) ? 1 : 0;
    }

    // Returns the numeric payload of a var.
    // Returns 0.0 if the value is not a number var.
    double mgms_var_number(const mgms_value *v)
    {
        auto *var = asVar(v);
        if (!var || !std::holds_alternative<double>(*var))
            return 0.0;
        return std::get<double>(*var);
    }

    // Returns the string payload of a var as a null-terminated UTF-8 buffer.
    // Returns NULL if the value is not a string var.
    // The buffer is owned by the document and stays valid until mgms_free.
    const char *mgms_var_string(const mgms_value *v)
    {
        auto *var = asVar(v);
        if (!var || !std::holds_alternative<std::string>(*var))
            return nullptr;
        return std::get<std::string>(*var).c_str();
    }
}