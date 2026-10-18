# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-XX-XX

### Added
- Macros: define with `&name { ... }` and reference with `&name`
- Range expansion inside `vec`: `1~5` and `5~1` (inclusive, step ±1)
- Single-quoted escaped strings: `'...'`, supporting `\n \t \r \' \\` and spanning multiple lines
- Batch declarations: `[stu] { ... }` at top level and inside a `stu`
- Compact `stu` rows inside a `vec`: `{ 1 2 }` filled against the most recent `stu` header

### Fixed
- `mgms_parse` now rejects a null source buffer instead of relying on undefined behavior
- `mgms_parse` no longer leaks the document when parsing throws
- Python `Value.__eq__` no longer raises on non-scalar nodes; it returns `NotImplemented` instead

### Changed
- Build artifacts are now written to `build/lib/` instead of the source tree `lib/`
- The dll is no longer committed to the repository; see `BUILD.md` for the build-and-copy steps
- Parser internals moved to the `mol::mgms` namespace; the public API stays in `mol`

### Compatibility
- Source files written for 1.0 remain valid and keep the same semantics
- C++ API (`mol::Document` / `mol::parse`), C ABI (`mgms_*`), and Python API (`parse` / `loads` / `Document` / `Value`) are unchanged
- No migration is required

## [1.0.0] - 2026-10-01

### Added
- Initial stable release
- Three data types: `stu` / `vec` / `var`
- C++ parser (`mol::parse`)
- C++ convenience layer (`mol::Document`)
- C ABI (`mini_gms_c.h`)
- Python binding (`mini_gms`)
- `#` line comments
- CMake build

[1.1.0]: https://github.com/moleia77/mini_gms/releases/tag/v1.1.0
[1.0.0]: https://github.com/moleia77/mini_gms/releases/tag/v1.0.0