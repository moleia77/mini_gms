# mini_gms build guide

This file explains how to build the mini_gms shared and static libraries
from source with CMake, on Windows, Linux, and macOS.

- [Requirements](#requirements)
- [Build](#build)
- [Output](#output)
- [Options](#options)
- [Clean](#clean)
- [Static linking on MSVC](#static-linking-on-msvc)
- [Python binding](#python-binding)

---

## Requirements

Before building, make sure you have:

- CMake 3.15 or newer
- A C++17 compiler:
  - Windows: Visual Studio 2022 (MSVC), or MinGW
  - Linux:   GCC 7+ or Clang 5+
  - macOS:   Apple Clang (Xcode Command Line Tools)

---

## Build

The default build produces a shared library. Pick the command block that
matches your platform and toolchain.

**Windows (Visual Studio 2022):**

```bat
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

**Windows (MinGW):**

```bat
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

**Linux / macOS:**

```bash
cmake -B build
cmake --build build
```

---

## Output

All build artifacts are placed in the `lib/` directory at the repository root.

| Platform | File |
|---|---|
| Windows | `lib/mini_gms.dll` |
| Linux   | `lib/libmini_gms.so` |
| macOS   | `lib/libmini_gms.dylib` |

---

## Options

Two CMake options control what is built.

| Option | Meaning | Default |
|---|---|---|
| `MGMS_BUILD_SHARED` | build shared library | `ON` |
| `MGMS_BUILD_STATIC` | build static library | `OFF` |

Examples:

```bash
# shared library only (default)
cmake -B build

# shared + static
cmake -B build -DMGMS_BUILD_STATIC=ON

# static only
cmake -B build -DMGMS_BUILD_SHARED=OFF -DMGMS_BUILD_STATIC=ON
```

---

## Clean

Remove the build directory and start over:

```bash
rm -rf build                          # Linux / macOS
rmdir /s /q build                     # Windows CMD
Remove-Item -Recurse -Force build     # Windows PowerShell
```

---

## Static linking on MSVC

The MSVC build uses `/MT` (static runtime), so `mini_gms.dll` has no
external runtime dependencies and can be copied to any Windows machine
without installing the VC++ Redistributable.

---

## Python binding

After building the shared library, install the Python package:

```bash
cd python
pip install .
```

The Python binding uses `ctypes` from the standard library and has no
third-party runtime dependencies. See `python/` for details.