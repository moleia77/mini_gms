# __init__.py - Python wrapper around the mini_gms C ABI.
#
# This module exposes two layers:
#   - Document : owns a parsed document and provides dict-like access
#   - Value    : wraps a single stu, vec, or var node inside a document
#
# Two convenience functions are provided on top:
#   - parse : returns a Document
#   - loads : returns a plain Python dict / list / scalar tree
#
# A Document owns the underlying C memory. Values obtained from a Document
# borrow that memory and must not be used after the Document is closed.

import ctypes
from ._ffi import lib, _s, _is_null


class MgmsError(Exception):
    # Raised when the parser reports a syntax error.
    pass


class Value:
    # Wraps a single node inside a Document.
    # A Value keeps a reference to its owning Document so the C memory
    # stays alive while the Value is in use.

    __slots__ = ("_ptr", "_doc")

    def __init__(self, ptr, doc):
        # Stores the raw C pointer and the owning Document.
        self._ptr = ptr
        self._doc = doc

    @property
    def is_stu(self):
        # True if this node is a stu.
        return bool(lib.mgms_is_stu(self._ptr))

    @property
    def is_vec(self):
        # True if this node is a vec.
        return bool(lib.mgms_is_vec(self._ptr))

    @property
    def is_var(self):
        # True if this node is a var (scalar).
        return bool(lib.mgms_is_var(self._ptr))

    @property
    def type(self):
        # Returns "stu", "vec", or "var".
        if self.is_stu:
            return "stu"
        if self.is_vec:
            return "vec"
        return "var"

    def keys(self):
        # Returns the member names of a stu, or an empty list otherwise.
        if not self.is_stu:
            return []
        n = lib.mgms_stu_count(self._ptr)
        return [_s(lib.mgms_stu_key(self._ptr, i)) for i in range(n)]

    def get(self, key):
        # Returns the value of a stu member, or None if not found.
        # Returns None if this node is not a stu.
        if not self.is_stu:
            return None
        k = key.encode("utf-8") if isinstance(key, str) else key
        ptr = lib.mgms_stu_get(self._ptr, k)
        if _is_null(ptr):
            return None
        return Value(ptr, self._doc)

    def items(self):
        # Returns a list of (key, Value) pairs for a stu, or an empty list
        # otherwise. Order follows the source text.
        if not self.is_stu:
            return []
        n = lib.mgms_stu_count(self._ptr)
        out = []
        for i in range(n):
            k = _s(lib.mgms_stu_key(self._ptr, i))
            p = lib.mgms_stu_value(self._ptr, i)
            out.append((k, Value(p, self._doc)))
        return out

    def __len__(self):
        # Number of elements in a vec, or members in a stu. 0 for a var.
        if self.is_vec:
            return lib.mgms_vec_count(self._ptr)
        if self.is_stu:
            return lib.mgms_stu_count(self._ptr)
        return 0

    def __iter__(self):
        # Iterates over vec elements, or over stu values.
        if self.is_vec:
            for i in range(lib.mgms_vec_count(self._ptr)):
                yield Value(lib.mgms_vec_at(self._ptr, i), self._doc)
        elif self.is_stu:
            for _, v in self.items():
                yield v

    def __getitem__(self, key):
        # Indexes a vec by int, or a stu by str.
        if isinstance(key, int) and self.is_vec:
            n = lib.mgms_vec_count(self._ptr)
            if key < 0:
                key += n
            if not (0 <= key < n):
                raise IndexError(key)
            return Value(lib.mgms_vec_at(self._ptr, key), self._doc)
        if isinstance(key, str) and self.is_stu:
            v = self.get(key)
            if v is None:
                raise KeyError(key)
            return v
        raise TypeError("invalid key type")

    @property
    def value(self):
        # Returns the Python value of a var: None, bool, float, or str.
        # Raises TypeError if this node is not a var.
        if not self.is_var:
            raise TypeError(f"not a var, got {self.type}")
        if lib.mgms_var_is_null(self._ptr):
            return None
        if lib.mgms_var_is_bool(self._ptr):
            return bool(lib.mgms_var_bool(self._ptr))
        if lib.mgms_var_is_number(self._ptr):
            return lib.mgms_var_number(self._ptr)
        if lib.mgms_var_is_string(self._ptr):
            return _s(lib.mgms_var_string(self._ptr))
        return None

    def __repr__(self):
        # Short debug form: shows scalar value, vec length, or stu keys.
        if self.is_var:
            return f"Value({self.value!r})"
        if self.is_vec:
            return f"Value(vec, len={len(self)})"
        return f"Value(stu, keys={self.keys()})"

    def __eq__(self, other):
        # Compares the underlying Python value against another Value or
        # against a plain Python object.
        if isinstance(other, Value):
            return self.value == other.value
        return self.value == other


class Document:
    # Owns a parsed mini_gms document.
    # Provides dict-like access to the top-level declarations and keeps the
    # underlying C memory alive until close() is called.

    __slots__ = ("_doc", "_closed")

    def __init__(self, src):
        # Parses src (str or bytes) into a new Document.
        # Raises MgmsError if the parser reports a syntax error.
        if isinstance(src, str):
            src = src.encode("utf-8")
        errbuf = ctypes.create_string_buffer(512)
        doc = lib.mgms_parse(src, len(src), errbuf, 512)
        if _is_null(doc):
            raise MgmsError(errbuf.value.decode("utf-8") or "unknown error")
        self._doc = doc
        self._closed = False

    def __del__(self):
        # Releases the underlying C memory when the object is collected.
        self.close()

    def _ensure_open(self):
        # Raises if the document has already been closed.
        if self._closed or _is_null(self._doc):
            raise MgmsError("document is closed")

    def close(self):
        # Releases the underlying C memory. Safe to call more than once.
        try:
            if not self._closed and not _is_null(self._doc):
                lib.mgms_free(self._doc)
        except Exception:
            pass
        self._doc = None
        self._closed = True

    def __enter__(self):
        # Supports use as a context manager.
        return self

    def __exit__(self, *exc):
        # Closes the document when leaving a with-block.
        self.close()

    def __len__(self):
        # Number of top-level declarations.
        self._ensure_open()
        return lib.mgms_top_count(self._doc)

    def __iter__(self):
        # Iterates over (name, Value) pairs at the top level.
        self._ensure_open()
        for i in range(len(self)):
            name = _s(lib.mgms_top_name(self._doc, i))
            ptr = lib.mgms_top_value(self._doc, i)
            yield name, Value(ptr, self)

    def keys(self):
        # Returns the top-level declaration names.
        return [name for name, _ in self]

    def get(self, key):
        # Returns the top-level value with the given name, or None.
        for name, v in self:
            if name == key:
                return v
        return None

    def __getitem__(self, key):
        # Returns the top-level value with the given name.
        # Raises KeyError if the name is not present.
        v = self.get(key)
        if v is None:
            raise KeyError(key)
        return v

    def to_dict(self):
        # Converts the whole document into plain Python objects:
        # stu -> dict, vec -> list, var -> None / bool / float / str.
        return {name: _to_python(v) for name, v in self}


def _to_python(v: Value):
    # Recursively converts a Value into plain Python objects.
    if v.is_stu:
        return {k: _to_python(val) for k, val in v.items()}
    if v.is_vec:
        return [_to_python(e) for e in v]
    return v.value


def parse(src) -> Document:
    # Parses src and returns a Document.
    return Document(src)


def loads(src) -> dict:
    # Parses src and returns plain Python objects.
    return Document(src).to_dict()
