# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What Builder Is

Builder is a metabuild system that lets developers configure C/C++ builds by writing C++ code instead of learning a separate build language. It is **not a compiler** — it compiles a user-provided `build.cpp`, loads it, calls `SetBuilderOptions()`, then invokes the real compiler (Clang/MSVC/GCC) with the resulting arguments.

## Commands

### Build Builder itself

**Windows:**
```bash
scripts/build.bat debug
scripts/build.bat release
```

**Linux:**
```bash
scripts/build.sh debug
scripts/build.sh release
```

### Build and run tests

**Windows:**
```bash
scripts/build_tests.bat debug
./bin/tests_main
```

**Linux:**
```bash
scripts/build_tests.sh debug
./bin/tests_main
```

### Create a release

```bash
scripts/make_release.bat   # Windows
scripts/make_release.sh    # Linux
```

### First-time setup

```bash
scripts/download_dependencies.bat   # Windows
scripts/download_dependencies.sh    # Linux
```

## Architecture

The build flow is:

```
CLI args → compile user's build.cpp → load as shared lib → call SetBuilderOptions()
         → select config → initialize compiler backend → compile sources → link
         → (optionally) generate compile_commands.json or VS solution
```

### Key source files

| File | Role |
|------|------|
| `src/builder.cpp` | Core orchestration — argument parsing, config loading, build loop, incremental builds |
| `src/backend_clang.cpp` | Clang/LLVM backend — compiles sources, generates `.d` dependency files, links |
| `src/backend_msvc.cpp` | MSVC backend — auto-detects MSVC install and Windows SDK, translates config to cl.exe/LINK.exe args |
| `src/visual_studio.cpp` | Generates `.sln` and `.vcxproj` Makefile projects that delegate to Builder |
| `src/win_support.cpp` | Windows SDK and MSVC path detection |
| `src/builder_local.h` | Internal types: compiler backend function pointer table, `buildContext_t` |
| `include/builder.h` | Public API — `BuildConfig`, `BuilderOptions`, `BUILDER_CALLBACK`, enums |
| `src/main.cpp` | Thin entry point calling `BuilderMain()` |
| `src/core/` | Foundation: memory, string, file I/O, process execution, hashing, hashmaps |
| `tests/` | Test suite using `tests/temper/temper.h` |

### Compiler backend abstraction

Backends (`backend_clang.cpp`, `backend_msvc.cpp`) export a factory function (`CreateCompilerBackend_Clang`, `CreateCompilerBackend_MSVC`) that fills a function pointer table defined in `builder_local.h`. `builder.cpp` calls through these pointers for `Init`, `CompileSourceFile`, and `LinkIntermediateFiles`.

### Incremental builds

Clang backend uses `-MMD` to emit `.d` dependency files per translation unit. On subsequent builds, Builder hashes source files and their transitive headers; if unchanged, the `.o` is reused.

### Config dependencies

`BuildConfig` has a `dependsOn` field. Builder resolves the dependency graph and builds prerequisite configs (e.g., a static library) before the config that depends on them.

## Code conventions

- Custom integer types: `u8`, `s32`, `u64`, `float64`, `bool8` (from `src/core/core_types.h`)
- `String` wraps a C string (from core)
- Memory: `mem_alloc()` / `mem_free()`; `defer(...)` for cleanup
- Errors: `assertf()` for programmer errors; `error()` / `fatal_error()` for runtime errors
- Structs use `_t` suffix for internal types
- Warnings-as-errors (`-Werror`) with `-Wall -Wextra -Weverything -Wpedantic`; suppressed warnings are explicitly listed in the build scripts
