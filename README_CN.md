[English](README.md) | [中文](README_CN.md)

# mini_gms

一种轻量、解析友好的数据配置语言。

mini_gms 是在开发 **GMS**（面向游戏引擎的 DSL）的过程中拆分出的数据配置子集。它承担类似于 JSON 的职责，但面向的是「由机器生成、由机器解析远多于由人阅读」的数据：它删掉了所有解析器能推断的东西，并补上了大型配置真正需要的构件——复用、序列和紧凑的表格行。游戏数据是最初的动机；语言本身是通用的。

![version](https://img.shields.io/badge/version-1.1.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![language](https://img.shields.io/badge/language-C%2B%2B17-blue)

---

## 目录

- [关于](#关于)
  - [mini_gms 是什么](#mini_gms-是什么)
  - [mini_gms 不是什么](#mini_gms-不是什么)
  - [特性总览](#特性总览)
  - [示例](#示例)
- [快速开始](#快速开始)
- [语法教程](#语法教程)
  - [1. 三种数据类型](#1-三种数据类型)
  - [2. 顶层声明必须带标签](#2-顶层声明必须带标签)
  - [3. 内层声明可省标签](#3-内层声明可省标签)
  - [4. 分隔与注释](#4-分隔与注释)
  - [5. 标量](#5-标量)
  - [6. 字符串：原始与转义](#6-字符串原始与转义)
  - [7. 嵌套](#7-嵌套)
  - [8. 用宏复用](#8-用宏复用)
  - [9. 范围展开](#9-范围展开)
  - [10. 批量声明](#10-批量声明)
  - [11. 紧凑 stu 行（模板继承）](#11-紧凑-stu-行模板继承)
  - [12. 综合示例](#12-综合示例)
- [各语言接口](#各语言接口)
  - [C++](#c)
  - [Python](#python)
  - [C ABI](#c-abi)
- [构建](#构建)
- [设计说明](#设计说明)
- [兼容性](#兼容性)
- [许可证](#许可证)

---

## 关于

### mini_gms 是什么

一种配置语言，只有三种数据类型、极少的符号，以及少量但有力的复用与压缩构件。

- **三种数据**：`stu`（结构体）、`vec`（数组）、`var`（标量）。
- **符号极少**：无逗号、无分号、键裸写。
- **顶层带标签**：`[stu] name { ... }`——入口同时携带类型和名字。
- **复用**：`&name` 宏和 `start~end` 范围。
- **压缩**：批量声明和紧凑 `stu` 行。
- **多语言**：原生 C++、稳定的 C ABI、Python 绑定。

源文件使用 `.mgms` 扩展名，例如 `game.mgms`。

### mini_gms 不是什么

- 不是面向人工编辑、跨系统交换的 JSON 替代品。JSON 优化的是「读一次就懂」；mini_gms 优化的是数据密度和机器生成。
- 不是编程语言。没有表达式、没有控制流、没有函数。值只有字面量、宏和范围三种来源。
- 不携带 schema。`stu` / `vec` / `var` 由形式推断，不需要声明。

除了三种数据类型之外，读者只需要再记住一条规则：`&name` 是一个整体标识符，有两种角色——出现在声明位置时登记一个值，出现在值位置时替换为该值。编辑器语法高亮会让这一点可见；语言的其余部分是纯形式驱动的。

### 特性总览

| 特性 | 起始版本 | 例子 |
|---|---|---|
| 三种数据类型 `stu` / `vec` / `var` | 1.0 | `[stu] a { b : 1 }` |
| 顶层带标签 | 1.0 | `[vec] n : { 1 2 3 }` |
| 内层省标签 | 1.0 | `stu` 内的 `b { c : 1 }` |
| `#` 行注释 | 1.0 | `width : 1280  # px` |
| 双引号原始字符串 | 1.0 | `"C:\path"` |
| 单引号转义字符串 | 1.1 | `'line1\nline2'` |
| 宏 `&name` | 1.1 | `[var] &base : 100`，然后 `x : &base` |
| 范围展开 `start~end` | 1.1 | `{ 1~5 }` |
| 批量声明 `[stu] { ... }` | 1.1 | `[stu] { a : 1  b : 2 }` |
| 紧凑 `stu` 行 | 1.1 | 模板行之后的 `{ 1 2 }` |
| 路径访问（`a.b[0].c`） | 1.0 | `doc.get<mol::mgms_num>("a.b[0].c")` |
| C++ / C ABI / Python 绑定 | 1.0 | 见[各语言接口](#各语言接口) |

### 示例

一段小游戏配置。它不是真实的生产文件——它被刻意塞满，为的是展示语言的每一个特性。

```
# 一个宏：登记名字 &hero_defaults，后面会引用
[stu] &hero_defaults { hp : 100  speed : 1.5 }

# 一个批量声明：没有名字，成员类型由形式推断
[stu] {
    title   : "Starfall"                    # var（原始字符串）
    tagline : 'a tiny\nspace adventure'     # var（转义字符串，可跨行）

    screen {                                # stu（嵌套）
        width  : 800
        height : 600
    }

    levels : { 1~5 }                        # vec（范围：1 2 3 4 5）

    enemies : {                             # vec of stu
        { name : "Slime"  hp : 20 }         # 模板行
        { "Bat"   10 }                      # 紧凑行
        { "Ghost" 30 }                      # 紧凑行
    }

    hero {                                  # stu，复用宏
        stats : &hero_defaults              # 替换为上面登记的 stu
        name  : "Nova"
    }
}
```

自上而下读：

- `[stu] &hero_defaults { ... }`——一个宏声明。标签 `[stu]` 给出类型，`&hero_defaults` 是名字；在这里登记，后面被替换。
- `[stu] { ... }`——批量声明。没有名字；每个成员的类型由形式推断（`: 值` → var，`{ ... }` → stu，`: { ... }` → vec）。
- `tagline : '...'`——转义字符串。`\n` 变成换行，字符串可以跨行。
- `levels : { 1~5 }`——范围。解析时展开为 `{ 1 2 3 4 5 }`。
- `enemies : { ... }`——一个 `vec`，第一行是完整的 `stu` 模板；后续行是紧凑行，按模板补齐。
- `stats : &hero_defaults`——宏引用。得到的是同一个节点，不是拷贝。

C++：

```cpp
#include "mini_gms.hpp"

auto doc = mol::MgmsDoc::parse(text);

auto title  = doc.get<mol::mgms_str>("title", "");              // "Starfall"
auto levels = doc.getVec("levels");                             // [1,2,3,4,5]
auto hp     = doc.get<mol::mgms_num>("enemies[0].hp", 0.0);     // 20
auto stats  = doc.node("hero.stats");                           // 共享的 stu
```

Python：

```python
import mini_gms

data = mini_gms.loads(text)
print(data['title'])              # Starfall
print(data['levels'])             # [1.0, 2.0, 3.0, 4.0, 5.0]
print(data['enemies'][0]['hp'])   # 20.0
print(data['hero']['stats'])      # {'hp': 100.0, 'speed': 1.5}
```

---

## 快速开始

一个最小的文件，然后在此基础上逐节叠加。

```
[var] a : 10
```

这就是一个完整的 mini_gms 文件：一条顶层标量声明。本文档后面的每一个特性，都是在这个基线上做加法。

---

## 语法教程

### 1. 三种数据类型

mini_gms 只有三种数据类型，仅凭形式就能决定是哪一种：

| 类型 | 标记 | 形式 | 含义 |
|---|---|---|---|
| `stu` | `{}`，无冒号 | `name { ... }` | 有序的命名成员列表 |
| `vec` | `:{}`，有冒号 | `name : { ... }` | 有序的值列表 |
| `var` | `:`，只有冒号 | `name : value` | 标量（null、bool、数字、字符串） |

- 有冒号无花括号 → `var`
- 有冒号有花括号 → `vec`
- 无冒号有花括号 → `stu`

`stu` 保留声明顺序，允许同名键；查找返回第一个匹配。这是刻意的：mini_gms 不在源文没有要求 map 语义的地方强加 map 语义。

### 2. 顶层声明必须带标签

顶层没有外层作用域，解析器需要标签才能知道类型：

```
[stu] config { ... }     ✅
config { ... }           ❌  （顶层没有标签）
```

即使只有一行的文件也必须带标签：

```
[var] a : 10
```

标签同时也是声明名字所在的位置：`[stu] name { ... }` 读作「一个名为 name 的 stu」，而不是「一个标签后面跟一个名字」。

### 3. 内层声明可省标签

在 `stu` 内部，形式已经暗示了类型，标签可以省略：

```
[stu] config {
    name   : "test"      # var
    window { ... }       # stu
    nums   : { 1 2 3 }   # vec
}
```

同样是三种形式，只有 `[tag]` 前缀是可选的。

### 4. 分隔与注释

没有逗号，也没有分号。元素之间靠空白分隔：空格、制表、换行。下面两段等价：

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

`#` 开始一条行注释，一直到行尾。可以单独占一行，也可以跟在任意 token 之后。字符串内的 `#` 不是注释。

```
# 整行注释
[stu] config {
    width : 1280    # 行尾注释
    color : "#FF0000"   # 这个 # 是字符串的一部分
}
```

### 5. 标量

| 类型 | 形式 | 例子 |
|---|---|---|
| null | 关键字 | `null` |
| 布尔 | 关键字 | `true`、`false` |
| 数字 | 可选 `-`、数字、可选 `.` | `100`、`-5`、`7.5` |
| 字符串 | 双引号或单引号 | `"hello"`、`'line\n'` |

整数和浮点统一存为 `double`，没有单独的整数类型。

### 6. 字符串：原始与转义

两种字符串形式，转义规则不同：

- `"..."`：原始字符串。不处理转义，`\n` 就是反斜杠加 `n`。**不能跨行。**
- `'...'`：转义字符串。处理 `\n`、`\t`、`\r`、`\'`、`\\`。**可以跨多行。**

```
[var] win_path : "C:\Users\test"    # 原始：反斜杠是字面量
[var] msg      : 'line1\nline2'     # 转义：\n 变成换行
[var] multi    : 'first
second'                             # 转义字符串可以跨行
```

Windows 路径、正则、任何反斜杠有意义的内容用原始形式。想要 `\n`、`\t` 或多行文本又不想拼接，用转义形式。

### 7. 嵌套

三种类型自由嵌套。

**stu 套 stu：**

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

**vec 放标量：**

```
[vec] numbers : { 1 2 3 4 5 }
```

**vec 放匿名 stu：**

```
[vec] cards : {
    { id : 1  name : "first" }
    { id : 2  name : "second" }
}
```

**vec 放匿名子 vec**（多维数组）：

```
[vec] matrix : {
    : { 1 2 3 }
    : { 4 5 6 }
    : { 7 8 9 }
}
```

`: { ... }` 是匿名子 `vec`，只作为 `vec` 元素出现；其他位置的 `:` 后面总是跟成员名。

### 8. 用宏复用

`&name` 是一个整体标识符：开头的 `&` 是名字的一部分，不是运算符，不引入任何新规则。**宏声明就是普通声明，只是名字以 `&` 开头**；标签规则照旧——顶层必须带 `[tag]`，`stu` 内部可省。

同一个名字有两种角色，由出现位置决定：

- **声明位置**：登记这个值。顶层写作 `[tag] &name ...`，`stu` 内部写作 `&name ...`。
- **值位置**：`:` 的右边，或 `vec` 内的元素位置。取出已登记的值进行替换。

顶层：

```
[var] &base_hp : 100              # 声明：登记 &base_hp
[var] player_hp : &base_hp        # 值位置：替换为 100
```

在 `stu` 内部（标签可省，和普通成员一致）：

```
[stu] config {
    &base { hp : 100  speed : 1.0 }

    enemy {
        stats : &base           # 替换为上面登记的 stu
        copy  : &base           # 同一个节点，不是拷贝
    }
}
```

规则：

- 引用得到的是**同一个节点**，不是拷贝。如果将来加入可变操作，改一处会处处可见。
- 宏必须先定义后引用。前向引用会抛 `undefined macro`。
- 同名重定义会覆盖前一个定义。
- 宏可以引用其他宏，只要各自在被引用前已定义。自引用（`&a { x : &a }`）在使用点上尚未定义，因此同样抛 `undefined macro`。

### 9. 范围展开

在 `vec` 内，`start~end` 展开为从 `start` 到 `end`（含端点）的每个整数。`start <= end` 时步长为 `+1`，否则为 `-1`。

```
[vec] up   : { 1~5 }      # 1 2 3 4 5
[vec] down : { 5~1 }      # 5 4 3 2 1
[vec] neg  : { -3~3 }     # -3 -2 -1 0 1 2 3
[vec] same : { 3~3 }      # 3
```

范围展开只发生在 `vec` 内。`~` 在其他位置没有意义，会报语法错误。

### 10. 批量声明

不带名字的 `[stu] { ... }` 块一次声明多个成员。每个成员的类型按形式推断，规则与 `stu` 内部相同。

```
[stu] {
    a : 1          # var
    b { c : 2 }    # stu
    d : { 1 2 3 }  # vec
}
```

这完全等价于写：

```
[var] a : 1
[stu] b { c : 2 }
[vec] d : { 1 2 3 }
```

批量块可以出现在顶层，也可以出现在 `stu` 内部。只有 `[stu]` 可以作为批量标签；`[vec] { ... }` 和 `[var] { ... }` 会报错，因为无法从多个值里推断出一个批量 `var` 应该产生什么。

### 11. 紧凑 stu 行（模板继承）

在 `vec` 内，第一个完整的 `stu` 成为模板。后续以标量开头的行按模板顺序补齐。

```
[vec] enemies : {
    { name : "Slime"  hp : 20 }   # 模板
    { "Bat"   10 }                # { name : "Bat"    hp : 10 }
    { "Ghost" 30 }                # { name : "Ghost"  hp : 30 }
}
```

规则：

- 值的数量必须与模板严格相等。
- 每个值必须是标量（数字、字符串、布尔、null）。紧凑行内不允许嵌套 `{}` 和 `:`。
- 在任何模板出现之前使用紧凑行会报错：没有东西可以补齐。
- 模板是该 `vec` 内第一个完整的 `stu`。如果需要别的顺序，先声明模板。

这是大型配置最需要的构件：一千个同构实体，写成一千行短行，而不是一千遍重复键名。

### 12. 综合示例

```
# 一个共享的"属性"宏，被玩家和敌人同时引用
[stu] &attributes { str : 10  dex : 8  int : 6 }

# 一个共享的"背包槽位"宏，被多处复用
[vec] &empty_slots : { null  null  null  null }

[stu] save {
    player {
        attributes : &attributes        # 引用共享 stu
        inventory  : &empty_slots       # 引用共享 vec
        name       : "Nova"
    }

    # 批量声明嵌套在 stu 内部：没有名字，成员类型由形式推断
    [stu] {
        gold    : 1200
        chapter : "prologue"
        checked : 'the gate\nis open'   # 转义字符串，跨行
    }

    # 一个 vec，第一行是模板，后续是紧凑行
    party : {
        { name : "Nova"   level : 1  hp : 30 }
        { "Iris"   2  25 }
        { "Kael"   3  40 }
    }

    # 嵌套 vec：每一章是一串任务编号
    quests : {
        : { 1~3 }       # 第 1 章：1 2 3
        : { 4~6 }       # 第 2 章：4 5 6
    }
}
```

这段文件里：

- `&attributes` 和 `&empty_slots` 是**两个不同类型的宏**——一个 `stu`、一个 `vec`，说明宏不限于某一种类型。
- `player.attributes` 和 `player.inventory` 是**值位置的宏引用**；前者替换为一个 `stu`，后者替换为一个 `vec`。
- `[stu] { ... }` 是**嵌套在 `stu` 内部的批量声明**，展开成 `gold`、`chapter`、`checked` 三个成员。
- `party` 的紧凑行说明模板继承不限于顶层 `vec`，任何 `vec` 都可以。
- `quests` 是**嵌套 vec + 范围展开**的组合：外层 `vec` 的每个元素是一个子 `vec`，子 `vec` 里是 `1~3` 这样的范围。

C++ 读取：

```cpp
auto doc = mol::MgmsDoc::parse(text);

auto str  = doc.get<mol::mgms_num>("save.player.attributes.str", 0.0); // 10
auto gold = doc.get<mol::mgms_num>("save.gold", 0.0);                  // 1200
auto q    = doc.get<mol::mgms_num>("save.quests[0][1]", 0.0);          // 2
```

Python 读取：

```python
data = mini_gms.loads(text)
print(data['save']['player']['attributes'])  # {'str': 10.0, 'dex': 8.0, 'int': 6.0}
print(data['save']['gold'])                  # 1200.0
print(data['save']['party'][1]['name'])      # Iris
print(data['save']['quests'][0])             # [1.0, 2.0, 3.0]
```

类型映射：

| mini_gms | Python |
|---|---|
| `stu` | `dict` |
| `vec` | `list` |
| `null` | `None` |
| `bool` | `bool` |
| `number` | `float` |
| `string` | `str` |

---

## 各语言接口

### C++

```cpp
#include "mini_gms.hpp"

using namespace mol;

// 解析
auto doc = MgmsDoc::parse(text);

// 按路径取标量
auto title = doc.get<mgms_str>("title", "");
auto hp    = doc.get<mgms_num>("enemies[0].hp", 0.0);

// 数组下标
auto first = doc.get<mgms_num>("levels[0]", 0.0);

// 必需值（缺失或类型不符时抛异常）
auto required = doc.require<mgms_num>("screen.width");

// 存在性检查
if (doc.has("title")) { ... }

// 取节点，手动遍历
auto enemies = doc.node("enemies");
if (enemies && enemies->isVec()) {
    for (auto& e : enemies->asVec()) { ... }
}
```

路径语法：

| 路径 | 含义 |
|---|---|
| `title` | 顶层 `title` |
| `levels[0]` | `levels` 的第 0 个元素 |
| `enemies[2].name` | 第三个敌人的 `name` |
| `hero.stats.hp` | 共享宏节点里的 `hp` |

类型别名：

```cpp
mol::mgms_null   // std::monostate
mol::mgms_bool   // bool
mol::mgms_num    // double
mol::mgms_str    // std::string
```

### Python

Python 包是对共享库的 `ctypes` 包装。共享库没有提交到仓库；在 `pip install .` 之前，请先按 [BUILD.md](BUILD.md) 完成构建和拷贝。

```bash
cd python
pip install .
```

```python
import mini_gms

# 解析为普通 Python 对象
data = mini_gms.loads(text)
print(data['title'])              # Starfall
print(data['levels'])             # [1.0, 2.0, 3.0, 4.0, 5.0]
print(data['enemies'][0]['hp'])   # 20.0
print(data['hero']['stats'])      # {'hp': 100.0, 'speed': 1.5}

# 保留 Document 以便按需访问
with mini_gms.parse(text) as doc:
    hero = doc['hero']
    print(hero['name'].value)     # Nova
    print(hero.keys())            # ['stats', 'name']
```

类型映射：

| mini_gms | Python |
|---|---|
| `stu` | `dict` |
| `vec` | `list` |
| `null` | `None` |
| `bool` | `bool` |
| `number` | `float` |
| `string` | `str` |

### C ABI

任何支持 C FFI 的语言都能调用动态库。

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

---

## 构建

见 [BUILD.md](BUILD.md)。

---

## 设计说明

**为什么顶层必须带标签。** 解析器需要确定性的入口。JSON 顶层用 `{}`，但 `{}` 不携带类型信息。mini_gms 把类型和名字放在同一个位置：`[stu] name`。内层声明可以省标签，因为外层作用域已经确定了类型。

**为什么可以省逗号、引号。** 空白分隔元素；键裸写；顶层标签携带类型。凡是解析器能推断的，语法里不写。这是刻意的取舍：写和解析都少打字，代价是读的人要记住推断规则。

**为什么宏定义与引用共用一套 `&name`。** `&` 只是名字的首字符，不引入任何新规则。宏声明就是普通声明，只是名字以 `&` 开头；标签规则、位置语义、查表行为，全部和普通声明一致。另一种做法是用两个符号——像 YAML 的 `&` 定义、`*` 引用——我们考虑过并放弃了。它会为一个一条规则就够的特性再加一个概念：`&name` 是名字；位置决定它做什么。语法高亮让两种角色可见，不需要第二个符号。

**为什么有批量声明和紧凑行。** 大型配置里有大量同构实体。写一万遍 `name : ..., hp : ...` 既浪费文件体积，也浪费人的注意力。批量声明去掉重复的 `[var]` / `[stu]` 标签；紧凑行去掉重复的键名。两者都是可选的：文件写全更清楚时，全写形式一直都在。

**为什么 `stu` 保序、允许同名键。** 源文本来就是有序的，模型如实反映。如果将来的特性需要 map 语义，可以在上面构建；模型不提前强加。

**为什么数字是 `double`。** 一种数值类型让值模型保持小、让 C ABI 保持简单。配置很少需要整数/浮点区分；需要时由使用方转换。

---

## 兼容性

mini_gms 1.1 与 1.0 完全向后兼容：

- 1.0 的 `.mgms` 文件仍然合法，语义不变。
- C++ API（`mol::MgmsDoc`、`mol::mgms::parse`）、C ABI（`mgms_*`）、Python API（`parse`、`loads`、`Document`、`Value`）均不变。
- 无需迁移。

1.1 新增了语法（宏、范围、转义字符串、批量声明、紧凑行），但不改变、不删除任何 1.0 构件。完整列表见 [CHANGELOG_CN.md](CHANGELOG_CN.md)。

---

## 许可证

MIT 许可证见 [LICENSE.txt](LICENSE.txt)。