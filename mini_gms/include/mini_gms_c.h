// mini_gms_c.h - public C ABI for the mini_gms configuration language.
//
// This header exposes a stable C interface so that any language with a C FFI
// (Python, Rust, Go, C#, ...) can parse mini_gms text and read values from it.
// It mirrors the C++ data model: every node is either a stu, a vec, or a var.
//
// Ownership rules:
//   - mgms_parse returns a mgms_doc* owned by the caller.
//   - The caller must release it with mgms_free.
//   - All mgms_value* pointers returned by accessors are borrowed from the
//     document and stay valid only until mgms_free is called.

#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Opaque handle to a parsed document.
    typedef struct mgms_doc mgms_doc;

    // Opaque handle to a single value node inside a document.
    typedef struct mgms_value mgms_value;

    // Parses mini_gms source text.
    // src and len describe the input buffer.
    // On success, returns a non-null document owned by the caller.
    // On failure, returns NULL and writes a message into errbuf when provided.
    mgms_doc *mgms_parse(const char *src, size_t len,
                         char *errbuf, size_t errbuf_size);

    // Releases a document returned by mgms_parse.
    // Passing NULL is safe and does nothing.
    void mgms_free(mgms_doc *doc);

    // Returns the number of top-level declarations in the document.
    size_t mgms_top_count(const mgms_doc *doc);

    // Returns the name of the i-th top-level declaration.
    // Returns NULL if the index is out of range.
    const char *mgms_top_name(const mgms_doc *doc, size_t i);

    // Returns the value node of the i-th top-level declaration.
    // Returns NULL if the index is out of range.
    const mgms_value *mgms_top_value(const mgms_doc *doc, size_t i);

    // Returns non-zero if the value is a stu (struct).
    int mgms_is_stu(const mgms_value *v);

    // Returns non-zero if the value is a vec (array).
    int mgms_is_vec(const mgms_value *v);

    // Returns non-zero if the value is a var (scalar).
    int mgms_is_var(const mgms_value *v);

    // Returns the number of members in a stu, or 0 if not a stu.
    size_t mgms_stu_count(const mgms_value *v);

    // Returns the key of the i-th member of a stu.
    // Returns NULL if the value is not a stu or the index is out of range.
    const char *mgms_stu_key(const mgms_value *v, size_t i);

    // Returns the value of the i-th member of a stu.
    // Returns NULL if the value is not a stu or the index is out of range.
    const mgms_value *mgms_stu_value(const mgms_value *v, size_t i);

    // Looks up a member of a stu by key.
    // Returns NULL if the value is not a stu or the key is missing.
    const mgms_value *mgms_stu_get(const mgms_value *v, const char *key);

    // Returns the number of elements in a vec, or 0 if not a vec.
    size_t mgms_vec_count(const mgms_value *v);

    // Returns the i-th element of a vec.
    // Returns NULL if the value is not a vec or the index is out of range.
    const mgms_value *mgms_vec_at(const mgms_value *v, size_t i);

    // Returns non-zero if the var holds null.
    int mgms_var_is_null(const mgms_value *v);

    // Returns non-zero if the var holds a boolean.
    int mgms_var_is_bool(const mgms_value *v);

    // Returns non-zero if the var holds a number.
    int mgms_var_is_number(const mgms_value *v);

    // Returns non-zero if the var holds a string.
    int mgms_var_is_string(const mgms_value *v);

    // Returns the boolean value of a var as 0 or 1.
    // Returns 0 if the value is not a boolean var.
    int mgms_var_bool(const mgms_value *v);

    // Returns the numeric value of a var.
    // Returns 0.0 if the value is not a number var.
    double mgms_var_number(const mgms_value *v);

    // Returns the string value of a var as a null-terminated UTF-8 buffer.
    // Returns NULL if the value is not a string var.
    // The pointer is owned by the document and stays valid until mgms_free.
    const char *mgms_var_string(const mgms_value *v);

#ifdef __cplusplus
}
#endif