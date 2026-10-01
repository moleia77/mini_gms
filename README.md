[English](README.md) | [中文](README_CN.md)

# mini_gms

A lightweight, parser-friendly configuration language.

mini_gms is the data-configuration subset split out while building **GMS**, and plays a role similar to JSON.

![version](https://img.shields.io/badge/version-1.0.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![language](https://img.shields.io/badge/C%2B%2B-17-blue)

---

## Table of Contents

- [About](#about)
- [Syntax](#syntax)
- [Interfaces](#interfaces)
- [Build](#build)
- [Design Notes](#design-notes)
- [Contributing](#contributing)
- [License](#license)

---

## About

mini_gms is a configuration language with three data types, a minimal symbol set, and a top-level tag on every declaration.

- **Three data types**: `[stu]` (struct), `[vec]` (array), `[var]` (scalar)
- **Minimal symbols**: no commas, no semicolons, unquoted keys
- **Tagged top level**: `[stu] name { ... }` — the entry point carries both type and name
- **Comments**: `#` line comments
- **Multi-language**: native C++, C ABI, Python binding

mini_gms source files are usually saved with the `.mgms` extension, for example `config.mgms`.

### About GMS

GMS (Game Markup Script) is a DSL scripting language for game engines, currently in development and not released yet.
mini_gms is its data-configuration subset, split out so it can be used on its own.

### Example

```
[stu] app {
    name : "MyApp"
    version : "1.0.0"

    window {
        width : 1280
        height : 720
        resizable : true
    }

    servers : {
        { host : "alpha.example.com"  port : 8080 }
        { host : "beta.example.com"   port : 8080 }
    }
}
```

C++:

```cpp
#include "mini_gms.hpp"

auto doc = mol::Document::parse(text);

auto name = doc.get<mol::string>("app.name", "");            // "MyApp"
auto w    = doc.get<mol::number>("app.window.width", 0.0);   // 1280
auto host = doc.get<mol::string>("app.servers[0].host", ""); // "alpha.example.com"
```

Python:

```python
import mini_gms

data = mini_gms.loads(text)
print(data['app']['name'])               # MyApp
print(data['app']['window']['width'])    # 1280.0
print(data['app']['servers'][0]['host']) # alpha.example.com
```

---

## Syntax

### Three data types

mini_gms has exactly three data types. The type is determined by two symbols:

| Type | Marker | Form |
|---|---|---|
| `stu` | `{}`, no colon | `[stu] name { ... }` |
| `vec` | `:{}`, colon | `[vec] name : { ... }` |
| `var` | `:`, colon only | `[var] name : value` |

```
name { ... }        → stu
name : { ... }      → vec
name : value        → var
```

- colon, no braces → `var`
- colon, braces → `vec`
- no colon, braces → `stu`

### Top level requires a tag

```
[stu] config { ... }     ✅
config { ... }           ❌
```

The top level has no enclosing scope, so the parser relies on the tag to determine the type. Even a single-line file must carry a tag:

```
[var] a : 10
```

### Nested declarations may omit the tag

Inside a `stu`, the tag may be omitted because the form already implies the type:

```
[stu] config {
    name : "test"          # var
    window { ... }         # stu
    nums : { 1 2 3 }       # vec
}
```

### Separators

No commas, no semicolons. Elements are separated by whitespace (spaces, newlines, tabs).

```
[vec] numbers : { 1 2 3 }
```

is equivalent to:

```
[vec] numbers : {
    1
    2
    3
}
```

### Scalar types

| Type | Form | Examples |
|---|---|---|
| string | double quotes | `"hello"` |
| number | integer or float | `100`, `-5`, `7.5` |
| boolean | keyword | `true`, `false` |
| null | keyword | `null` |

Integers and floats are both represented as `double`.

### Nesting

**stu inside stu**:

```
[stu] game {
    window {
        title : "test"
        size {
            width : 1280
            height : 720
        }
    }
}
```

**vec of scalars**:

```
[vec] numbers : { 1 2 3 4 5 }
```

**vec of anonymous stu**:

```
[vec] cards : {
    { id : 1  name : "first" }
    { id : 2  name : "second" }
}
```

**vec of anonymous sub-vec** (multi-dimensional array):

```
[vec] matrix : {
    : { 1 2 3 }
    : { 4 5 6 }
    : { 7 8 9 }
}
```

The `: { ... }` form is an anonymous sub-`vec`. It only appears in multi-dimensional arrays; elsewhere `:` is always followed by a member name.

### Comments

`#` line comments, allowed anywhere:

```
# full-line comment
[stu] config {
    width : 1280    # trailing comment
    height : 720
}
```

`#` inside a string is not a comment:

```
color : "#FF0000"    # this is the comment
```

### Full example

```
# application configuration
[stu] config {
    title : "MyApp"
    version : "1.0.0"
    debug : true

    window {
        width : 1280
        height : 720
        fullscreen : false
    }

    paths {
        audio : "res/audio/"
        image : "res/img/"
    }
}

[stu] entities {
    player {
        name : "Alice"
        hp : 100
        attack {
            damage : 20
            interval : 1.4
        }
    }
    dummy {
        name : "Training Dummy"
        hp : 9999
        attack : null
    }
}

[vec] numbers : { 1 2 3 4 5 }

[vec] cards : {
    { id : 1  name : "first"   cost : 100 }
    { id : 2  name : "second"  cost : 50 }
}

[var] screenWidth : 1280
[var] appTitle : "MyApp"
[var] nothing : null
```

---

## Interfaces

### C++

```cpp
#include "mini_gms.hpp"

using namespace mol;

// parse
auto doc = Document::parse(text);

// scalar access by path
auto name = doc.get<string>("config.title", "");
auto w    = doc.get<number>("config.window.width", 0.0);
auto dbg  = doc.get<boolean>("config.debug", false);

// array index
auto first = doc.get<number>("config.nums[0]", 0.0);

// required value (throws if missing or type mismatch)
auto required = doc.require<number>("config.window.width");

// existence check
if (doc.has("config.title")) { ... }

// node access for manual traversal
auto nums = doc.node("config.nums");
if (nums && nums->isVec()) {
    for (auto& e : nums->asVec()) { ... }
}
```

Path syntax:

| Path | Meaning |
|---|---|
| `config.title` | `title` of top-level `config` |
| `config.nums[0]` | element 0 of `nums` |
| `config.players[2].name` | `name` of player 2 |

Type aliases:

```cpp
mol::null      // std::monostate
mol::boolean   // bool
mol::number    // double
mol::string    // std::string
```

### Python

```bash
cd python
pip install .
```

```python
import mini_gms

# parse to dict
data = mini_gms.loads(text)
print(data['config']['title'])          # MyApp
print(data['config']['window']['width']) # 1280.0

# keep Document for on-demand access
with mini_gms.parse(text) as doc:
    cfg = doc['config']
    print(cfg['title'].value)           # MyApp
    print(cfg.keys())                   # ['title', 'version', 'debug', ...]
```

Type mapping:

| mini_gms | Python |
|---|---|
| `stu` | `dict` |
| `vec` | `list` |
| `null` | `None` |
| `bool` | `bool` |
| `number` | `float` |
| `string` | `str` |

### C ABI

Any language with a C FFI can use the shared library.

```c
#include "mini_gms_c.h"

char errbuf[512];
mgms_doc* doc = mgms_parse(src, len, errbuf, 512);
if (!doc) { fprintf(stderr, "%s\n", errbuf); return; }

size_t n = mgms_top_count(doc);
for (size_t i = 0; i < n; ++i) {
    const char* name = mgms_top_name(doc, i);
    const mgms_value* v = mgms_top_value(doc, i);
    // ...
}
mgms_free(doc);
```

See `include/mini_gms_c.h` for the full function list.

---

## Build

See [BUILD.txt](BUILD.txt).

---

## Design Notes

### Why the top level requires a tag

A parser needs a deterministic entry point. JSON uses `{}` at the top level, but `{}` carries no type information. mini_gms uses `[stu] name` — the same position provides both the type and the name.

Nested declarations may omit the tag because the enclosing scope already determines the type. Inside a `stu`, the form (`{}` / `:{}` / `:`) is enough.

### Why commas and quotes are unnecessary

- **No commas**: whitespace separates elements
- **Unquoted keys**: the parser can locate the key boundary by position
- **Tagged top level**: the entry point carries the type

**Anything the parser can infer is left out of the syntax.**

### Comparison with JSON

| | JSON | mini_gms |
|---|---|---|
| Top level | `{ ... }` (anonymous container) | `[stu] name { ... }` (type + name) |
| Keys | must be quoted | unquoted |
| Separators | commas | whitespace |
| Comments | none | `#` line comments |
| Top-level scalar | must be wrapped in `{}` | `[var] a : 10` on one line |

mini_gms syntax comes from GMS, not designed as a JSON alternative.

---

## Contributing

Open source projects are a picture of collaboration and generosity. Please use the links below, submit a pull request, or reach out by email.

[Request Feature][feature-request-url]<br>
[Report Bug][bug-report-url]

---

## License

[![License][license-shield]][license-url]

MIT License. See [LICENSE.txt](LICENSE.txt).

---

[feature-request-url]: https://github.com/moleia77/mini_gms/issues/new?labels=enhancement&template=feature-request.md
[bug-report-url]: https://github.com/moleia77/mini_gms/issues/new?labels=bug&template=bug-report.md

[license-shield]: https://img.shields.io/github/license/moleia77/mini_gms.svg?style=for-the-badge
[license-url]: https://github.com/moleia77/mini_gms/blob/main/LICENSE.txt