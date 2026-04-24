# Builder API Reference

Tabulated reference for [`include/builder.h`](include/builder.h).

For examples see the [`README.md`](README.md).

## `BuildConfig`

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Config name, selected at the command line via `--config=`. Must be unique. Required when multiple configs are defined. |
| `binaryName` | `string` | Output binary name. File extension is added automatically unless `removeFileExtension` is set. |
| `binaryFolder` | `string` | Output folder, relative to the build file. Created if it doesn't exist. |
| `intermediateFolder` | `string` | Where `.o` files go, relative to `binaryFolder`. Defaults to `binaryFolder` if unset. |
| `binaryType` | `BinaryType` | `BINARY_TYPE_EXE` (default), `BINARY_TYPE_STATIC_LIBRARY`, `BINARY_TYPE_DYNAMIC_LIBRARY`. |
| `sourceFiles` | `string[]` | Source files to compile. Paths are relative to the build file. Supports wildcards (`src/**/*.cpp`). |
| `defines` | `string[]` | Preprocessor defines. The `-D` prefix is added automatically. |
| `additionalIncludes` | `string[]` | Additional include paths. |
| `additionalLibPaths` | `string[]` | Additional library search paths (`-L` / `/LIBPATH:`). |
| `additionalLibs` | `string[]` | Libraries to link against (`-l` / `.lib`). File extension not required on Windows. |
| `dependsOn` | `BuildConfig[]` | Configs that must be built before this one. Registered automatically when passed to `AddBuildConfig`. |
| `languageVersion` | `LanguageVersion` | e.g. `LANGUAGE_VERSION_CPP20`, `LANGUAGE_VERSION_C99`. Sets `-std` / `/std`. |
| `optimizationLevel` | `OptimizationLevel` | `OPTIMIZATION_LEVEL_O0`–`O3`. MSVC has no `/O3` - Builder warns and falls back to `/O2`. |
| `warningLevels` | `string[]` | Warning groups to enable, e.g. `{ "-Wall", "-Wextra" }`. None enabled by default. |
| `ignoreWarnings` | `string[]` | Warnings to suppress, e.g. `{ "-Wno-unused-parameter" }` / `/wd`. |
| `additionalCompilerArguments` | `string[]` | Appended verbatim to compiler args. |
| `additionalLinkerArguments` | `string[]` | Appended verbatim to linker args. |
| `warningsAsErrors` | `bool` | Treat warnings as errors. |
| `removeSymbols` | `bool` | Strip debug symbols from the binary. |
| `removeFileExtension` | `bool` | Strip the file extension from the output binary name. |
| `OnPreBuild` | `void (*)()` | Called just before compilation. |
| `OnPostBuild` | `void (*)()` | Called just after compilation. |

## `BuilderOptions`

| Field | Type | Description |
|-------|------|-------------|
| `compilerPath` | `string` | Path to the compiler. Set to `"cl"` for MSVC auto-detection. Defaults to Builder's bundled Clang. |
| `compilerVersion` | `string` | Expected compiler version. Builder warns if the actual version doesn't match. Useful for keeping teams on the same toolchain. |
| `forceRebuild` | `bool` | Rebuild all binaries and intermediate files regardless of timestamps. |
| `consolidateCompilerArgs` | `bool` | Print shared compiler args once instead of repeating them for every source file. |
| `linkAgainstWindowsDynamicRuntime` | `bool` | Link the Windows dynamic CRT instead of static. Windows only. |
| `noDefaultLibs` | `bool` | Skip default library linking. MSVC: `/NODEFAULTLIB`. Linux Clang: `-nodefaultlibs`. No-op for static library builds. |
| `generateSolution` | `bool` | Generate a Visual Studio solution. Skips the code build. |
| `generateVSCodeJSONFiles` | `bool` | Generate `.vscode/tasks.json` and `.vscode/launch.json`. |
| `generateZedJSONFiles` | `bool` | Generate `.zed/tasks.json` and `.zed/debug.json`. |
| `generateCompilationDatabase` | `bool` | Generate `compile_commands.json` on a successful build. |
| `solution` | `VisualStudioSolution` | Visual Studio solution settings. See below. |
| `vsCodeJSONOptions` | `VSCodeJSONOptions` | VS Code JSON generation settings. See below. |
| `zedJSONOptions` | `ZedJSONOptions` | Zed JSON generation settings. See below. |

---

## `VisualStudioSolution`

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Solution name, also used as the `.sln` filename. |
| `path` | `string` | Folder where the solution and projects are generated, relative to the build file. |
| `platforms` | `string[]` | Target platforms, e.g. `{ "x64" }`. |
| `projects` | `VisualStudioProject[]` | Projects in the solution. |

## `VisualStudioProject`

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Project name as shown in Visual Studio. Slashes create project folders, e.g. `"tools/my-tool"`. |
| `configs` | `VisualStudioConfig[]` | Build configurations for this project. |
| `codeFolders` | `string[]` | Folders to scan for source files shown in the Solution Explorer. Defaults to the source folders from each config if empty. |
| `fileExtensions` | `string[]` | File extensions to include when scanning `codeFolders`. Defaults to `c, cpp, cc, cxx, h, hpp, inl` if empty. |

## `VisualStudioConfig`

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Config name as shown in Visual Studio, e.g. `"debug"`. Does not need to be unique. |
| `options` | `BuildConfig` | The `BuildConfig` to build when this Visual Studio config is selected. |
| `additionalBuildArgs` | `string[]` | Extra arguments appended to the Builder command line when building this config. |
| `debuggerArguments` | `string[]` | Default debugger command line arguments. |

---

## `VSCodeJSONOptions`

| Field | Type | Description |
|-------|------|-------------|
| `builderPath` | `string` | Path to the Builder executable VS Code will invoke. Defaults to `"builder"` (assumes Builder is on `PATH`). |
| `taskConfigs` | `VSCodeTaskConfig[]` | Build tasks written to `tasks.json`. |
| `launchConfigs` | `VSCodeLaunchConfig[]` | Launch/debug configurations written to `launch.json`. |

## `VSCodeTaskConfig`

| Field | Type | Description |
|-------|------|-------------|
| `config` | `BuildConfig` | The config to build. |
| `additionalBuildArgs` | `string[]` | Extra arguments appended to the Builder command line for this task. |

## `VSCodeLaunchConfig`

| Field | Type | Description |
|-------|------|-------------|
| `binaryName` | `string` | Path to the binary to launch. |
| `args` | `string[]` | Command line arguments passed to the binary. |
| `cwd` | `string` | Working directory. Defaults to `${workspaceFolder}`. |
| `debuggerType` | `VSCodeDebuggerType` | `VSCODE_DEBUGGER_TYPE_CPPVSDBG` (Windows MSVC), `VSCODE_DEBUGGER_TYPE_CPPDBG_GDB`, `VSCODE_DEBUGGER_TYPE_CPPDBG_LLDB`. |

---

## `ZedJSONOptions`

| Field | Type | Description |
|-------|------|-------------|
| `builderPath` | `string` | Path to the Builder executable Zed will invoke. Defaults to `"builder"` (assumes Builder is on `PATH`). |
| `taskConfigs` | `ZedTaskConfig[]` | Build tasks written to `.zed/tasks.json`. |
| `debugConfigs` | `ZedDebugConfig[]` | Debug configurations written to `.zed/debug.json`. |

## `ZedTaskConfig`

| Field | Type | Description |
|-------|------|-------------|
| `config` | `BuildConfig` | The config to build. |
| `args` | `string[]` | Extra arguments appended to the Builder command line for this task. |

## `ZedDebugConfig`

| Field | Type | Description |
|-------|------|-------------|
| `binaryName` | `string` | Path to the binary to debug. |
| `args` | `string[]` | Command line arguments passed to the binary. |
| `cwd` | `string` | Working directory. Defaults to `${ZED_WORKTREE_ROOT}`. |
