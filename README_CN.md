[English](README.md) | [中文](README_CN.md)

# mini_gms

一种轻量、解析友好的配置语言

mini_gms 是在开发 **GMS** 的过程中拆分出的数据配置子集，承担类似于 JSON 的职责

![version](https://img.shields.io/badge/version-1.0.0-blue)
![license](https://img.shields.io/badge/license-MIT-green)
![platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)
![language](https://img.shields.io/badge/C%2B%2B-17-blue)

---

## 目录

- [关于](#关于)
- [语法](#语法)
- [各语言接口](#各语言接口)
- [构建](#构建)
- [设计说明](#设计说明)
- [贡献](#贡献)
- [许可证](#许可证)

---

## 关于

mini_gms 是一种配置语言，有三种数据类型、极少的符号、顶层带标签

- **三种数据**：`[stu]`(结构体)、`[vec]`(数组)、`[var]`(标量)
- **符号极少**：无逗号、无分号、键裸写
- **顶层带标签**：`[stu] name { ... }`，入口同时携带类型和名字
- **注释**：`#` 行注释
- **多语言**：C++ 原生、C ABI、Python 绑定

mini_gms 源文件通常保存为 `.mgms` 扩展名，例如 `config.mgms`

### 关于 GMS

GMS (Game Markup Script) 是一个面向游戏引擎的 DSL 脚本，目前仍在开发中，尚未发布
mini_gms 是它的数据配置子集，被拆分出来以便单独使用

### 示例

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

C++：

```cpp
#include "mini_gms.hpp"

auto doc = mol::Document::parse(text);

auto name = doc.get<mol::string>("app.name", "");            // "MyApp"
auto w    = doc.get<mol::number>("app.window.width", 0.0);   // 1280
auto host = doc.get<mol::string>("app.servers[0].host", ""); // "alpha.example.com"
```

Python：

```python
import mini_gms

data = mini_gms.loads(text)
print(data['app']['name'])               # MyApp
print(data['app']['window']['width'])    # 1280.0
print(data['app']['servers'][0]['host']) # alpha.example.com
```

---

## 语法

### 三种数据

mini_gms 只有三种数据类型由两个符号决定：

| 类型 | 标记 | 写法 |
|---|---|---|
| `stu` | `{}`，无冒号 | `[stu] name { ... }` |
| `vec` | `:{}`，有冒号 | `[vec] name : { ... }` |
| `var` | `:`，只有冒号 | `[var] name : value` |

```
name { ... }        → stu
name : { ... }      → vec
name : value        → var
```

- 有冒号无花括号 → `var`
- 有冒号有花括号 → `vec`
- 无冒号有花括号 → `stu`

### 顶层必须带标签

```
[stu] config { ... }     ✅
config { ... }           ❌
```

顶层没有外层作用域，解析器依赖标签判断类型即使文件只有一行，也必须带标签：

```
[var] a : 10
```

### 内层可省标签

`stu` 内部可省标签，因为写法已经暗示了类型：

```
[stu] config {
    name : "test"          # var
    window { ... }         # stu
    nums : { 1 2 3 }       # vec
}
```

### 分隔

无逗号、无分号元素之间靠空白(空格、换行、制表)分隔

```
[vec] numbers : { 1 2 3 }
```

等价于：

```
[vec] numbers : {
    1
    2
    3
}
```

### 标量类型

| 类型 | 写法 | 例子 |
|---|---|---|
| 字符串 | 双引号 | `"hello"` |
| 数字 | 整数或浮点 | `100`、`-5`、`7.5` |
| 布尔 | 关键字 | `true`、`false` |
| 空值 | 关键字 | `null` |

整数和浮点统一为 `double`

### 嵌套

**stu 套 stu**：

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

**vec 放标量**：

```
[vec] numbers : { 1 2 3 4 5 }
```

**vec 放匿名 stu**：

```
[vec] cards : {
    { id : 1  name : "first" }
    { id : 2  name : "second" }
}
```

**vec 放匿名子数组**(多维数组)：

```
[vec] matrix : {
    : { 1 2 3 }
    : { 4 5 6 }
    : { 7 8 9 }
}
```

### 注释

`#` 行注释，可放任意位置：

```
# 整行注释
[stu] config {
    width : 1280    # 行尾注释
    height : 720
}
```

字符串内的 `#` 不是注释：

```
color : "#FF0000"    # 这才是注释
```

### 完整示例

```
# 应用配置
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

## 各语言接口

### C++

```cpp
#include "mini_gms.hpp"

using namespace mol;

// 解析
auto doc = Document::parse(text);

// 按路径取标量
auto name = doc.get<string>("config.title", "");
auto w    = doc.get<number>("config.window.width", 0.0);
auto dbg  = doc.get<boolean>("config.debug", false);

// 数组索引
auto first = doc.get<number>("config.nums[0]", 0.0);

// 一定有值(缺失或类型不匹配抛异常)
auto required = doc.require<number>("config.window.width");

// 判断存在
if (doc.has("config.title")) { ... }

// 取节点自己遍历
auto nums = doc.node("config.nums");
if (nums && nums->isVec()) {
    for (auto& e : nums->asVec()) { ... }
}
```

路径语法：

| 路径 | 含义 |
|---|---|
| `config.title` | 顶层 `config` 的 `title` |
| `config.nums[0]` | `nums` 的第 0 个元素 |
| `config.players[2].name` | 第 2 个玩家的 `name` |

类型别名：

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

# 解析成 dict
data = mini_gms.loads(text)
print(data['config']['title'])          # MyApp
print(data['config']['window']['width']) # 1280.0

# 保留 Document 按需访问
with mini_gms.parse(text) as doc:
    cfg = doc['config']
    print(cfg['title'].value)           # MyApp
    print(cfg.keys())                   # ['title', 'version', 'debug', ...]
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

任何支持 C FFI 的语言都能调用动态库

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

完整函数列表见 `include/mini_gms_c.h`

---

## 构建

见 [BUILD.md](mini_gms/BUILD.md)

---

## 设计说明

### 为什么顶层必须带标签

解析器需要确定性的入口,JSON 顶层用 `{}` 当入口，但 `{}` 不携带类型信息mini_gms 用 `[stu] name`——同一个位置同时给出类型和名字

内层可省标签，是因为外层作用域已经确定了类型进了 `stu`，靠 `{}` / `:{}` / `:` 的写法就够

### 为什么可以省逗号、引号

- **无逗号**：空白能分隔元素
- **键裸写**：解析器能靠位置定位键边界
- **顶层带标签**：入口自带类型

**凡是解析器能推断的，语法里不写**

### 和 JSON 对比

| | JSON | mini_gms |
|---|---|---|
| 顶层 | `{ ... }`(匿名容器) | `[stu] name { ... }`(类型 + 名字) |
| 键 | 必须引号 | 裸写 |
| 分隔 | 逗号 | 空白 |
| 注释 | 不支持 | `#` 行注释 |
| 顶层单值 | 必须包 `{}` | `[var] a : 10` 一行 |

mini_gms 的语法来自 GMS，不是为对标 JSON 设计的

---

## 贡献

开源项目是协作与慷慨的图景请通过下面的链接提出讨论、提交 PR，或邮件联系

[请求新功能][feature-request-url]<br>
[报告 Bug][bug-report-url]

---

## 许可证

[![License][license-shield]][license-url]

MIT 许可证见 [LICENSE.txt](LICENSE.txt)

---

[feature-request-url]: https://github.com/moleia77/mini_gms/issues/new?labels=enhancement&template=feature-request.md
[bug-report-url]: https://github.com/moleia77/mini_gms/issues/new?labels=bug&template=bug-report.md

[license-shield]: https://img.shields.io/github/license/moleia77/mini_gms.svg?style=for-the-badge
[license-url]: https://github.com/moleia77/mini_gms/blob/main/LICENSE.txt