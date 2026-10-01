# _ffi.py - ctypes bindings for the mini_gms shared library.
#
# This module loads the mini_gms shared library and declares the argument
# and return types of every function in the C ABI. Other Python modules
# in this package import `lib` from here and call those functions directly.
#
# The shared library is looked up in this order:
#   1. Next to this file (the normal case after `pip install .`)
#   2. In the repository's `lib/` directory (the case when running from source)

import ctypes
import sys
from pathlib import Path
from ctypes import c_char_p, c_size_t, c_void_p, c_int, c_double


def _library_names():
    # Returns the candidate shared library file names for the current platform.
    if sys.platform == "win32":
        return ["mini_gms.dll"]
    if sys.platform == "darwin":
        return ["libmini_gms.dylib", "mini_gms.dylib"]
    return ["libmini_gms.so", "mini_gms.so"]


def _find_lib():
    # Locates the mini_gms shared library and returns its absolute path.
    # Raises FileNotFoundError listing every path that was tried.
    here = Path(__file__).resolve().parent
    names = _library_names()

    candidates = [here / n for n in names]
    root = here.parent.parent
    candidates += [root / "lib" / n for n in names]

    for p in candidates:
        if p.exists():
            return str(p)

    raise FileNotFoundError(
        "mini_gms shared library not found. Tried:\n  "
        + "\n  ".join(str(p) for p in candidates)
    )


# Loaded shared library handle used by the rest of the package.
lib = ctypes.CDLL(_find_lib())

# Parses mini_gms source text into a document handle.
lib.mgms_parse.restype = c_void_p
lib.mgms_parse.argtypes = [c_char_p, c_size_t, c_char_p, c_size_t]

# Frees a document handle returned by mgms_parse.
lib.mgms_free.restype = None
lib.mgms_free.argtypes = [c_void_p]

# Returns the number of top-level declarations.
lib.mgms_top_count.restype = c_size_t
lib.mgms_top_count.argtypes = [c_void_p]

# Returns the name of a top-level declaration.
lib.mgms_top_name.restype = c_char_p
lib.mgms_top_name.argtypes = [c_void_p, c_size_t]

# Returns the value node of a top-level declaration.
lib.mgms_top_value.restype = c_void_p
lib.mgms_top_value.argtypes = [c_void_p, c_size_t]

# Type checks for a value node.
lib.mgms_is_stu.restype = c_int
lib.mgms_is_stu.argtypes = [c_void_p]

lib.mgms_is_vec.restype = c_int
lib.mgms_is_vec.argtypes = [c_void_p]

lib.mgms_is_var.restype = c_int
lib.mgms_is_var.argtypes = [c_void_p]

# stu member access.
lib.mgms_stu_count.restype = c_size_t
lib.mgms_stu_count.argtypes = [c_void_p]

lib.mgms_stu_key.restype = c_char_p
lib.mgms_stu_key.argtypes = [c_void_p, c_size_t]

lib.mgms_stu_value.restype = c_void_p
lib.mgms_stu_value.argtypes = [c_void_p, c_size_t]

lib.mgms_stu_get.restype = c_void_p
lib.mgms_stu_get.argtypes = [c_void_p, c_char_p]

# vec element access.
lib.mgms_vec_count.restype = c_size_t
lib.mgms_vec_count.argtypes = [c_void_p]

lib.mgms_vec_at.restype = c_void_p
lib.mgms_vec_at.argtypes = [c_void_p, c_size_t]

# var type checks.
lib.mgms_var_is_null.restype = c_int
lib.mgms_var_is_null.argtypes = [c_void_p]

lib.mgms_var_is_bool.restype = c_int
lib.mgms_var_is_bool.argtypes = [c_void_p]

lib.mgms_var_is_number.restype = c_int
lib.mgms_var_is_number.argtypes = [c_void_p]

lib.mgms_var_is_string.restype = c_int
lib.mgms_var_is_string.argtypes = [c_void_p]

# var payload access.
lib.mgms_var_bool.restype = c_int
lib.mgms_var_bool.argtypes = [c_void_p]

lib.mgms_var_number.restype = c_double
lib.mgms_var_number.argtypes = [c_void_p]

lib.mgms_var_string.restype = c_char_p
lib.mgms_var_string.argtypes = [c_void_p]


def _s(ptr):
    # Decodes a C string pointer to a Python str, or returns None if NULL.
    return ptr.decode("utf-8") if ptr else None


def _is_null(ptr):
    # Returns True if the given ctypes pointer is NULL.
    return ptr is None or ptr == 0