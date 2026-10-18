# 更新日志

本项目所有值得记录的变更都会写在这里。

格式基于 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，
版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [1.1.0] - 2026-XX-XX

### 新增
- 宏：用 `&name { ... }` 定义、用 `&name` 引用
- `vec` 内的范围展开：`1~5` 与 `5~1`（含端点，步长 ±1）
- 单引号转义字符串：`'...'`，支持 `\n \t \r \' \\`，可跨行
- 批量声明：顶层与 `stu` 内部的 `[stu] { ... }`
- `vec` 内的紧凑 `stu` 行：`{ 1 2 }`，按最近的 `stu` 表头补齐

### 修复
- `mgms_parse` 拒绝空源缓冲区，不再依赖未定义行为
- `mgms_parse` 在解析抛异常时不再泄漏文档
- Python `Value.__eq__` 对非标量节点不再抛异常，改为返回 `NotImplemented`

### 变更
- 构建产物输出到 `build/lib/`，不再写入源码树 `lib/`
- dll 不再提交进仓库；构建与复制步骤见 `BUILD.md`
- 解析器内部移入 `mol::mgms` 命名空间；公开 API 仍在 `mol`

### 兼容性
- 1.0 的源文件仍然合法，语义不变
- C++ API（`mol::Document` / `mol::parse`）、C ABI（`mgms_*`）、Python API（`parse` / `loads` / `Document` / `Value`）均不变
- 无需迁移

## [1.0.0] - 2026-10-01

### 新增
- 首个稳定版本
- 三种数据：`[stu]` / `[vec]` / `[var]`
- C++ 解析器（`mol::parse`）
- C++ 使用层（`mol::Document`）
- C ABI（`mini_gms_c.h`）
- Python 绑定（`mini_gms`）
- `#` 行注释
- CMake 构建

[1.1.0]: https://github.com/moleia77/mini_gms/releases/tag/v1.1.0
[1.0.0]: https://github.com/moleia77/mini_gms/releases/tag/v1.0.0