[English](README.md) | [中文](README_CN.md)

# mini_gms

A lightweight, parser-friendly data configuration language.

mini_gms was split out of **GMS** (a DSL for game engines) as its data-configuration subset. It plays a role similar to JSON, but targets data that is generated and parsed by machines far more often than it is read by humans: it removes everything the parser can infer, and adds the constructs that large configs actually need — reuse, sequences, and compact tabular rows. Game data was the original motivation; the language itself is general.

![version](https://img.shields.io/badge/version-1.1.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![language](https://img.shields.io/badge/language-C%2B%2B17-blue)

---

## Table of Contents

- [About](#about)
  - [What mini_gms is](#what-mini_gms-is)
  - [What mini_gms is not](#what-mini_gms-is-not)
  - [Feature overview](#feature-overview)
  - [Example](#example)
- [Quick Start](#quick-start)
- [Syntax Tutorial](#syntax-tutorial)
  - [1. The three data types](#1-the-three-data-types)
  - [2. Top-level declarations need a tag](#2-top-level-declarations-need-a-tag)
  - [3. Nested declarations may omit the tag](#3-nested-declarations-may-omit-the-tag)
  - [4. Separators and comments](#4-separators-and-comments)
  - [5. Scalars](#5-scalars)
  - [6. Strings: raw vs escaped](#6-strings-raw-vs-escaped)
  - [7. Nesting](#7-nesting)
  - [8. Reuse with macros](#8-reuse-with-macros)
  - [9. Range expansion](#9-range-expansion)
  - [10. Batch declarations](#10-batch-declarations)
  - [11. Compact stu rows (template inheritance)](#11-compact-stu-rows-template-inheritance)
  - [12. Putting it together](#12-putting-it-together)
- [Interfaces](#interfaces)
  - [C++](#c)
  - [Python](#python)
  - [C ABI](#c-abi)
- [Build](#build)
- [Design Notes](#design-notes)
- [Compatibility](#compatibility)
- [License](#license)

---

## About

### What mini_gms is

A configuration language with exactly three data types, a minimal symbol set, and a small number of powerful constructs for reuse and compaction.

- **Three data types**: `stu` (struct), `vec` (array), `var` (scalar).
- **Minimal symbols**: no commas, no semicolons, unquoted keys.
- **Tagged top level**: `[stu] name { ... }` — the entry point carries both the type and the name.
- **Reuse**: `&name` macros and `start~end` ranges.
- **Compaction**: batch declarations and compact `stu` rows.
- **Multi-language**: native C++, a stable C ABI, and a Python binding.

Source files use the `.mgms` extension, for example `game.mgms`.

### What mini_gms is not

- It is not a JSON alternative for human-edited, cross-system interchange. JSON optimizes for "read once, understand immediately"; mini_gms optimizes for data density and machine generation.
- It is not a programming language. There are no expressions, no control flow, no functions. Values are literals, macros, or ranges.
- It does not carry a schema. `stu` / `vec` / `var` are inferred from the form, not declared.

Beyond the three data types, the reader only has to remember one rule: `&name` is a single identifier with two roles — it records a value where a declaration is expected, and it substitutes that value where a value is expected. Editor syntax highlighting makes this visible; the rest of the language is form-driven.

### Feature overview

| Feature | Since | Example |
|---|---|---|
| Three data types `stu` / `vec` / `var` | 1.0 | `[stu] a { b : 1 }` |
| Tagged top level | 1.0 | `[vec] n : { 1 2 3 }` |
| Untagged nested declarations | 1.0 | `b { c : 1 }` inside a `stu` |
| `#` line comments | 1.0 | `width : 1280  # px` |
| Raw double-quoted strings | 1.0 | `"C:\path"` |
| Escaped single-quoted strings | 1.1 | `'line1\nline2'` |
| Macros `&name` | 1.1 | `[var] &base : 100`, then `x : &base` |
| Range expansion `start~end` | 1.1 | `{ 1~5 }` |
| Batch declarations `[stu] { ... }` | 1.1 | `[stu] { a : 1  b : 2 }` |
| Compact `stu` rows | 1.1 | `{ 1 2 }` after a template row |
| Path access (`a.b[0].c`) | 1.0 | `doc.get<number>("a.b[0].c")` |
| C++ / C ABI / Python bindings | 1.0 | see [Interfaces](#interfaces) |

### Example

A small game config. It is not a realistic production file — it is deliberately packed so that one screen shows every feature of the language.

```
# a macro: records the name &hero_defaults, referenced later
[stu] &hero_defaults { hp : 100  speed : 1.5 }

# a batch declaration: no name, members inferred from their form
[stu] {
    title   : "Starfall"                    # var (raw string)
    tagline : 'a tiny\nspace adventure'     # var (escaped string, spans lines)

    screen {                                # stu (nested)
        width  : 800
        height : 600
    }

    levels : { 1~5 }                        # vec (range: 1 2 3 4 5)

    enemies : {                             # vec of stu
        { name : "Slime"  hp : 20 }         # template row
        { "Bat"   10 }                      # compact row
        { "Ghost" 30 }                      # compact row
    }

    hero {                                  # stu, reusing the macro
        stats : &hero_defaults              # substitutes the stu above
        name  : "Nova"
    }
}
```

Reading it top to bottom:

- `[stu] &hero_defaults { ... }` — a macro declaration. The tag `[stu]` gives the type, `&hero_defaults` is the name; it is recorded here and substituted later.
- `[stu] { ... }` — a batch declaration. No name; each member's type is inferred from its form (`: value` → var, `{ ... }` → stu, `: { ... }` → vec).
- `tagline : '...'` — an escaped string. `\n` becomes a newline, and the string may span lines.
- `levels : { 1~5 }` — a range. Expands to `{ 1 2 3 4 5 }` at parse time.
- `enemies : { ... }` — a vec whose first row is a full `stu` template; later rows are compact and filled against it.
- `stats : &hero_defaults` — a macro reference. Yields the same node, not a copy.

C++:

```cpp
#include "mini_gms.hpp"

auto doc = mol::Document::parse(text);

auto title  = doc.get<mol::string>("title", "");              // "Starfall"
auto levels = doc.getVec("levels");                           // [1,2,3,4,5]
auto hp     = doc.get<mol::number>("enemies[0].hp", 0.0);     // 20
auto stats  = doc.node("hero.stats");                         // the shared stu
```

Python:

```python
import mini_gms

data = mini_gms.loads(text)
print(data['title'])              # Starfall
print(data['levels'])             # [1.0, 2.0, 3.0, 4.0, 5.0]
print(data['enemies'][0]['hp'])   # 20.0
print(data['hero']['stats'])      # {'hp': 100.0, 'speed': 1.5}
```

---

## Quick Start

A minimal file, then the same file built up step by step.

```
[var] a : 10
```

That is a complete mini_gms file: one top-level scalar declaration. Every feature in this document is an addition to this baseline.

---

## Syntax Tutorial

### 1. The three data types

mini_gms has exactly three data types. The form alone determines which one you get:

| Type | Marker | Form | Meaning |
|---|---|---|---|
| `stu` | `{}`, no colon | `name { ... }` | ordered list of named members |
| `vec` | `:{}`, colon | `name : { ... }` | ordered list of values |
| `var` | `:`, colon only | `name : value` | scalar (null, bool, number, string) |

- colon, no braces → `var`
- colon, braces → `vec`
- no colon, braces → `stu`

`stu` preserves declaration order and allows duplicate keys; lookup returns the first match. This is intentional: mini_gms does not impose a map semantics where the source does not ask for one.

### 2. Top-level declarations need a tag

The top level has no enclosing scope, so the parser needs the tag to know the type:

```
[stu] config { ... }     ✅
config { ... }           ❌  (no tag at top level)
```

Even a one-line file must carry a tag:

```
[var] a : 10
```

The tag is also where the declaration name lives: `[stu] name { ... }` reads as "a stu named name", not "a tag followed by a name".

### 3. Nested declarations may omit the tag

Inside a `stu`, the form already implies the type, so the tag can be dropped:

```
[stu] config {
    name   : "test"      # var
    window { ... }       # stu
    nums   : { 1 2 3 }   # vec
}
```

The same three forms apply; only the `[tag]` prefix is optional.

### 4. Separators and comments

There are no commas and no semicolons. Elements are separated by whitespace: spaces, tabs, newlines. These two are equivalent:

```
[vec] numbers : { 1 2 3 }
```

```
[vec] numbers : {
    1
    2
    3
}
```

`#` starts a line comment that runs to the end of the line. It may appear on its own line or after any token. A `#` inside a string is not a comment.

```
# full-line comment
[stu] config {
    width : 1280    # trailing comment
    color : "#FF0000"   # this # is part of the string
}
```

### 5. Scalars

| Type | Form | Examples |
|---|---|---|
| null | keyword | `null` |
| boolean | keyword | `true`, `false` |
| number | optional `-`, digits, optional `.` | `100`, `-5`, `7.5` |
| string | double or single quotes | `"hello"`, `'line\n'` |

Integers and floats are both stored as `double`. There is no separate integer type.

### 6. Strings: raw vs escaped

Two string forms, with different escape rules:

- `"..."`: raw string. No escapes are processed, so `\n` is a backslash followed by `n`. **Cannot span lines.**
- `'...'`: escaped string. Processes `\n`, `\t`, `\r`, `\'`, `\\`. **Can span multiple lines.**

```
[var] win_path : "C:\Users\test"    # raw: the backslashes are literal
[var] msg      : 'line1\nline2'     # escaped: \n becomes a newline
[var] multi    : 'first
second'                             # escaped strings may span lines
```

Choose the raw form for Windows paths, regexes, and anything where backslashes are meaningful. Choose the escaped form when you want `\n`, `\t`, or multi-line text without concatenation.

### 7. Nesting

The three types nest freely.

**stu inside stu:**

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

**vec of scalars:**

```
[vec] numbers : { 1 2 3 4 5 }
```

**vec of anonymous stu:**

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

The `: { ... }` form is an anonymous sub-`vec`. It only appears as a `vec` element; elsewhere `:` is always followed by a member name.

### 8. Reuse with macros

`&name` is a single identifier: the leading `&` is part of the name, not an operator, and introduces no new rules. **A macro declaration is just an ordinary declaration whose name starts with `&`**; tag rules are unchanged — a tag is required at the top level, and may be omitted inside a `stu`.

The same name has two roles, decided by where it appears:

- **Declaration position**: records this value. At the top level it is written `[tag] &name ...`; inside a `stu` it is written `&name ...`.
- **Value position**: the right side of `:`, or an element inside a `vec`. The recorded value is substituted.

At the top level:

```
[var] &base_hp : 100              # declaration: records &base_hp
[var] player_hp : &base_hp        # value position: substitutes 100
```

Inside a `stu` (tag may be omitted, as with any member):

```
[stu] config {
    &base { hp : 100  speed : 1.0 }

    [stu] enemy {
        stats : &base           # substitutes the stu recorded above
        copy  : &base           # the same node, not a copy
    }
}
```

Rules:

- A reference yields the **same node**, not a copy. If mutation is ever added, changes in one place would be visible everywhere.
- Macros must be defined before they are referenced. A forward reference raises `undefined macro`.
- Redefining a name overwrites the previous definition.
- Macros may reference other macros, as long as each is defined before use. A self-reference (`&a { x : &a }`) is undefined at the point of use and therefore raises `undefined macro`.

### 9. Range expansion

Inside a `vec`, `start~end` expands to every integer from `start` to `end`, inclusive. The step is `+1` when `start <= end` and `-1` otherwise.

```
[vec] up   : { 1~5 }      # 1 2 3 4 5
[vec] down : { 5~1 }      # 5 4 3 2 1
[vec] neg  : { -3~3 }     # -3 -2 -1 0 1 2 3
[vec] same : { 3~3 }      # 3
```

Range expansion only happens inside a `vec`. `~` has no meaning elsewhere and raises a syntax error.

### 10. Batch declarations

A `[stu] { ... }` block without a name declares several members at once. Each member's type is inferred by its form, using the same rules as inside a `stu`.

```
[stu] {
    a : 1          # var
    b { c : 2 }    # stu
    d : { 1 2 3 }  # vec
}
```

This is exactly equivalent to writing:

```
[var] a : 1
[stu] b { c : 2 }
[vec] d : { 1 2 3 }
```

A batch block may appear at the top level or inside a `stu`. Only `[stu]` is accepted as a batch tag; `[vec] { ... }` and `[var] { ... }` raise an error, because there is no meaningful way to infer which of several values a batch `var` would produce.

### 11. Compact stu rows (template inheritance)

Inside a `vec`, the first complete `stu` becomes the template. Later rows that start with a scalar are filled against that template, in order.

```
[vec] enemies : {
    { name : "Slime"  hp : 20 }   # template
    { "Bat"   10 }                # { name : "Bat"    hp : 10 }
    { "Ghost" 30 }                # { name : "Ghost"  hp : 30 }
}
```

Rules:

- The number of values must match the template exactly.
- Each value must be a scalar (number, string, bool, or null). Nested `{}` and `:` are not allowed in a compact row.
- A compact row before any template raises an error: there is nothing to fill against.
- The template is the first full `stu` in that `vec`. If you need a different order, declare the template first.

This is the construct that large configs need most: a thousand entities with the same shape, written as a thousand short rows instead of a thousand repeated key names.

### 12. Putting it together

The About example shows every construct once. The example below is a different scenario, focused on **how the constructs combine**: a macro referenced from several places, a batch declaration nested inside a `stu`, and ranges inside a nested `vec`.

```
# a shared "attributes" macro, referenced by the player and the enemy
[stu] &attributes { str : 10  dex : 8  int : 6 }

# a shared "empty slots" macro, reused in several places
[vec] &empty_slots : { null  null  null  null }

[stu] save {
    player {
        attributes : &attributes        # reference to the shared stu
        inventory  : &empty_slots       # reference to the shared vec
        name       : "Nova"
    }

    # batch declaration nested inside a stu: no name, member types inferred
    [stu] {
        gold    : 1200
        chapter : "prologue"
        checked : 'the gate\nis open'   # escaped string, spans lines
    }

    # a vec whose first row is a template, followed by compact rows
    party : {
        { name : "Nova"   level : 1  hp : 30 }
        { "Iris"   2  25 }
        { "Kael"   3  40 }
    }

    # nested vec: each chapter is a sequence of quest ids
    quests : {
        : { 1~3 }       # chapter 1: 1 2 3
        : { 4~6 }       # chapter 2: 4 5 6
    }
}
```

In this file:

- `&attributes` and `&empty_slots` are **two macros of different types** — one `stu`, one `vec` — showing that macros are not limited to one type.
- `player.attributes` and `player.inventory` are **macro references in value position**; the first substitutes a `stu`, the second a `vec`.
- `[stu] { ... }` is a **batch declaration nested inside a `stu`**, expanded into three members: `gold`, `chapter`, `checked`.
- The compact rows in `party` show that template inheritance is not limited to the top-level `vec`; any `vec` can use it.
- `quests` combines **nested vec and range expansion**: each element of the outer `vec` is a sub-`vec`, and each sub-`vec` is a `1~3` range.

C++:

```cpp
auto doc = mol::Document::parse(text);

auto str  = doc.get<mol::number>("save.player.attributes.str", 0.0); // 10
auto gold = doc.get<mol::number>("save.gold", 0.0);                  // 1200
auto q    = doc.get<mol::number>("save.quests[0][1]", 0.0);          // 2
```

Python:

```python
data = mini_gms.loads(text)
print(data['save']['player']['attributes'])  # {'str': 10.0, 'dex': 8.0, 'int': 6.0}
print(data['save']['gold'])                  # 1200.0
print(data['save']['party'][1]['name'])      # Iris
print(data['save']['quests'][0])             # [1.0, 2.0, 3.0]
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
auto title = doc.get<string>("title", "");
auto hp    = doc.get<number>("enemies[0].hp", 0.0);

// array index
auto first = doc.get<number>("levels[0]", 0.0);

// required value (throws if missing or type mismatch)
auto required = doc.require<number>("screen.width");

// existence check
if (doc.has("title")) { ... }

// node access for manual traversal
auto enemies = doc.node("enemies");
if (enemies && enemies->isVec()) {
    for (auto& e : enemies->asVec()) { ... }
}
```

Path syntax:

| Path | Meaning |
|---|---|
| `title` | top-level `title` |
| `levels[0]` | element 0 of `levels` |
| `enemies[2].name` | `name` of the third enemy |
| `hero.stats.hp` | `hp` inside the shared macro node |

Type aliases:

```cpp
mol::null      // std::monostate
mol::boolean   // bool
mol::number    // double
mol::string    // std::string
```

### Python

The Python package is a `ctypes` wrapper around the shared library. The shared library is not committed to the repository; see [BUILD.md](BUILD.md) for the build-and-copy steps before `pip install .`.

```bash
cd python
pip install .
```

```python
import mini_gms

# parse to plain Python objects
data = mini_gms.loads(text)
print(data['title'])              # Starfall
print(data['levels'])             # [1.0, 2.0, 3.0, 4.0, 5.0]
print(data['enemies'][0]['hp'])   # 20.0
print(data['hero']['stats'])      # {'hp': 100.0, 'speed': 1.5}

# keep a Document for on-demand access
with mini_gms.parse(text) as doc:
    hero = doc['hero']
    print(hero['name'].value)     # Nova
    print(hero.keys())            # ['stats', 'name']
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

See [BUILD.md](BUILD.md).

---

## Design Notes

**Why the top level requires a tag.** A parser needs a deterministic entry point. JSON uses `{}` at the top level, but `{}` carries no type information. mini_gms puts the type and the name in the same position: `[stu] name`. Nested declarations can drop the tag because the enclosing scope already determines the type.

**Why commas and quotes are unnecessary.** Whitespace separates elements; keys are bare; the top-level tag carries the type. Anything the parser can infer is left out of the syntax. This is a deliberate trade: fewer characters to write and parse, at the cost of a reader needing to know the inference rules.

**Why macros use one `&name` form for both roles.** `&` is just the first character of the name; it introduces no new rules. A macro declaration is an ordinary declaration whose name starts with `&`; tag rules, position semantics, and lookup all match ordinary declarations. A separate pair of symbols — YAML's `&` for definition and `*` for reference — was considered and rejected. It would add a second concept for a feature that fits in one rule: `&name` is a name; the position decides what it does. Syntax highlighting makes the two roles visible without a second symbol.

**Why batch declarations and compact rows exist.** Large configs contain many same-shaped entities. Writing `name : ..., hp : ...` ten thousand times is a waste of both file size and human attention. Batch declarations remove the repeated `[var]` / `[stu]` tags; compact rows remove the repeated key names. Both are opt-in: the full form is always available when a file is clearer written out.

**Why `stu` keeps order and allows duplicate keys.** The source is ordered, and the model reflects that. If a later feature needs map semantics, it can be built on top; the model does not impose it prematurely.

**Why numbers are `double`.** One numeric type keeps the value model small and the C ABI simple. Configs rarely need integer/float distinction; when they do, the consumer can convert.

---

## Compatibility

mini_gms 1.1 is fully backward compatible with 1.0:

- 1.0 `.mgms` files remain valid and keep the same meaning.
- The C++ API (`mol::Document`, `mol::parse`), the C ABI (`mgms_*`), and the Python API (`parse`, `loads`, `Document`, `Value`) are unchanged.
- No migration is required.

1.1 adds new syntax (macros, ranges, escaped strings, batch declarations, compact rows) but does not change or remove any 1.0 construct. See [CHANGELOG.md](CHANGELOG.md) for the full list.

---

## License

MIT License. See [LICENSE.txt](LICENSE.txt).
